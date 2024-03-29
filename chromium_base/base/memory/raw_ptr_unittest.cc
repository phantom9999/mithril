// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_ptr.h"

#include <climits>
#include <cstddef>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

#include "base/allocator/buildflags.h"
#include "base/allocator/partition_alloc_features.h"
#include "base/allocator/partition_alloc_support.h"
#include "base/allocator/partition_allocator/dangling_raw_ptr_checks.h"
#include "base/allocator/partition_allocator/partition_alloc.h"
#include "base/allocator/partition_allocator/partition_alloc_base/numerics/checked_math.h"
#include "base/allocator/partition_allocator/partition_alloc_buildflags.h"
#include "base/allocator/partition_allocator/partition_alloc_config.h"
#include "base/allocator/partition_allocator/partition_alloc_constants.h"
#include "base/allocator/partition_allocator/tagging.h"
#include "base/cpu.h"
#include "base/logging.h"
#include "base/memory/raw_ptr_asan_service.h"
#include "base/memory/raw_ref.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/gtest_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "build/build_config.h"
#include "build/buildflag.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

#if BUILDFLAG(ENABLE_BASE_TRACING) && BUILDFLAG(PA_USE_BASE_TRACING)
#include "third_party/perfetto/include/perfetto/test/traced_value_test_support.h"  // no-presubmit-check nogncheck
#endif  // BUILDFLAG(ENABLE_BASE_TRACING) && BUILDFLAG(PA_USE_BASE_TRACING)

#if defined(RAW_PTR_USE_MTE_CHECKED_PTR)
#include "base/allocator/partition_allocator/partition_tag_types.h"
#endif

#if BUILDFLAG(USE_ASAN_BACKUP_REF_PTR)
#include <sanitizer/asan_interface.h>
#endif

using testing::AllOf;
using testing::HasSubstr;
using testing::Test;

static_assert(sizeof(raw_ptr<void>) == sizeof(void*),
              "raw_ptr shouldn't add memory overhead");
static_assert(sizeof(raw_ptr<int>) == sizeof(int*),
              "raw_ptr shouldn't add memory overhead");
static_assert(sizeof(raw_ptr<std::string>) == sizeof(std::string*),
              "raw_ptr shouldn't add memory overhead");

#if !BUILDFLAG(USE_BACKUP_REF_PTR)
// |is_trivially_copyable| assertion means that arrays/vectors of raw_ptr can
// be copied by memcpy.
static_assert(std::is_trivially_copyable<raw_ptr<void>>::value,
              "raw_ptr should be trivially copyable");
static_assert(std::is_trivially_copyable<raw_ptr<int>>::value,
              "raw_ptr should be trivially copyable");
static_assert(std::is_trivially_copyable<raw_ptr<std::string>>::value,
              "raw_ptr should be trivially copyable");

// |is_trivially_default_constructible| assertion helps retain implicit default
// constructors when raw_ptr is used as a union field.  Example of an error
// if this assertion didn't hold:
//
//     ../../base/trace_event/trace_arguments.h:249:16: error: call to
//     implicitly-deleted default constructor of 'base::trace_event::TraceValue'
//         TraceValue ret;
//                    ^
//     ../../base/trace_event/trace_arguments.h:211:26: note: default
//     constructor of 'TraceValue' is implicitly deleted because variant field
//     'as_pointer' has a non-trivial default constructor
//       raw_ptr<const void> as_pointer;
static_assert(std::is_trivially_default_constructible<raw_ptr<void>>::value,
              "raw_ptr should be trivially default constructible");
static_assert(std::is_trivially_default_constructible<raw_ptr<int>>::value,
              "raw_ptr should be trivially default constructible");
static_assert(
    std::is_trivially_default_constructible<raw_ptr<std::string>>::value,
    "raw_ptr should be trivially default constructible");
#endif  // !BUILDFLAG(USE_BACKUP_REF_PTR)

// Don't use base::internal for testing raw_ptr API, to test if code outside
// this namespace calls the correct functions from this namespace.
namespace {

using RawPtrCountingImpl =
    base::internal::RawPtrCountingImplWrapperForTest<base::DefaultRawPtrType>;
using RawPtrCountingMayDangleImpl =
    base::internal::RawPtrCountingImplWrapperForTest<base::RawPtrMayDangle>;

template <typename T>
using CountingRawPtr = raw_ptr<T, RawPtrCountingImpl>;

template <typename T>
using CountingRawPtrMayDangle = raw_ptr<T, RawPtrCountingMayDangleImpl>;

struct MyStruct {
  int x;
};

struct Base1 {
  explicit Base1(int b1) : b1(b1) {}
  int b1;
};

struct Base2 {
  explicit Base2(int b2) : b2(b2) {}
  int b2;
};

struct Derived : Base1, Base2 {
  Derived(int b1, int b2, int d) : Base1(b1), Base2(b2), d(d) {}
  int d;
};

class RawPtrTest : public Test {
 protected:
  void SetUp() override {
    RawPtrCountingImpl::ClearCounters();
    RawPtrCountingMayDangleImpl::ClearCounters();
  }
};

// Struct intended to be used with designated initializers and passed
// to the `CountingRawPtrHas()` matcher.
struct CountingRawPtrExpectations {
  absl::optional<int> wrap_raw_ptr_cnt;
  absl::optional<int> release_wrapped_ptr_cnt;
  absl::optional<int> get_for_dereference_cnt;
  absl::optional<int> get_for_extraction_cnt;
  absl::optional<int> get_for_comparison_cnt;
  absl::optional<int> wrapped_ptr_swap_cnt;
  absl::optional<int> wrapped_ptr_less_cnt;
  absl::optional<int> pointer_to_member_operator_cnt;
};

#define REPORT_UNEQUAL_RAW_PTR_COUNTER(member_name, CounterClassImpl) \
  {                                                                   \
    if (arg.member_name.has_value() &&                                \
        arg.member_name.value() != CounterClassImpl::member_name) {   \
      *result_listener << "Expected `" #member_name "` to be "        \
                       << arg.member_name.value() << " but got "      \
                       << CounterClassImpl::member_name << "; ";      \
      result = false;                                                 \
    }                                                                 \
  }
#define REPORT_UNEQUAL_RAW_PTR_COUNTERS(result, CounterClassImpl)             \
  {                                                                           \
    result = true;                                                            \
    REPORT_UNEQUAL_RAW_PTR_COUNTER(wrap_raw_ptr_cnt, CounterClassImpl)        \
    REPORT_UNEQUAL_RAW_PTR_COUNTER(release_wrapped_ptr_cnt, CounterClassImpl) \
    REPORT_UNEQUAL_RAW_PTR_COUNTER(get_for_dereference_cnt, CounterClassImpl) \
    REPORT_UNEQUAL_RAW_PTR_COUNTER(get_for_extraction_cnt, CounterClassImpl)  \
    REPORT_UNEQUAL_RAW_PTR_COUNTER(get_for_comparison_cnt, CounterClassImpl)  \
    REPORT_UNEQUAL_RAW_PTR_COUNTER(wrapped_ptr_swap_cnt, CounterClassImpl)    \
    REPORT_UNEQUAL_RAW_PTR_COUNTER(wrapped_ptr_less_cnt, CounterClassImpl)    \
    REPORT_UNEQUAL_RAW_PTR_COUNTER(pointer_to_member_operator_cnt,            \
                                   CounterClassImpl)                          \
  }

// Matcher used with `CountingRawPtr`. Provides slightly shorter
// boilerplate for verifying counts.

// Implicit `arg` has type `CountingRawPtrExpectations`.
MATCHER(CountingRawPtrHasCounts, "`CountingRawPtr` has specified counters") {
  bool result = true;
  REPORT_UNEQUAL_RAW_PTR_COUNTERS(result, RawPtrCountingImpl);
  return result;
}

// Implicit `arg` has type `CountingRawPtrExpectations`.
MATCHER(MayDangleCountingRawPtrHasCounts,
        "`MayDangleCountingRawPtr` has specified counters") {
  bool result = true;
  REPORT_UNEQUAL_RAW_PTR_COUNTERS(result, RawPtrCountingMayDangleImpl);
  return result;
}

#undef REPORT_UNEQUAL_RAW_PTR_COUNTERS
#undef REPORT_UNEQUAL_RAW_PTR_COUNTER

// Use this instead of std::ignore, to prevent the instruction from getting
// optimized out by the compiler.
volatile int g_volatile_int_to_ignore;

TEST_F(RawPtrTest, NullStarDereference) {
  raw_ptr<int> ptr = nullptr;
  EXPECT_DEATH_IF_SUPPORTED(g_volatile_int_to_ignore = *ptr, "");
}

TEST_F(RawPtrTest, NullArrowDereference) {
  raw_ptr<MyStruct> ptr = nullptr;
  EXPECT_DEATH_IF_SUPPORTED(g_volatile_int_to_ignore = ptr->x, "");
}

TEST_F(RawPtrTest, NullExtractNoDereference) {
  CountingRawPtr<int> ptr = nullptr;
  // No dereference hence shouldn't crash.
  int* raw = ptr;
  std::ignore = raw;
  EXPECT_THAT((CountingRawPtrExpectations{.get_for_dereference_cnt = 0,
                                          .get_for_extraction_cnt = 1,
                                          .get_for_comparison_cnt = 0}),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, NullCmpExplicit) {
  CountingRawPtr<int> ptr = nullptr;
  EXPECT_TRUE(ptr == nullptr);
  EXPECT_TRUE(nullptr == ptr);
  EXPECT_FALSE(ptr != nullptr);
  EXPECT_FALSE(nullptr != ptr);
  // No need to unwrap pointer, just compare against 0.
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, NullCmpBool) {
  CountingRawPtr<int> ptr = nullptr;
  EXPECT_FALSE(ptr);
  EXPECT_TRUE(!ptr);
  // No need to unwrap pointer, just compare against 0.
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

void FuncThatAcceptsBool(bool b) {}

bool IsValidNoCast(CountingRawPtr<int> ptr) {
  return !!ptr;  // !! to avoid implicit cast
}
bool IsValidNoCast2(CountingRawPtr<int> ptr) {
  return ptr && true;
}

TEST_F(RawPtrTest, BoolOpNotCast) {
  CountingRawPtr<int> ptr = nullptr;
  volatile bool is_valid = !!ptr;  // !! to avoid implicit cast
  is_valid = ptr || is_valid;      // volatile, so won't be optimized
  if (ptr)
    is_valid = true;
  [[maybe_unused]] bool is_not_valid = !ptr;
  if (!ptr)
    is_not_valid = true;
  std::ignore = IsValidNoCast(ptr);
  std::ignore = IsValidNoCast2(ptr);
  FuncThatAcceptsBool(!ptr);
  // No need to unwrap pointer, just compare against 0.
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

bool IsValidWithCast(CountingRawPtr<int> ptr) {
  return ptr;
}

// This test is mostly for documentation purposes. It demonstrates cases where
// |operator T*| is called first and then the pointer is converted to bool,
// as opposed to calling |operator bool| directly. The former may be more
// costly, so the caller has to be careful not to trigger this path.
TEST_F(RawPtrTest, CastNotBoolOp) {
  CountingRawPtr<int> ptr = nullptr;
  [[maybe_unused]] bool is_valid = ptr;
  is_valid = IsValidWithCast(ptr);
  FuncThatAcceptsBool(ptr);
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 3,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, StarDereference) {
  int foo = 42;
  CountingRawPtr<int> ptr = &foo;
  EXPECT_EQ(*ptr, 42);
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 1,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, ArrowDereference) {
  MyStruct foo = {42};
  CountingRawPtr<MyStruct> ptr = &foo;
  EXPECT_EQ(ptr->x, 42);
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 1,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, Delete) {
  CountingRawPtr<int> ptr = new int(42);
  delete ptr;
  // The pointer was extracted using implicit cast before passing to |delete|.
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 1,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, ClearAndDelete) {
  CountingRawPtr<int> ptr(new int);
  ptr.ClearAndDelete();

  // TODO(crbug.com/1346513): clang-format has a difficult time making
  // sense of preprocessor arms mixed with designated initializers.
  //
  // clang-format off
  EXPECT_THAT((CountingRawPtrExpectations {
                .wrap_raw_ptr_cnt = 1,
                .release_wrapped_ptr_cnt = 1,
                .get_for_dereference_cnt = 0,
                .get_for_extraction_cnt = 1,
                .wrapped_ptr_swap_cnt = 0,
              }),
              CountingRawPtrHasCounts());
  // clang-format on
  EXPECT_EQ(ptr.get(), nullptr);
}

TEST_F(RawPtrTest, ClearAndDeleteArray) {
  CountingRawPtr<int> ptr(new int[8]);
  ptr.ClearAndDeleteArray();

  // TODO(crbug.com/1346513): clang-format has a difficult time making
  // sense of preprocessor arms mixed with designated initializers.
  //
  // clang-format off
  EXPECT_THAT((CountingRawPtrExpectations {
                .wrap_raw_ptr_cnt = 1,
                .release_wrapped_ptr_cnt = 1,
                .get_for_dereference_cnt = 0,
                .get_for_extraction_cnt = 1,
                .wrapped_ptr_swap_cnt = 0,
              }),
              CountingRawPtrHasCounts());
  // clang-format on
  EXPECT_EQ(ptr.get(), nullptr);
}

TEST_F(RawPtrTest, ExtractAsDangling) {
  CountingRawPtr<int> ptr(new int);

  if constexpr (std::is_same_v<RawPtrCountingImpl,
                               RawPtrCountingMayDangleImpl>) {
    auto expectations = CountingRawPtrExpectations{
        .wrap_raw_ptr_cnt = 1,
        .release_wrapped_ptr_cnt = 0,
        .get_for_dereference_cnt = 0,
        .wrapped_ptr_swap_cnt = 0,
    };
    EXPECT_THAT((expectations), CountingRawPtrHasCounts());
    EXPECT_THAT((expectations), MayDangleCountingRawPtrHasCounts());
  } else {
    EXPECT_THAT((CountingRawPtrExpectations{
                    .wrap_raw_ptr_cnt = 1,
                    .release_wrapped_ptr_cnt = 0,
                    .get_for_dereference_cnt = 0,
                    .wrapped_ptr_swap_cnt = 0,
                }),
                CountingRawPtrHasCounts());
    EXPECT_THAT((CountingRawPtrExpectations{
                    .wrap_raw_ptr_cnt = 0,
                    .release_wrapped_ptr_cnt = 0,
                    .get_for_dereference_cnt = 0,
                    .wrapped_ptr_swap_cnt = 0,
                }),
                MayDangleCountingRawPtrHasCounts());
  }

  EXPECT_TRUE(ptr.get());

  CountingRawPtrMayDangle<int> dangling = ptr.ExtractAsDangling();

  if constexpr (std::is_same_v<RawPtrCountingImpl,
                               RawPtrCountingMayDangleImpl>) {
    auto expectations = CountingRawPtrExpectations{
        .wrap_raw_ptr_cnt = 1,
        .release_wrapped_ptr_cnt = 1,
        .get_for_dereference_cnt = 0,
        .wrapped_ptr_swap_cnt = 0,
    };
    EXPECT_THAT((expectations), CountingRawPtrHasCounts());
    EXPECT_THAT((expectations), MayDangleCountingRawPtrHasCounts());
  } else {
    EXPECT_THAT((CountingRawPtrExpectations{
                    .wrap_raw_ptr_cnt = 1,
                    .release_wrapped_ptr_cnt = 1,
                    .get_for_dereference_cnt = 0,
                    .wrapped_ptr_swap_cnt = 0,
                }),
                CountingRawPtrHasCounts());
    EXPECT_THAT((CountingRawPtrExpectations{
                    .wrap_raw_ptr_cnt = 1,
                    .release_wrapped_ptr_cnt = 0,
                    .get_for_dereference_cnt = 0,
                    .wrapped_ptr_swap_cnt = 0,
                }),
                MayDangleCountingRawPtrHasCounts());
  }

  EXPECT_FALSE(ptr.get());
  EXPECT_TRUE(dangling.get());

  dangling.ClearAndDelete();
}

TEST_F(RawPtrTest, ExtractAsDanglingFromDangling) {
  CountingRawPtrMayDangle<int> ptr(new int);

  EXPECT_THAT((CountingRawPtrExpectations{
                  .wrap_raw_ptr_cnt = 1,
                  .release_wrapped_ptr_cnt = 0,
                  .get_for_dereference_cnt = 0,
                  .wrapped_ptr_swap_cnt = 0,
              }),
              MayDangleCountingRawPtrHasCounts());

  CountingRawPtrMayDangle<int> dangling = ptr.ExtractAsDangling();

  // wrap_raw_ptr_cnt remains `1` because, as `ptr` is already a dangling
  // pointer, we are only moving `ptr` to `dangling` here to avoid extra cost.
  EXPECT_THAT((CountingRawPtrExpectations{
                  .wrap_raw_ptr_cnt = 1,
                  .release_wrapped_ptr_cnt = 1,
                  .get_for_dereference_cnt = 0,
                  .wrapped_ptr_swap_cnt = 0,
              }),
              MayDangleCountingRawPtrHasCounts());

  dangling.ClearAndDelete();
}

TEST_F(RawPtrTest, ConstVolatileVoidPtr) {
  int32_t foo[] = {1234567890};
  CountingRawPtr<const volatile void> ptr = foo;
  EXPECT_EQ(*static_cast<const volatile int32_t*>(ptr), 1234567890);
  // Because we're using a cast, the extraction API kicks in, which doesn't
  // know if the extracted pointer will be dereferenced or not.
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 1,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, VoidPtr) {
  int32_t foo[] = {1234567890};
  CountingRawPtr<void> ptr = foo;
  EXPECT_EQ(*static_cast<int32_t*>(ptr), 1234567890);
  // Because we're using a cast, the extraction API kicks in, which doesn't
  // know if the extracted pointer will be dereferenced or not.
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 1,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, OperatorEQ) {
  int foo;
  CountingRawPtr<int> ptr1 = nullptr;
  EXPECT_TRUE(ptr1 == ptr1);

  CountingRawPtr<int> ptr2 = nullptr;
  EXPECT_TRUE(ptr1 == ptr2);

  CountingRawPtr<int> ptr3 = &foo;
  EXPECT_TRUE(&foo == ptr3);
  EXPECT_TRUE(ptr3 == &foo);
  EXPECT_FALSE(ptr1 == ptr3);

  ptr1 = &foo;
  EXPECT_TRUE(ptr1 == ptr3);
  EXPECT_TRUE(ptr3 == ptr1);

  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 12,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, OperatorNE) {
  int foo;
  CountingRawPtr<int> ptr1 = nullptr;
  EXPECT_FALSE(ptr1 != ptr1);

  CountingRawPtr<int> ptr2 = nullptr;
  EXPECT_FALSE(ptr1 != ptr2);

  CountingRawPtr<int> ptr3 = &foo;
  EXPECT_FALSE(&foo != ptr3);
  EXPECT_FALSE(ptr3 != &foo);
  EXPECT_TRUE(ptr1 != ptr3);

  ptr1 = &foo;
  EXPECT_FALSE(ptr1 != ptr3);
  EXPECT_FALSE(ptr3 != ptr1);

  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 12,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, OperatorEQCast) {
  int foo = 42;
  const int* raw_int_ptr = &foo;
  volatile void* raw_void_ptr = &foo;
  CountingRawPtr<volatile int> checked_int_ptr = &foo;
  CountingRawPtr<const void> checked_void_ptr = &foo;
  EXPECT_TRUE(checked_int_ptr == checked_int_ptr);
  EXPECT_TRUE(checked_int_ptr == raw_int_ptr);
  EXPECT_TRUE(raw_int_ptr == checked_int_ptr);
  EXPECT_TRUE(checked_void_ptr == checked_void_ptr);
  EXPECT_TRUE(checked_void_ptr == raw_void_ptr);
  EXPECT_TRUE(raw_void_ptr == checked_void_ptr);
  EXPECT_TRUE(checked_int_ptr == checked_void_ptr);
  EXPECT_TRUE(checked_int_ptr == raw_void_ptr);
  EXPECT_TRUE(raw_int_ptr == checked_void_ptr);
  EXPECT_TRUE(checked_void_ptr == checked_int_ptr);
  EXPECT_TRUE(checked_void_ptr == raw_int_ptr);
  EXPECT_TRUE(raw_void_ptr == checked_int_ptr);
  // Make sure that all cases are handled by operator== (faster) and none by the
  // cast operator (slower).
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 16,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, OperatorEQCastHierarchy) {
  Derived derived_val(42, 84, 1024);
  Derived* raw_derived_ptr = &derived_val;
  const Base1* raw_base1_ptr = &derived_val;
  volatile Base2* raw_base2_ptr = &derived_val;
  // Double check the basic understanding of pointers: Even though the numeric
  // value (i.e. the address) isn't equal, the pointers are still equal. That's
  // because from derived to base adjusts the address.
  // raw_ptr must behave the same, which is checked below.
  ASSERT_NE(reinterpret_cast<uintptr_t>(raw_base2_ptr),
            reinterpret_cast<uintptr_t>(raw_derived_ptr));
  ASSERT_TRUE(raw_base2_ptr == raw_derived_ptr);

  CountingRawPtr<const volatile Derived> checked_derived_ptr = &derived_val;
  CountingRawPtr<volatile Base1> checked_base1_ptr = &derived_val;
  CountingRawPtr<const Base2> checked_base2_ptr = &derived_val;
  EXPECT_TRUE(checked_derived_ptr == checked_derived_ptr);
  EXPECT_TRUE(checked_derived_ptr == raw_derived_ptr);
  EXPECT_TRUE(raw_derived_ptr == checked_derived_ptr);
  EXPECT_TRUE(checked_derived_ptr == checked_base1_ptr);
  EXPECT_TRUE(checked_derived_ptr == raw_base1_ptr);
  EXPECT_TRUE(raw_derived_ptr == checked_base1_ptr);
  EXPECT_TRUE(checked_base1_ptr == checked_derived_ptr);
  EXPECT_TRUE(checked_base1_ptr == raw_derived_ptr);
  EXPECT_TRUE(raw_base1_ptr == checked_derived_ptr);
  // |base2_ptr| points to the second base class of |derived|, so will be
  // located at an offset. While the stored raw uinptr_t values shouldn't match,
  // ensure that the internal pointer manipulation correctly offsets when
  // casting up and down the class hierarchy.
  EXPECT_NE(reinterpret_cast<uintptr_t>(checked_base2_ptr.get()),
            reinterpret_cast<uintptr_t>(checked_derived_ptr.get()));
  EXPECT_NE(reinterpret_cast<uintptr_t>(raw_base2_ptr),
            reinterpret_cast<uintptr_t>(checked_derived_ptr.get()));
  EXPECT_NE(reinterpret_cast<uintptr_t>(checked_base2_ptr.get()),
            reinterpret_cast<uintptr_t>(raw_derived_ptr));
  EXPECT_TRUE(checked_derived_ptr == checked_base2_ptr);
  EXPECT_TRUE(checked_derived_ptr == raw_base2_ptr);
  EXPECT_TRUE(raw_derived_ptr == checked_base2_ptr);
  EXPECT_TRUE(checked_base2_ptr == checked_derived_ptr);
  EXPECT_TRUE(checked_base2_ptr == raw_derived_ptr);
  EXPECT_TRUE(raw_base2_ptr == checked_derived_ptr);
  // Make sure that all cases are handled by operator== (faster) and none by the
  // cast operator (slower).
  // The 4 extractions come from .get() checks, that compare raw addresses.
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 4,
                  .get_for_comparison_cnt = 20,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, OperatorNECast) {
  int foo = 42;
  volatile int* raw_int_ptr = &foo;
  const void* raw_void_ptr = &foo;
  CountingRawPtr<const int> checked_int_ptr = &foo;
  CountingRawPtr<volatile void> checked_void_ptr = &foo;
  EXPECT_FALSE(checked_int_ptr != checked_int_ptr);
  EXPECT_FALSE(checked_int_ptr != raw_int_ptr);
  EXPECT_FALSE(raw_int_ptr != checked_int_ptr);
  EXPECT_FALSE(checked_void_ptr != checked_void_ptr);
  EXPECT_FALSE(checked_void_ptr != raw_void_ptr);
  EXPECT_FALSE(raw_void_ptr != checked_void_ptr);
  EXPECT_FALSE(checked_int_ptr != checked_void_ptr);
  EXPECT_FALSE(checked_int_ptr != raw_void_ptr);
  EXPECT_FALSE(raw_int_ptr != checked_void_ptr);
  EXPECT_FALSE(checked_void_ptr != checked_int_ptr);
  EXPECT_FALSE(checked_void_ptr != raw_int_ptr);
  EXPECT_FALSE(raw_void_ptr != checked_int_ptr);
  // Make sure that all cases are handled by operator== (faster) and none by the
  // cast operator (slower).
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 16,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, OperatorNECastHierarchy) {
  Derived derived_val(42, 84, 1024);
  const Derived* raw_derived_ptr = &derived_val;
  volatile Base1* raw_base1_ptr = &derived_val;
  const Base2* raw_base2_ptr = &derived_val;
  CountingRawPtr<volatile Derived> checked_derived_ptr = &derived_val;
  CountingRawPtr<const Base1> checked_base1_ptr = &derived_val;
  CountingRawPtr<const volatile Base2> checked_base2_ptr = &derived_val;
  EXPECT_FALSE(checked_derived_ptr != checked_derived_ptr);
  EXPECT_FALSE(checked_derived_ptr != raw_derived_ptr);
  EXPECT_FALSE(raw_derived_ptr != checked_derived_ptr);
  EXPECT_FALSE(checked_derived_ptr != checked_base1_ptr);
  EXPECT_FALSE(checked_derived_ptr != raw_base1_ptr);
  EXPECT_FALSE(raw_derived_ptr != checked_base1_ptr);
  EXPECT_FALSE(checked_base1_ptr != checked_derived_ptr);
  EXPECT_FALSE(checked_base1_ptr != raw_derived_ptr);
  EXPECT_FALSE(raw_base1_ptr != checked_derived_ptr);
  // |base2_ptr| points to the second base class of |derived|, so will be
  // located at an offset. While the stored raw uinptr_t values shouldn't match,
  // ensure that the internal pointer manipulation correctly offsets when
  // casting up and down the class hierarchy.
  EXPECT_NE(reinterpret_cast<uintptr_t>(checked_base2_ptr.get()),
            reinterpret_cast<uintptr_t>(checked_derived_ptr.get()));
  EXPECT_NE(reinterpret_cast<uintptr_t>(raw_base2_ptr),
            reinterpret_cast<uintptr_t>(checked_derived_ptr.get()));
  EXPECT_NE(reinterpret_cast<uintptr_t>(checked_base2_ptr.get()),
            reinterpret_cast<uintptr_t>(raw_derived_ptr));
  EXPECT_FALSE(checked_derived_ptr != checked_base2_ptr);
  EXPECT_FALSE(checked_derived_ptr != raw_base2_ptr);
  EXPECT_FALSE(raw_derived_ptr != checked_base2_ptr);
  EXPECT_FALSE(checked_base2_ptr != checked_derived_ptr);
  EXPECT_FALSE(checked_base2_ptr != raw_derived_ptr);
  EXPECT_FALSE(raw_base2_ptr != checked_derived_ptr);
  // Make sure that all cases are handled by operator== (faster) and none by the
  // cast operator (slower).
  // The 4 extractions come from .get() checks, that compare raw addresses.
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 4,
                  .get_for_comparison_cnt = 20,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, Cast) {
  Derived derived_val(42, 84, 1024);
  raw_ptr<Derived> checked_derived_ptr = &derived_val;
  Base1* raw_base1_ptr = checked_derived_ptr;
  EXPECT_EQ(raw_base1_ptr->b1, 42);
  Base2* raw_base2_ptr = checked_derived_ptr;
  EXPECT_EQ(raw_base2_ptr->b2, 84);

  Derived* raw_derived_ptr = static_cast<Derived*>(raw_base1_ptr);
  EXPECT_EQ(raw_derived_ptr->b1, 42);
  EXPECT_EQ(raw_derived_ptr->b2, 84);
  EXPECT_EQ(raw_derived_ptr->d, 1024);
  raw_derived_ptr = static_cast<Derived*>(raw_base2_ptr);
  EXPECT_EQ(raw_derived_ptr->b1, 42);
  EXPECT_EQ(raw_derived_ptr->b2, 84);
  EXPECT_EQ(raw_derived_ptr->d, 1024);

  raw_ptr<Base1> checked_base1_ptr = raw_derived_ptr;
  EXPECT_EQ(checked_base1_ptr->b1, 42);
  raw_ptr<Base2> checked_base2_ptr = raw_derived_ptr;
  EXPECT_EQ(checked_base2_ptr->b2, 84);

  raw_ptr<Derived> checked_derived_ptr2 =
      static_cast<Derived*>(checked_base1_ptr);
  EXPECT_EQ(checked_derived_ptr2->b1, 42);
  EXPECT_EQ(checked_derived_ptr2->b2, 84);
  EXPECT_EQ(checked_derived_ptr2->d, 1024);
  checked_derived_ptr2 = static_cast<Derived*>(checked_base2_ptr);
  EXPECT_EQ(checked_derived_ptr2->b1, 42);
  EXPECT_EQ(checked_derived_ptr2->b2, 84);
  EXPECT_EQ(checked_derived_ptr2->d, 1024);

  const Derived* raw_const_derived_ptr = checked_derived_ptr2;
  EXPECT_EQ(raw_const_derived_ptr->b1, 42);
  EXPECT_EQ(raw_const_derived_ptr->b2, 84);
  EXPECT_EQ(raw_const_derived_ptr->d, 1024);

  raw_ptr<const Derived> checked_const_derived_ptr = raw_const_derived_ptr;
  EXPECT_EQ(checked_const_derived_ptr->b1, 42);
  EXPECT_EQ(checked_const_derived_ptr->b2, 84);
  EXPECT_EQ(checked_const_derived_ptr->d, 1024);

  const Derived* raw_const_derived_ptr2 = checked_const_derived_ptr;
  EXPECT_EQ(raw_const_derived_ptr2->b1, 42);
  EXPECT_EQ(raw_const_derived_ptr2->b2, 84);
  EXPECT_EQ(raw_const_derived_ptr2->d, 1024);

  raw_ptr<const Derived> checked_const_derived_ptr2 = raw_derived_ptr;
  EXPECT_EQ(checked_const_derived_ptr2->b1, 42);
  EXPECT_EQ(checked_const_derived_ptr2->b2, 84);
  EXPECT_EQ(checked_const_derived_ptr2->d, 1024);

  raw_ptr<const Derived> checked_const_derived_ptr3 = checked_derived_ptr2;
  EXPECT_EQ(checked_const_derived_ptr3->b1, 42);
  EXPECT_EQ(checked_const_derived_ptr3->b2, 84);
  EXPECT_EQ(checked_const_derived_ptr3->d, 1024);

  volatile Derived* raw_volatile_derived_ptr = checked_derived_ptr2;
  EXPECT_EQ(raw_volatile_derived_ptr->b1, 42);
  EXPECT_EQ(raw_volatile_derived_ptr->b2, 84);
  EXPECT_EQ(raw_volatile_derived_ptr->d, 1024);

  raw_ptr<volatile Derived> checked_volatile_derived_ptr =
      raw_volatile_derived_ptr;
  EXPECT_EQ(checked_volatile_derived_ptr->b1, 42);
  EXPECT_EQ(checked_volatile_derived_ptr->b2, 84);
  EXPECT_EQ(checked_volatile_derived_ptr->d, 1024);

  void* raw_void_ptr = checked_derived_ptr;
  raw_ptr<void> checked_void_ptr = raw_derived_ptr;
  raw_ptr<Derived> checked_derived_ptr3 = static_cast<Derived*>(raw_void_ptr);
  raw_ptr<Derived> checked_derived_ptr4 =
      static_cast<Derived*>(checked_void_ptr);
  EXPECT_EQ(checked_derived_ptr3->b1, 42);
  EXPECT_EQ(checked_derived_ptr3->b2, 84);
  EXPECT_EQ(checked_derived_ptr3->d, 1024);
  EXPECT_EQ(checked_derived_ptr4->b1, 42);
  EXPECT_EQ(checked_derived_ptr4->b2, 84);
  EXPECT_EQ(checked_derived_ptr4->d, 1024);
}

TEST_F(RawPtrTest, UpcastConvertible) {
  {
    Derived derived_val(42, 84, 1024);
    raw_ptr<Derived> checked_derived_ptr = &derived_val;

    raw_ptr<Base1> checked_base1_ptr(checked_derived_ptr);
    EXPECT_EQ(checked_base1_ptr->b1, 42);
    raw_ptr<Base2> checked_base2_ptr(checked_derived_ptr);
    EXPECT_EQ(checked_base2_ptr->b2, 84);

    checked_base1_ptr = checked_derived_ptr;
    EXPECT_EQ(checked_base1_ptr->b1, 42);
    checked_base2_ptr = checked_derived_ptr;
    EXPECT_EQ(checked_base2_ptr->b2, 84);

    EXPECT_EQ(checked_base1_ptr, checked_derived_ptr);
    EXPECT_EQ(checked_base2_ptr, checked_derived_ptr);
  }

  {
    Derived derived_val(42, 84, 1024);
    raw_ptr<Derived> checked_derived_ptr1 = &derived_val;
    raw_ptr<Derived> checked_derived_ptr2 = &derived_val;
    raw_ptr<Derived> checked_derived_ptr3 = &derived_val;
    raw_ptr<Derived> checked_derived_ptr4 = &derived_val;

    raw_ptr<Base1> checked_base1_ptr(std::move(checked_derived_ptr1));
    EXPECT_EQ(checked_base1_ptr->b1, 42);
    raw_ptr<Base2> checked_base2_ptr(std::move(checked_derived_ptr2));
    EXPECT_EQ(checked_base2_ptr->b2, 84);

    checked_base1_ptr = std::move(checked_derived_ptr3);
    EXPECT_EQ(checked_base1_ptr->b1, 42);
    checked_base2_ptr = std::move(checked_derived_ptr4);
    EXPECT_EQ(checked_base2_ptr->b2, 84);
  }
}

TEST_F(RawPtrTest, UpcastNotConvertible) {
  class Base {};
  class Derived : private Base {};
  class Unrelated {};
  EXPECT_FALSE((std::is_convertible<raw_ptr<Derived>, raw_ptr<Base>>::value));
  EXPECT_FALSE((std::is_convertible<raw_ptr<Unrelated>, raw_ptr<Base>>::value));
  EXPECT_FALSE((std::is_convertible<raw_ptr<Unrelated>, raw_ptr<void>>::value));
  EXPECT_FALSE((std::is_convertible<raw_ptr<void>, raw_ptr<Unrelated>>::value));
  EXPECT_FALSE(
      (std::is_convertible<raw_ptr<int64_t>, raw_ptr<int32_t>>::value));
  EXPECT_FALSE(
      (std::is_convertible<raw_ptr<int16_t>, raw_ptr<int32_t>>::value));
}

TEST_F(RawPtrTest, UpcastPerformance) {
  {
    Derived derived_val(42, 84, 1024);
    CountingRawPtr<Derived> checked_derived_ptr = &derived_val;
    CountingRawPtr<Base1> checked_base1_ptr(checked_derived_ptr);
    CountingRawPtr<Base2> checked_base2_ptr(checked_derived_ptr);
    checked_base1_ptr = checked_derived_ptr;
    checked_base2_ptr = checked_derived_ptr;
  }

  {
    Derived derived_val(42, 84, 1024);
    CountingRawPtr<Derived> checked_derived_ptr = &derived_val;
    CountingRawPtr<Base1> checked_base1_ptr(std::move(checked_derived_ptr));
    CountingRawPtr<Base2> checked_base2_ptr(std::move(checked_derived_ptr));
    checked_base1_ptr = std::move(checked_derived_ptr);
    checked_base2_ptr = std::move(checked_derived_ptr);
  }

  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, CustomSwap) {
  int foo1, foo2;
  CountingRawPtr<int> ptr1(&foo1);
  CountingRawPtr<int> ptr2(&foo2);
  // Recommended use pattern.
  using std::swap;
  swap(ptr1, ptr2);
  EXPECT_EQ(ptr1.get(), &foo2);
  EXPECT_EQ(ptr2.get(), &foo1);
  EXPECT_EQ(RawPtrCountingImpl::wrapped_ptr_swap_cnt, 1);
}

TEST_F(RawPtrTest, StdSwap) {
  int foo1, foo2;
  CountingRawPtr<int> ptr1(&foo1);
  CountingRawPtr<int> ptr2(&foo2);
  std::swap(ptr1, ptr2);
  EXPECT_EQ(ptr1.get(), &foo2);
  EXPECT_EQ(ptr2.get(), &foo1);
  EXPECT_EQ(RawPtrCountingImpl::wrapped_ptr_swap_cnt, 0);
}

TEST_F(RawPtrTest, PostIncrementOperator) {
  int foo[] = {42, 43, 44, 45};
  CountingRawPtr<int> ptr = foo;
  for (int i = 0; i < 4; ++i) {
    ASSERT_EQ(*ptr++, 42 + i);
  }
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 4,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, PostDecrementOperator) {
  int foo[] = {42, 43, 44, 45};
  CountingRawPtr<int> ptr = &foo[3];
  for (int i = 3; i >= 0; --i) {
    ASSERT_EQ(*ptr--, 42 + i);
  }
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 4,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, PreIncrementOperator) {
  int foo[] = {42, 43, 44, 45};
  CountingRawPtr<int> ptr = foo;
  for (int i = 0; i < 4; ++i, ++ptr) {
    ASSERT_EQ(*ptr, 42 + i);
  }
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 4,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, PreDecrementOperator) {
  int foo[] = {42, 43, 44, 45};
  CountingRawPtr<int> ptr = &foo[3];
  for (int i = 3; i >= 0; --i, --ptr) {
    ASSERT_EQ(*ptr, 42 + i);
  }
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 4,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, PlusEqualOperator) {
  int foo[] = {42, 43, 44, 45};
  CountingRawPtr<int> ptr = foo;
  for (int i = 0; i < 4; i += 2, ptr += 2) {
    ASSERT_EQ(*ptr, 42 + i);
  }
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 2,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, PlusEqualOperatorTypes) {
  int foo[] = {42, 43, 44, 45};
  CountingRawPtr<int> ptr = foo;
  ASSERT_EQ(*ptr, 42);
  ptr += 2;  // Positive literal.
  ASSERT_EQ(*ptr, 44);
  ptr -= 2;  // Negative literal.
  ASSERT_EQ(*ptr, 42);
  ptr += ptrdiff_t{1};  // ptrdiff_t.
  ASSERT_EQ(*ptr, 43);
  ptr += size_t{2};  // size_t.
  ASSERT_EQ(*ptr, 45);
}

TEST_F(RawPtrTest, MinusEqualOperator) {
  int foo[] = {42, 43, 44, 45};
  CountingRawPtr<int> ptr = &foo[3];
  for (int i = 3; i >= 0; i -= 2, ptr -= 2) {
    ASSERT_EQ(*ptr, 42 + i);
  }
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 2,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, MinusEqualOperatorTypes) {
  int foo[] = {42, 43, 44, 45};
  CountingRawPtr<int> ptr = &foo[3];
  ASSERT_EQ(*ptr, 45);
  ptr -= 2;  // Positive literal.
  ASSERT_EQ(*ptr, 43);
  ptr -= -2;  // Negative literal.
  ASSERT_EQ(*ptr, 45);
  ptr -= ptrdiff_t{2};  // ptrdiff_t.
  ASSERT_EQ(*ptr, 43);
  ptr -= size_t{1};  // size_t.
  ASSERT_EQ(*ptr, 42);
}

TEST_F(RawPtrTest, PlusOperator) {
  int foo[] = {42, 43, 44, 45};
  CountingRawPtr<int> ptr = foo;
  for (int i = 0; i < 4; ++i) {
    ASSERT_EQ(*(ptr + i), 42 + i);
  }
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 4,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, MinusOperator) {
  int foo[] = {42, 43, 44, 45};
  CountingRawPtr<int> ptr = &foo[4];
  for (int i = 1; i <= 4; ++i) {
    ASSERT_EQ(*(ptr - i), 46 - i);
  }
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 4,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, MinusDeltaOperator) {
  int foo[] = {42, 43, 44, 45};
  CountingRawPtr<int> ptrs[] = {&foo[0], &foo[1], &foo[2], &foo[3], &foo[4]};
  for (int i = 0; i <= 4; ++i) {
    for (int j = 0; j <= 4; ++j) {
      ASSERT_EQ(ptrs[i] - ptrs[j], i - j);
      ASSERT_EQ(ptrs[i] - &foo[j], i - j);
      ASSERT_EQ(&foo[i] - ptrs[j], i - j);
    }
  }
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, AdvanceString) {
  const char kChars[] = "Hello";
  std::string str = kChars;
  CountingRawPtr<const char> ptr = str.c_str();
  for (size_t i = 0; i < str.size(); ++i, ++ptr) {
    ASSERT_EQ(*ptr, kChars[i]);
  }
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 5,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, AssignmentFromNullptr) {
  CountingRawPtr<int> wrapped_ptr;
  wrapped_ptr = nullptr;
  EXPECT_THAT((CountingRawPtrExpectations{
                  .wrap_raw_ptr_cnt = 0,
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

void FunctionWithRawPtrParameter(raw_ptr<int> actual_ptr, int* expected_ptr) {
  EXPECT_EQ(actual_ptr.get(), expected_ptr);
  EXPECT_EQ(*actual_ptr, *expected_ptr);
}

// This test checks that raw_ptr<T> can be passed by value into function
// parameters.  This is mostly a smoke test for TRIVIAL_ABI attribute.
TEST_F(RawPtrTest, FunctionParameters_ImplicitlyMovedTemporary) {
  int x = 123;
  FunctionWithRawPtrParameter(
      raw_ptr<int>(&x),  // Temporary that will be moved into the function.
      &x);
}

// This test checks that raw_ptr<T> can be passed by value into function
// parameters.  This is mostly a smoke test for TRIVIAL_ABI attribute.
TEST_F(RawPtrTest, FunctionParameters_ExplicitlyMovedLValue) {
  int x = 123;
  raw_ptr<int> ptr(&x);
  FunctionWithRawPtrParameter(std::move(ptr), &x);
}

// This test checks that raw_ptr<T> can be passed by value into function
// parameters.  This is mostly a smoke test for TRIVIAL_ABI attribute.
TEST_F(RawPtrTest, FunctionParameters_Copy) {
  int x = 123;
  raw_ptr<int> ptr(&x);
  FunctionWithRawPtrParameter(ptr,  // `ptr` will be copied into the function.
                              &x);
}

TEST_F(RawPtrTest, SetLookupUsesGetForComparison) {
  std::set<CountingRawPtr<int>> set;
  int x = 123;
  CountingRawPtr<int> ptr(&x);

  RawPtrCountingImpl::ClearCounters();
  set.emplace(&x);
  EXPECT_THAT((CountingRawPtrExpectations{
                  .wrap_raw_ptr_cnt = 1,
                  // Nothing to compare to yet.
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 0,
                  .wrapped_ptr_less_cnt = 0,
              }),
              CountingRawPtrHasCounts());

  RawPtrCountingImpl::ClearCounters();
  set.emplace(ptr);
  EXPECT_THAT((CountingRawPtrExpectations{
                  .wrap_raw_ptr_cnt = 0,
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 0,
                  // 2 items to compare to => 4 calls.
                  .get_for_comparison_cnt = 4,
                  // 1 element to compare to => 2 calls.
                  .wrapped_ptr_less_cnt = 2,
              }),
              CountingRawPtrHasCounts());

  RawPtrCountingImpl::ClearCounters();
  set.count(&x);
  EXPECT_THAT((CountingRawPtrExpectations{
                  .wrap_raw_ptr_cnt = 0,
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 0,
                  // 2 comparisons => 2 extractions. Less than before, because
                  // this time a raw pointer is one side of the comparison.
                  .get_for_comparison_cnt = 2,
                  // 2 items to compare to => 4 calls.
                  .wrapped_ptr_less_cnt = 2,
              }),
              CountingRawPtrHasCounts());

  RawPtrCountingImpl::ClearCounters();
  set.count(ptr);
  EXPECT_THAT((CountingRawPtrExpectations{
                  .wrap_raw_ptr_cnt = 0,
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 0,
                  // 2 comparisons => 4 extractions.
                  .get_for_comparison_cnt = 4,
                  // 2 items to compare to => 4 calls.
                  .wrapped_ptr_less_cnt = 2,
              }),
              CountingRawPtrHasCounts());
}

TEST_F(RawPtrTest, ComparisonOperatorUsesGetForComparison) {
  int x = 123;
  CountingRawPtr<int> ptr(&x);

  RawPtrCountingImpl::ClearCounters();
  EXPECT_FALSE(ptr < ptr);
  EXPECT_FALSE(ptr > ptr);
  EXPECT_TRUE(ptr <= ptr);
  EXPECT_TRUE(ptr >= ptr);
  EXPECT_THAT((CountingRawPtrExpectations{
                  .wrap_raw_ptr_cnt = 0,
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 8,
                  // < is used directly, not std::less().
                  .wrapped_ptr_less_cnt = 0,
              }),
              CountingRawPtrHasCounts());

  RawPtrCountingImpl::ClearCounters();
  EXPECT_FALSE(ptr < &x);
  EXPECT_FALSE(ptr > &x);
  EXPECT_TRUE(ptr <= &x);
  EXPECT_TRUE(ptr >= &x);
  EXPECT_THAT((CountingRawPtrExpectations{
                  .wrap_raw_ptr_cnt = 0,
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 4,
                  .wrapped_ptr_less_cnt = 0,
              }),
              CountingRawPtrHasCounts());

  RawPtrCountingImpl::ClearCounters();
  EXPECT_FALSE(&x < ptr);
  EXPECT_FALSE(&x > ptr);
  EXPECT_TRUE(&x <= ptr);
  EXPECT_TRUE(&x >= ptr);
  EXPECT_THAT((CountingRawPtrExpectations{
                  .wrap_raw_ptr_cnt = 0,
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 0,
                  .get_for_comparison_cnt = 4,
                  .wrapped_ptr_less_cnt = 0,
              }),
              CountingRawPtrHasCounts());
}

// This test checks how the std library handles collections like
// std::vector<raw_ptr<T>>.
//
// When this test is written, reallocating std::vector's storage (e.g.
// when growing the vector) requires calling raw_ptr's destructor on the
// old storage (after std::move-ing the data to the new storage).  In
// the future we hope that TRIVIAL_ABI (or [trivially_relocatable]]
// proposed by P1144 [1]) will allow memcpy-ing the elements into the
// new storage (without invoking destructors and move constructors
// and/or move assignment operators).  At that point, the assert in the
// test should be modified to capture the new, better behavior.
//
// In the meantime, this test serves as a basic correctness test that
// ensures that raw_ptr<T> stored in a std::vector passes basic smoke
// tests.
//
// [1]
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p1144r5.html#wording-attribute
TEST_F(RawPtrTest, TrivialRelocability) {
  std::vector<CountingRawPtr<int>> vector;
  int x = 123;

  // See how many times raw_ptr's destructor is called when std::vector
  // needs to increase its capacity and reallocate the internal vector
  // storage (moving the raw_ptr elements).
  RawPtrCountingImpl::ClearCounters();
  size_t number_of_capacity_changes = 0;
  do {
    size_t previous_capacity = vector.capacity();
    while (vector.capacity() == previous_capacity)
      vector.emplace_back(&x);
    number_of_capacity_changes++;
  } while (number_of_capacity_changes < 10);
#if BUILDFLAG(USE_BACKUP_REF_PTR)
  // TODO(lukasza): In the future (once C++ language and std library
  // support custom trivially relocatable objects) this #if branch can
  // be removed (keeping only the right long-term expectation from the
  // #else branch).
  EXPECT_NE(0, RawPtrCountingImpl::release_wrapped_ptr_cnt);
#else
  // This is the right long-term expectation.
  //
  // (This EXPECT_EQ assertion is slightly misleading in
  // !USE_BACKUP_REF_PTR mode, because RawPtrNoOpImpl has a default
  // destructor that doesn't go through
  // RawPtrCountingImpl::ReleaseWrappedPtr.  Nevertheless, the spirit of
  // the EXPECT_EQ is correct + the assertion should be true in the
  // long-term.)
  EXPECT_EQ(0, RawPtrCountingImpl::release_wrapped_ptr_cnt);
#endif

  // Basic smoke test that raw_ptr elements in a vector work okay.
  for (const auto& elem : vector) {
    EXPECT_EQ(elem.get(), &x);
    EXPECT_EQ(*elem, x);
  }

  // Verification that release_wrapped_ptr_cnt does capture how many times the
  // destructors are called (e.g. that it is not always zero).
  RawPtrCountingImpl::ClearCounters();
  size_t number_of_cleared_elements = vector.size();
  vector.clear();
#if BUILDFLAG(USE_BACKUP_REF_PTR)
  EXPECT_EQ((int)number_of_cleared_elements,
            RawPtrCountingImpl::release_wrapped_ptr_cnt);
#else
  // TODO(lukasza): !USE_BACKUP_REF_PTR / RawPtrNoOpImpl has a default
  // destructor that doesn't go through
  // RawPtrCountingImpl::ReleaseWrappedPtr.  So we can't really depend
  // on `g_release_wrapped_ptr_cnt`.  This #else branch should be
  // deleted once USE_BACKUP_REF_PTR is removed (e.g. once BackupRefPtr
  // ships to the Stable channel).
  EXPECT_EQ(0, RawPtrCountingImpl::release_wrapped_ptr_cnt);
  std::ignore = number_of_cleared_elements;
#endif
}

struct BaseStruct {
  explicit BaseStruct(int a) : a(a) {}
  virtual ~BaseStruct() = default;

  int a;
};

struct DerivedType1 : public BaseStruct {
  explicit DerivedType1(int a, int b) : BaseStruct(a), b(b) {}
  int b;
};

struct DerivedType2 : public BaseStruct {
  explicit DerivedType2(int a, int c) : BaseStruct(a), c(c) {}
  int c;
};

TEST_F(RawPtrTest, DerivedStructsComparison) {
  DerivedType1 derived_1(42, 84);
  raw_ptr<DerivedType1> checked_derived1_ptr = &derived_1;
  DerivedType2 derived_2(21, 10);
  raw_ptr<DerivedType2> checked_derived2_ptr = &derived_2;

  // Make sure that comparing a |DerivedType2*| to a |DerivedType1*| casted
  // as a |BaseStruct*| doesn't cause CFI errors.
  EXPECT_NE(checked_derived1_ptr,
            static_cast<BaseStruct*>(checked_derived2_ptr.get()));
  EXPECT_NE(static_cast<BaseStruct*>(checked_derived1_ptr.get()),
            checked_derived2_ptr);
}

#if BUILDFLAG(ENABLE_BASE_TRACING) && BUILDFLAG(PA_USE_BASE_TRACING)
TEST_F(RawPtrTest, TracedValueSupport) {
  // Serialise nullptr.
  EXPECT_EQ(perfetto::TracedValueToString(raw_ptr<int>()), "0x0");

  {
    // If the pointer is non-null, its dereferenced value will be serialised.
    int value = 42;
    EXPECT_EQ(perfetto::TracedValueToString(raw_ptr<int>(&value)), "42");
  }

  struct WithTraceSupport {
    void WriteIntoTrace(perfetto::TracedValue ctx) const {
      std::move(ctx).WriteString("result");
    }
  };

  {
    WithTraceSupport value;
    EXPECT_EQ(perfetto::TracedValueToString(raw_ptr<WithTraceSupport>(&value)),
              "result");
  }
}
#endif  // BUILDFLAG(ENABLE_BASE_TRACING) && BUILDFLAG(PA_USE_BASE_TRACING)

class PmfTestBase {
 public:
  int MemFunc(char, double) const { return 11; }
};

class PmfTestDerived : public PmfTestBase {
 public:
  using PmfTestBase::MemFunc;
  int MemFunc(float, double) { return 22; }
};

}  // namespace

namespace base {
namespace internal {

#if BUILDFLAG(USE_BACKUP_REF_PTR) && !defined(MEMORY_TOOL_REPLACES_ALLOCATOR)

void HandleOOM(size_t unused_size) {
  LOG(FATAL) << "Out of memory";
}

static constexpr partition_alloc::PartitionOptions kOpts = {
    partition_alloc::PartitionOptions::AlignedAlloc::kDisallowed,
    partition_alloc::PartitionOptions::ThreadCache::kDisabled,
    partition_alloc::PartitionOptions::Quarantine::kDisallowed,
    partition_alloc::PartitionOptions::Cookie::kAllowed,
    partition_alloc::PartitionOptions::BackupRefPtr::kEnabled,
    partition_alloc::PartitionOptions::BackupRefPtrZapping::kEnabled,
    partition_alloc::PartitionOptions::UseConfigurablePool::kNo,
};

class BackupRefPtrTest : public testing::Test {
 protected:
  void SetUp() override {
    // TODO(bartekn): Avoid using PartitionAlloc API directly. Switch to
    // new/delete once PartitionAlloc Everywhere is fully enabled.
    partition_alloc::PartitionAllocGlobalInit(HandleOOM);
    allocator_.init(kOpts);
  }

  partition_alloc::PartitionAllocator allocator_;
};

TEST_F(BackupRefPtrTest, Basic) {
  base::CPU cpu;

  int* raw_ptr1 =
      reinterpret_cast<int*>(allocator_.root()->Alloc(sizeof(int), ""));
  // Use the actual raw_ptr implementation, not a test substitute, to
  // exercise real PartitionAlloc paths.
  raw_ptr<int> wrapped_ptr1 = raw_ptr1;

  *raw_ptr1 = 42;
  EXPECT_EQ(*raw_ptr1, *wrapped_ptr1);

  allocator_.root()->Free(raw_ptr1);
#if DCHECK_IS_ON() || BUILDFLAG(ENABLE_BACKUP_REF_PTR_SLOW_CHECKS)
  // In debug builds, the use-after-free should be caught immediately.
  EXPECT_DEATH_IF_SUPPORTED(g_volatile_int_to_ignore = *wrapped_ptr1, "");
#else   // DCHECK_IS_ON() || BUILDFLAG(ENABLE_BACKUP_REF_PTR_SLOW_CHECKS)
  if (cpu.has_mte()) {
    // If the hardware supports MTE, the use-after-free should also be caught.
    EXPECT_DEATH_IF_SUPPORTED(g_volatile_int_to_ignore = *wrapped_ptr1, "");
  } else {
    // The allocation should be poisoned since there's a raw_ptr alive.
    EXPECT_NE(*wrapped_ptr1, 42);
  }

  // The allocator should not be able to reuse the slot at this point.
  void* raw_ptr2 = allocator_.root()->Alloc(sizeof(int), "");
  EXPECT_NE(partition_alloc::UntagPtr(raw_ptr1),
            partition_alloc::UntagPtr(raw_ptr2));
  allocator_.root()->Free(raw_ptr2);

  // When the last reference is released, the slot should become reusable.
  wrapped_ptr1 = nullptr;
  void* raw_ptr3 = allocator_.root()->Alloc(sizeof(int), "");
  EXPECT_EQ(partition_alloc::UntagPtr(raw_ptr1),
            partition_alloc::UntagPtr(raw_ptr3));
  allocator_.root()->Free(raw_ptr3);
#endif  // DCHECK_IS_ON() || BUILDFLAG(ENABLE_BACKUP_REF_PTR_SLOW_CHECKS)
}

TEST_F(BackupRefPtrTest, ZeroSized) {
  std::vector<raw_ptr<void>> ptrs;
  // Use a reasonable number of elements to fill up the slot span.
  for (int i = 0; i < 128 * 1024; ++i) {
    // Constructing a raw_ptr instance from a zero-sized allocation should
    // not result in a crash.
    ptrs.emplace_back(allocator_.root()->Alloc(0, ""));
  }
}

TEST_F(BackupRefPtrTest, EndPointer) {
  // This test requires a fresh partition with an empty free list.
  // Check multiple size buckets and levels of slot filling.
  for (int size = 0; size < 1024; size += sizeof(void*)) {
    // Creating a raw_ptr from an address right past the end of an allocation
    // should not result in a crash or corrupt the free list.
    char* raw_ptr1 =
        reinterpret_cast<char*>(allocator_.root()->Alloc(size, ""));
    raw_ptr<char> wrapped_ptr = raw_ptr1 + size;
    wrapped_ptr = nullptr;
    // We need to make two more allocations to turn the possible free list
    // corruption into an observable crash.
    char* raw_ptr2 =
        reinterpret_cast<char*>(allocator_.root()->Alloc(size, ""));
    char* raw_ptr3 =
        reinterpret_cast<char*>(allocator_.root()->Alloc(size, ""));

    // Similarly for operator+=.
    char* raw_ptr4 =
        reinterpret_cast<char*>(allocator_.root()->Alloc(size, ""));
    wrapped_ptr = raw_ptr4;
    wrapped_ptr += size;
    wrapped_ptr = nullptr;
    char* raw_ptr5 =
        reinterpret_cast<char*>(allocator_.root()->Alloc(size, ""));
    char* raw_ptr6 =
        reinterpret_cast<char*>(allocator_.root()->Alloc(size, ""));

    allocator_.root()->Free(raw_ptr1);
    allocator_.root()->Free(raw_ptr2);
    allocator_.root()->Free(raw_ptr3);
    allocator_.root()->Free(raw_ptr4);
    allocator_.root()->Free(raw_ptr5);
    allocator_.root()->Free(raw_ptr6);
  }
}

TEST_F(BackupRefPtrTest, QuarantinedBytes) {
  uint64_t* raw_ptr1 = reinterpret_cast<uint64_t*>(
      allocator_.root()->Alloc(sizeof(uint64_t), ""));
  raw_ptr<uint64_t> wrapped_ptr1 = raw_ptr1;
  EXPECT_EQ(allocator_.root()->total_size_of_brp_quarantined_bytes.load(
                std::memory_order_relaxed),
            0U);
  EXPECT_EQ(allocator_.root()->total_count_of_brp_quarantined_slots.load(
                std::memory_order_relaxed),
            0U);

  // Memory should get quarantined.
  allocator_.root()->Free(raw_ptr1);
  EXPECT_GT(allocator_.root()->total_size_of_brp_quarantined_bytes.load(
                std::memory_order_relaxed),
            0U);
  EXPECT_EQ(allocator_.root()->total_count_of_brp_quarantined_slots.load(
                std::memory_order_relaxed),
            1U);

  // Non quarantined free should not effect total_size_of_brp_quarantined_bytes
  void* raw_ptr2 = allocator_.root()->Alloc(sizeof(uint64_t), "");
  allocator_.root()->Free(raw_ptr2);

  // Freeing quarantined memory should bring the size back down to zero.
  wrapped_ptr1 = nullptr;
  EXPECT_EQ(allocator_.root()->total_size_of_brp_quarantined_bytes.load(
                std::memory_order_relaxed),
            0U);
  EXPECT_EQ(allocator_.root()->total_count_of_brp_quarantined_slots.load(
                std::memory_order_relaxed),
            0U);
}

void RunBackupRefPtrImplAdvanceTest(
    partition_alloc::PartitionAllocator& allocator,
    size_t requested_size) {
  char* ptr = static_cast<char*>(allocator.root()->Alloc(requested_size, ""));
  raw_ptr<char> protected_ptr = ptr;

  protected_ptr += 123;
  protected_ptr -= 123;
  protected_ptr = protected_ptr + 123;
  protected_ptr = protected_ptr - 123;
  protected_ptr += requested_size / 2;
  protected_ptr =
      protected_ptr + requested_size / 2;  // end-of-allocation address is ok
  EXPECT_CHECK_DEATH(protected_ptr = protected_ptr + 1);
  EXPECT_CHECK_DEATH(protected_ptr += 1);
  EXPECT_CHECK_DEATH(++protected_ptr);

  // Even though |protected_ptr| is already puinting to the end of the
  // allocation, assign it explicitly to make sure the underlying implementation
  // doesn't "switch" to the next slot.
  protected_ptr = ptr + requested_size;
  protected_ptr -= requested_size / 2;
  protected_ptr = protected_ptr - requested_size / 2;
  EXPECT_CHECK_DEATH(protected_ptr = protected_ptr - 1);
  EXPECT_CHECK_DEATH(protected_ptr -= 1);
  EXPECT_CHECK_DEATH(--protected_ptr);

  allocator.root()->Free(ptr);
}

TEST_F(BackupRefPtrTest, Advance) {
  // This requires some internal PartitionAlloc knowledge, but for the test to
  // work well the allocation + extras have to fill out the entire slot. That's
  // because PartitionAlloc doesn't know exact allocation size and bases the
  // guards on the slot size.
  //
  // A power of two is a safe choice for a slot size, then adjust it for extras.
  size_t slot_size = 512;
  size_t requested_size =
      allocator_.root()->AdjustSizeForExtrasSubtract(slot_size);
  // Verify that we're indeed fillin up the slot.
  ASSERT_EQ(
      requested_size,
      allocator_.root()->AllocationCapacityFromRequestedSize(requested_size));
  RunBackupRefPtrImplAdvanceTest(allocator_, requested_size);

  // We don't have the same worry for single-slot spans, as PartitionAlloc knows
  // exactly where the allocation ends.
  size_t raw_size = 300003;
  ASSERT_GT(raw_size, partition_alloc::internal::MaxRegularSlotSpanSize());
  ASSERT_LE(raw_size, partition_alloc::internal::kMaxBucketed);
  requested_size = allocator_.root()->AdjustSizeForExtrasSubtract(slot_size);
  RunBackupRefPtrImplAdvanceTest(allocator_, requested_size);

  // Same for direct map.
  raw_size = 1001001;
  ASSERT_GT(raw_size, partition_alloc::internal::kMaxBucketed);
  requested_size = allocator_.root()->AdjustSizeForExtrasSubtract(slot_size);
  RunBackupRefPtrImplAdvanceTest(allocator_, requested_size);
}

TEST_F(BackupRefPtrTest, GetDeltaElems) {
  size_t requested_size = allocator_.root()->AdjustSizeForExtrasSubtract(512);
  char* ptr1 = static_cast<char*>(allocator_.root()->Alloc(requested_size, ""));
  char* ptr2 = static_cast<char*>(allocator_.root()->Alloc(requested_size, ""));
  ASSERT_LT(ptr1, ptr2);  // There should be a ref-count between slots.
  raw_ptr<char> protected_ptr1 = ptr1;
  raw_ptr<char> protected_ptr1_2 = ptr1 + 1;
  raw_ptr<char> protected_ptr1_3 = ptr1 + requested_size - 1;
  raw_ptr<char> protected_ptr1_4 = ptr1 + requested_size;
  raw_ptr<char> protected_ptr2 = ptr2;
  raw_ptr<char> protected_ptr2_2 = ptr2 + 1;

  EXPECT_EQ(protected_ptr1_2 - protected_ptr1, 1);
  EXPECT_EQ(protected_ptr1 - protected_ptr1_2, -1);
  EXPECT_EQ(protected_ptr1_3 - protected_ptr1,
            checked_cast<ptrdiff_t>(requested_size) - 1);
  EXPECT_EQ(protected_ptr1 - protected_ptr1_3,
            -checked_cast<ptrdiff_t>(requested_size) + 1);
  EXPECT_EQ(protected_ptr1_4 - protected_ptr1,
            checked_cast<ptrdiff_t>(requested_size));
  EXPECT_EQ(protected_ptr1 - protected_ptr1_4,
            -checked_cast<ptrdiff_t>(requested_size));
  EXPECT_CHECK_DEATH(protected_ptr2 - protected_ptr1);
  EXPECT_CHECK_DEATH(protected_ptr1 - protected_ptr2);
  EXPECT_CHECK_DEATH(protected_ptr2 - protected_ptr1_4);
  EXPECT_CHECK_DEATH(protected_ptr1_4 - protected_ptr2);
  EXPECT_CHECK_DEATH(protected_ptr2_2 - protected_ptr1);
  EXPECT_CHECK_DEATH(protected_ptr1 - protected_ptr2_2);
  EXPECT_CHECK_DEATH(protected_ptr2_2 - protected_ptr1_4);
  EXPECT_CHECK_DEATH(protected_ptr1_4 - protected_ptr2_2);
  EXPECT_EQ(protected_ptr2_2 - protected_ptr2, 1);
  EXPECT_EQ(protected_ptr2 - protected_ptr2_2, -1);

  allocator_.root()->Free(ptr1);
  allocator_.root()->Free(ptr2);
}

bool IsQuarantineEmpty(partition_alloc::PartitionAllocator& allocator) {
  return allocator.root()->total_size_of_brp_quarantined_bytes.load(
             std::memory_order_relaxed) == 0;
}

struct BoundRawPtrTestHelper {
  static BoundRawPtrTestHelper* Create(
      partition_alloc::PartitionAllocator& allocator) {
    return new (allocator.root()->Alloc(sizeof(BoundRawPtrTestHelper), ""))
        BoundRawPtrTestHelper(allocator);
  }

  explicit BoundRawPtrTestHelper(partition_alloc::PartitionAllocator& allocator)
      : owning_allocator(allocator),
        once_callback(
            BindOnce(&BoundRawPtrTestHelper::DeleteItselfAndCheckIfInQuarantine,
                     Unretained(this))),
        repeating_callback(BindRepeating(
            &BoundRawPtrTestHelper::DeleteItselfAndCheckIfInQuarantine,
            Unretained(this))) {}

  void DeleteItselfAndCheckIfInQuarantine() {
    auto& allocator = owning_allocator;
    EXPECT_TRUE(IsQuarantineEmpty(allocator));

    // Since we use a non-default partition, `delete` has to be simulated.
    this->~BoundRawPtrTestHelper();
    allocator.root()->Free(this);

    EXPECT_FALSE(IsQuarantineEmpty(allocator));
  }

  partition_alloc::PartitionAllocator& owning_allocator;
  OnceClosure once_callback;
  RepeatingClosure repeating_callback;
};

// Check that bound callback arguments remain protected by BRP for the
// entire duration of a callback invocation.
TEST_F(BackupRefPtrTest, Bind) {
  // This test requires a separate partition; otherwise, unrelated allocations
  // might interfere with `IsQuarantineEmpty`.
  auto* object_for_once_callback1 = BoundRawPtrTestHelper::Create(allocator_);
  std::move(object_for_once_callback1->once_callback).Run();
  EXPECT_TRUE(IsQuarantineEmpty(allocator_));

  auto* object_for_repeating_callback1 =
      BoundRawPtrTestHelper::Create(allocator_);
  std::move(object_for_repeating_callback1->repeating_callback).Run();
  EXPECT_TRUE(IsQuarantineEmpty(allocator_));

  // `RepeatingCallback` has both lvalue and rvalue versions of `Run`.
  auto* object_for_repeating_callback2 =
      BoundRawPtrTestHelper::Create(allocator_);
  object_for_repeating_callback2->repeating_callback.Run();
  EXPECT_TRUE(IsQuarantineEmpty(allocator_));
}

#if defined(PA_REF_COUNT_CHECK_COOKIE)
TEST_F(BackupRefPtrTest, ReinterpretCast) {
  void* ptr = allocator_.root()->Alloc(16, "");
  allocator_.root()->Free(ptr);

  raw_ptr<void>* wrapped_ptr = reinterpret_cast<raw_ptr<void>*>(&ptr);
  // The reference count cookie check should detect that the allocation has
  // been already freed.
  BASE_EXPECT_DEATH(*wrapped_ptr = nullptr, "");
}
#endif

namespace {

// Install dangling raw_ptr handlers and restore them when going out of scope.
class ScopedInstallDanglingRawPtrChecks {
 public:
  ScopedInstallDanglingRawPtrChecks() {
    enabled_feature_list_.InitWithFeaturesAndParameters(
        {{features::kPartitionAllocDanglingPtr, {{"mode", "crash"}}}},
        {/* disabled_features */});
    old_detected_fn_ = partition_alloc::GetDanglingRawPtrDetectedFn();
    old_dereferenced_fn_ = partition_alloc::GetDanglingRawPtrReleasedFn();
    allocator::InstallDanglingRawPtrChecks();
  }
  ~ScopedInstallDanglingRawPtrChecks() {
    partition_alloc::SetDanglingRawPtrDetectedFn(old_detected_fn_);
    partition_alloc::SetDanglingRawPtrReleasedFn(old_dereferenced_fn_);
  }

 private:
  test::ScopedFeatureList enabled_feature_list_;
  partition_alloc::DanglingRawPtrDetectedFn* old_detected_fn_;
  partition_alloc::DanglingRawPtrReleasedFn* old_dereferenced_fn_;
};

}  // namespace

TEST_F(BackupRefPtrTest, RawPtrMayDangle) {
  ScopedInstallDanglingRawPtrChecks enable_dangling_raw_ptr_checks;

  void* ptr = allocator_.root()->Alloc(16, "");
  raw_ptr<void, DisableDanglingPtrDetection> dangling_ptr = ptr;
  allocator_.root()->Free(ptr);  // No dangling raw_ptr reported.
  dangling_ptr = nullptr;        // No dangling raw_ptr reported.
}

TEST_F(BackupRefPtrTest, RawPtrNotDangling) {
  ScopedInstallDanglingRawPtrChecks enable_dangling_raw_ptr_checks;

  void* ptr = allocator_.root()->Alloc(16, "");
  raw_ptr<void> dangling_ptr = ptr;
#if BUILDFLAG(ENABLE_DANGLING_RAW_PTR_CHECKS)
  BASE_EXPECT_DEATH(
      {
        allocator_.root()->Free(ptr);  // Dangling raw_ptr detected.
        dangling_ptr = nullptr;        // Dangling raw_ptr released.
      },
      AllOf(HasSubstr("Detected dangling raw_ptr"),
            HasSubstr("The memory was freed at:"),
            HasSubstr("The dangling raw_ptr was released at:")));
#endif
}

// Check the comparator operators work, even across raw_ptr with different
// dangling policies.
TEST_F(BackupRefPtrTest, DanglingPtrComparison) {
  ScopedInstallDanglingRawPtrChecks enable_dangling_raw_ptr_checks;

  void* ptr_1 = allocator_.root()->Alloc(16, "");
  void* ptr_2 = allocator_.root()->Alloc(16, "");

  if (ptr_1 > ptr_2)
    std::swap(ptr_1, ptr_2);

  raw_ptr<void, DisableDanglingPtrDetection> dangling_ptr_1 = ptr_1;
  raw_ptr<void, DisableDanglingPtrDetection> dangling_ptr_2 = ptr_2;
  raw_ptr<void> not_dangling_ptr_1 = ptr_1;
  raw_ptr<void> not_dangling_ptr_2 = ptr_2;

  EXPECT_EQ(dangling_ptr_1, not_dangling_ptr_1);
  EXPECT_EQ(dangling_ptr_2, not_dangling_ptr_2);
  EXPECT_NE(dangling_ptr_1, not_dangling_ptr_2);
  EXPECT_NE(dangling_ptr_2, not_dangling_ptr_1);
  EXPECT_LT(dangling_ptr_1, not_dangling_ptr_2);
  EXPECT_GT(dangling_ptr_2, not_dangling_ptr_1);
  EXPECT_LT(not_dangling_ptr_1, dangling_ptr_2);
  EXPECT_GT(not_dangling_ptr_2, dangling_ptr_1);

  not_dangling_ptr_1 = nullptr;
  not_dangling_ptr_2 = nullptr;

  allocator_.root()->Free(ptr_1);
  allocator_.root()->Free(ptr_2);
}

// Check the assignment operator works, even across raw_ptr with different
// dangling policies.
TEST_F(BackupRefPtrTest, DanglingPtrAssignment) {
  ScopedInstallDanglingRawPtrChecks enable_dangling_raw_ptr_checks;

  void* ptr = allocator_.root()->Alloc(16, "");

  raw_ptr<void, DisableDanglingPtrDetection> dangling_ptr_1;
  raw_ptr<void, DisableDanglingPtrDetection> dangling_ptr_2;
  raw_ptr<void> not_dangling_ptr;

  dangling_ptr_1 = ptr;

  not_dangling_ptr = dangling_ptr_1;
  dangling_ptr_1 = nullptr;

  dangling_ptr_2 = not_dangling_ptr;
  not_dangling_ptr = nullptr;

  allocator_.root()->Free(ptr);

  dangling_ptr_1 = dangling_ptr_2;
  dangling_ptr_2 = nullptr;

  not_dangling_ptr = dangling_ptr_1;
  dangling_ptr_1 = nullptr;
}

// Check the copy constructor works, even across raw_ptr with different dangling
// policies.
TEST_F(BackupRefPtrTest, DanglingPtrCopyContructor) {
  ScopedInstallDanglingRawPtrChecks enable_dangling_raw_ptr_checks;

  void* ptr = allocator_.root()->Alloc(16, "");

  raw_ptr<void, DisableDanglingPtrDetection> dangling_ptr_1(ptr);
  raw_ptr<void> not_dangling_ptr_1(ptr);

  raw_ptr<void, DisableDanglingPtrDetection> dangling_ptr_2(not_dangling_ptr_1);
  raw_ptr<void> not_dangling_ptr_2(dangling_ptr_1);

  not_dangling_ptr_1 = nullptr;
  not_dangling_ptr_2 = nullptr;

  allocator_.root()->Free(ptr);
}

TEST_F(BackupRefPtrTest, RawPtrExtractAsDangling) {
  ScopedInstallDanglingRawPtrChecks enable_dangling_raw_ptr_checks;

  raw_ptr<int> ptr =
      static_cast<int*>(allocator_.root()->Alloc(sizeof(int), ""));
  allocator_.root()->Free(
      ptr.ExtractAsDangling());  // No dangling raw_ptr reported.
  EXPECT_EQ(ptr, nullptr);
}

TEST_F(BackupRefPtrTest, RawPtrDeleteWithoutExtractAsDangling) {
  ScopedInstallDanglingRawPtrChecks enable_dangling_raw_ptr_checks;

  raw_ptr<int> ptr =
      static_cast<int*>(allocator_.root()->Alloc(sizeof(int), ""));
#if BUILDFLAG(ENABLE_DANGLING_RAW_PTR_CHECKS)
  BASE_EXPECT_DEATH(
      {
        allocator_.root()->Free(ptr.get());  // Dangling raw_ptr detected.
        ptr = nullptr;                       // Dangling raw_ptr released.
      },
      AllOf(HasSubstr("Detected dangling raw_ptr"),
            HasSubstr("The memory was freed at:"),
            HasSubstr("The dangling raw_ptr was released at:")));
#endif
}

#endif  // BUILDFLAG(USE_BACKUP_REF_PTR) &&
        // !defined(MEMORY_TOOL_REPLACES_ALLOCATOR)

#if BUILDFLAG(USE_ASAN_BACKUP_REF_PTR)

struct AsanStruct {
  int x;

  void func() { ++x; }
};

#define ASAN_BRP_PROTECTED(x) "MiraclePtr Status: PROTECTED\\n.*" x
#define ASAN_BRP_MANUAL_ANALYSIS(x) \
  "MiraclePtr Status: MANUAL ANALYSIS REQUIRED\\n.*" x
#define ASAN_BRP_NOT_PROTECTED(x) "MiraclePtr Status: NOT PROTECTED\\n.*" x

const char kAsanBrpProtected_Dereference[] =
    ASAN_BRP_PROTECTED("dangling pointer was being dereferenced");
const char kAsanBrpProtected_Callback[] = ASAN_BRP_PROTECTED(
    "crash occurred inside a callback where a raw_ptr<T> pointing to the same "
    "region");
const char kAsanBrpMaybeProtected_Extraction[] = ASAN_BRP_MANUAL_ANALYSIS(
    "pointer to the same region was extracted from a raw_ptr<T>");
const char kAsanBrpNotProtected_Instantiation[] = ASAN_BRP_NOT_PROTECTED(
    "pointer to an already freed region was assigned to a raw_ptr<T>");
const char kAsanBrpNotProtected_EarlyAllocation[] = ASAN_BRP_NOT_PROTECTED(
    "crash occurred while accessing a region that was allocated before "
    "MiraclePtr was activated");
const char kAsanBrpNotProtected_NoRawPtrAccess[] =
    ASAN_BRP_NOT_PROTECTED("No raw_ptr<T> access to this region was detected");
const char kAsanBrpMaybeProtected_Race[] =
    ASAN_BRP_MANUAL_ANALYSIS("\\nThe \"use\" and \"free\" threads don't match");
const char kAsanBrpMaybeProtected_ThreadPool[] =
    ASAN_BRP_MANUAL_ANALYSIS("\\nThis crash occurred in the thread pool");

#undef ASAN_BRP_PROTECTED
#undef ASAN_BRP_MANUAL_ANALYSIS
#undef ASAN_BRP_NOT_PROTECTED

class AsanBackupRefPtrTest : public testing::Test {
 protected:
  void SetUp() override {
    if (!RawPtrAsanService::GetInstance().IsEnabled()) {
      base::RawPtrAsanService::GetInstance().Configure(
          base::EnableDereferenceCheck(true), base::EnableExtractionCheck(true),
          base::EnableInstantiationCheck(true));
    } else {
      ASSERT_TRUE(base::RawPtrAsanService::GetInstance()
                      .is_dereference_check_enabled());
      ASSERT_TRUE(
          base::RawPtrAsanService::GetInstance().is_extraction_check_enabled());
      ASSERT_TRUE(base::RawPtrAsanService::GetInstance()
                      .is_instantiation_check_enabled());
    }
  }

  static void SetUpTestSuite() { early_allocation_ptr_ = new AsanStruct; }
  static void TearDownTestSuite() { delete early_allocation_ptr_; }
  static raw_ptr<AsanStruct> early_allocation_ptr_;
};

raw_ptr<AsanStruct> AsanBackupRefPtrTest::early_allocation_ptr_ = nullptr;

TEST_F(AsanBackupRefPtrTest, Dereference) {
  raw_ptr<AsanStruct> protected_ptr = new AsanStruct;

  // The four statements below should succeed.
  (*protected_ptr).x = 1;
  (*protected_ptr).func();
  ++(protected_ptr->x);
  protected_ptr->func();

  delete protected_ptr.get();

  EXPECT_DEATH_IF_SUPPORTED((*protected_ptr).x = 1,
                            kAsanBrpProtected_Dereference);
  EXPECT_DEATH_IF_SUPPORTED((*protected_ptr).func(),
                            kAsanBrpProtected_Dereference);
  EXPECT_DEATH_IF_SUPPORTED(++(protected_ptr->x),
                            kAsanBrpProtected_Dereference);
  EXPECT_DEATH_IF_SUPPORTED(protected_ptr->func(),
                            kAsanBrpProtected_Dereference);

  // The following statement should not trigger a dereference, so it should
  // succeed without crashing even though *protected_ptr is no longer valid.
  [[maybe_unused]] AsanStruct* ptr = protected_ptr;
}

TEST_F(AsanBackupRefPtrTest, Extraction) {
  raw_ptr<AsanStruct> protected_ptr = new AsanStruct;

  AsanStruct* ptr1 = protected_ptr;  // Shouldn't crash.
  ptr1->x = 0;

  delete protected_ptr.get();

  EXPECT_DEATH_IF_SUPPORTED(
      {
        AsanStruct* ptr2 = protected_ptr;
        ptr2->x = 1;
      },
      kAsanBrpMaybeProtected_Extraction);
}

TEST_F(AsanBackupRefPtrTest, Instantiation) {
  AsanStruct* ptr = new AsanStruct;

  raw_ptr<AsanStruct> protected_ptr1 = ptr;  // Shouldn't crash.
  protected_ptr1 = nullptr;

  delete ptr;

  EXPECT_DEATH_IF_SUPPORTED(
      { [[maybe_unused]] raw_ptr<AsanStruct> protected_ptr2 = ptr; },
      kAsanBrpNotProtected_Instantiation);
}

TEST_F(AsanBackupRefPtrTest, InstantiationInvalidPointer) {
  void* ptr1 = reinterpret_cast<void*>(0xfefefefefefefefe);

  [[maybe_unused]] raw_ptr<void> protected_ptr1 = ptr1;  // Shouldn't crash.

  size_t shadow_scale, shadow_offset;
  __asan_get_shadow_mapping(&shadow_scale, &shadow_offset);
  [[maybe_unused]] raw_ptr<void> protected_ptr2 =
      reinterpret_cast<void*>(shadow_offset);  // Shouldn't crash.
}

TEST_F(AsanBackupRefPtrTest, UserPoisoned) {
  AsanStruct* ptr = new AsanStruct;
  __asan_poison_memory_region(ptr, sizeof(AsanStruct));

  [[maybe_unused]] raw_ptr<AsanStruct> protected_ptr1 =
      ptr;  // Shouldn't crash.

  delete ptr;  // Should crash now.
  EXPECT_DEATH_IF_SUPPORTED(
      { [[maybe_unused]] raw_ptr<AsanStruct> protected_ptr2 = ptr; },
      kAsanBrpNotProtected_Instantiation);
}

TEST_F(AsanBackupRefPtrTest, EarlyAllocationDetection) {
  if (!early_allocation_ptr_) {
    // We can't run this test if we don't have a pre-BRP-ASan-initialization
    // allocation.
    return;
  }
  raw_ptr<AsanStruct> late_allocation_ptr = new AsanStruct;
  EXPECT_FALSE(RawPtrAsanService::GetInstance().IsSupportedAllocation(
      early_allocation_ptr_.get()));
  EXPECT_TRUE(RawPtrAsanService::GetInstance().IsSupportedAllocation(
      late_allocation_ptr.get()));

  delete late_allocation_ptr.get();
  delete early_allocation_ptr_.get();

  EXPECT_FALSE(RawPtrAsanService::GetInstance().IsSupportedAllocation(
      early_allocation_ptr_.get()));
  EXPECT_TRUE(RawPtrAsanService::GetInstance().IsSupportedAllocation(
      late_allocation_ptr.get()));

  EXPECT_DEATH_IF_SUPPORTED({ early_allocation_ptr_->func(); },
                            kAsanBrpNotProtected_EarlyAllocation);
  EXPECT_DEATH_IF_SUPPORTED({ late_allocation_ptr->func(); },
                            kAsanBrpProtected_Dereference);

  early_allocation_ptr_ = nullptr;
}

TEST_F(AsanBackupRefPtrTest, BoundRawPtr) {
  // This test is for the handling of raw_ptr<T> type objects being passed
  // directly to Bind.

  raw_ptr<AsanStruct> protected_ptr = new AsanStruct;

  // First create our test callbacks while `*protected_ptr` is still valid, and
  // we will then invoke them after deleting `*protected_ptr`.

  // `ptr` is protected in this callback, as raw_ptr<T> will be mapped to an
  // UnretainedWrapper containing a raw_ptr<T> which is guaranteed to outlive
  // the function call.
  auto ptr_callback = base::BindOnce(
      [](AsanStruct* ptr) {
        // This will crash and should be detected as a protected access.
        ptr->func();
      },
      protected_ptr);

  // Now delete `*protected_ptr` and check that the callbacks we created are
  // handled correctly.
  delete protected_ptr.get();
  protected_ptr = nullptr;

  EXPECT_DEATH_IF_SUPPORTED(std::move(ptr_callback).Run(),
                            kAsanBrpProtected_Callback);
}

TEST_F(AsanBackupRefPtrTest, BoundArgumentsProtected) {
  raw_ptr<AsanStruct> protected_ptr = new AsanStruct;
  raw_ptr<AsanStruct> protected_ptr2 = new AsanStruct;

  // First create our test callbacks while `*protected_ptr` is still valid, and
  // we will then invoke them after deleting `*protected_ptr`.

  // `ptr` is protected in this callback even after `*ptr` has been deleted,
  // since the allocation will be kept alive by the internal `raw_ptr<T>` inside
  // base::Unretained().
  auto safe_callback = base::BindOnce(
      [](AsanStruct* ptr) {
        // This will crash and should be detected as a protected access.
        ptr->func();
      },
      base::Unretained(protected_ptr));

  // Both `inner_ptr` and `outer_ptr` are protected in these callbacks, since
  // both are bound before `*ptr` is deleted. This test is making sure that
  // `inner_ptr` is treated as protected.
  auto safe_nested_inner_callback = base::BindOnce(
      [](AsanStruct* outer_ptr, base::OnceClosure inner_callback) {
        std::move(inner_callback).Run();
        // This will never be executed, as we will crash in inner_callback
        ASSERT_TRUE(false);
      },
      base::Unretained(protected_ptr),
      base::BindOnce(
          [](AsanStruct* inner_ptr) {
            // This will crash and should be detected as a protected access.
            inner_ptr->func();
          },
          base::Unretained(protected_ptr2)));

  // Both `inner_ptr` and `outer_ptr` are protected in these callbacks, since
  // both are bound before `*ptr` is deleted. This test is making sure that
  // `outer_ptr` is still treated as protected after `inner_callback` has run.
  auto safe_nested_outer_callback = base::BindOnce(
      [](AsanStruct* outer_ptr, base::OnceClosure inner_callback) {
        std::move(inner_callback).Run();
        // This will crash and should be detected as a protected access.
        outer_ptr->func();
      },
      base::Unretained(protected_ptr),
      base::BindOnce(
          [](AsanStruct* inner_ptr) {
            // Do nothing.
          },
          base::Unretained(protected_ptr2)));

  // Now delete `*protected_ptr` and check that the callbacks we created are
  // handled correctly.
  delete protected_ptr.get();
  delete protected_ptr2.get();
  protected_ptr = nullptr;
  protected_ptr2 = nullptr;

  EXPECT_DEATH_IF_SUPPORTED(std::move(safe_callback).Run(),
                            kAsanBrpProtected_Callback);
  EXPECT_DEATH_IF_SUPPORTED(std::move(safe_nested_inner_callback).Run(),
                            kAsanBrpProtected_Callback);
  EXPECT_DEATH_IF_SUPPORTED(std::move(safe_nested_outer_callback).Run(),
                            kAsanBrpProtected_Callback);
}

TEST_F(AsanBackupRefPtrTest, BoundArgumentsNotProtected) {
  raw_ptr<AsanStruct> protected_ptr = new AsanStruct;

  // First create our test callbacks while `*protected_ptr` is still valid, and
  // we will then invoke them after deleting `*protected_ptr`.

  // `ptr` is not protected in this callback after `*ptr` has been deleted, as
  // integer-type bind arguments do not use an internal `raw_ptr<T>`.
  auto unsafe_callback = base::BindOnce(
      [](uintptr_t address) {
        AsanStruct* ptr = reinterpret_cast<AsanStruct*>(address);
        // This will crash and should not be detected as a protected access.
        ptr->func();
      },
      reinterpret_cast<uintptr_t>(protected_ptr.get()));

  // In this case, `outer_ptr` is protected in these callbacks, since it is
  // bound before `*ptr` is deleted. We want to make sure that the access to
  // `inner_ptr` is not automatically treated as protected (although it actually
  // is) because we're trying to limit the protection scope to be very
  // conservative here.
  auto unsafe_nested_inner_callback = base::BindOnce(
      [](AsanStruct* outer_ptr, base::OnceClosure inner_callback) {
        std::move(inner_callback).Run();
        // This will never be executed, as we will crash in inner_callback
        NOTREACHED();
      },
      base::Unretained(protected_ptr),
      base::BindOnce(
          [](uintptr_t inner_address) {
            AsanStruct* inner_ptr =
                reinterpret_cast<AsanStruct*>(inner_address);
            // This will crash and should be detected as maybe protected, since
            // it follows an extraction operation when the outer callback is
            // invoked
            inner_ptr->func();
          },
          reinterpret_cast<uintptr_t>(protected_ptr.get())));

  // In this case, `inner_ptr` is protected in these callbacks, since it is
  // bound before `*ptr` is deleted. We want to make sure that the access to
  // `outer_ptr` is not automatically treated as protected, since it isn't.
  auto unsafe_nested_outer_callback = base::BindOnce(
      [](uintptr_t outer_address, base::OnceClosure inner_callback) {
        { std::move(inner_callback).Run(); }
        AsanStruct* outer_ptr = reinterpret_cast<AsanStruct*>(outer_address);
        // This will crash and should be detected as maybe protected, since it
        // follows an extraction operation when the inner callback is invoked.
        outer_ptr->func();
      },
      reinterpret_cast<uintptr_t>(protected_ptr.get()),
      base::BindOnce(
          [](AsanStruct* inner_ptr) {
            // Do nothing
          },
          base::Unretained(protected_ptr)));

  // Now delete `*protected_ptr` and check that the callbacks we created are
  // handled correctly.
  delete protected_ptr.get();
  protected_ptr = nullptr;

  EXPECT_DEATH_IF_SUPPORTED(std::move(unsafe_callback).Run(),
                            kAsanBrpNotProtected_NoRawPtrAccess);
  EXPECT_DEATH_IF_SUPPORTED(std::move(unsafe_nested_inner_callback).Run(),
                            kAsanBrpMaybeProtected_Extraction);
  EXPECT_DEATH_IF_SUPPORTED(std::move(unsafe_nested_outer_callback).Run(),
                            kAsanBrpMaybeProtected_Extraction);
}

TEST_F(AsanBackupRefPtrTest, BoundArgumentsInstantiation) {
  // This test is ensuring that instantiations of `raw_ptr` inside callbacks are
  // handled correctly.

  raw_ptr<AsanStruct> protected_ptr = new AsanStruct;

  // First create our test callback while `*protected_ptr` is still valid.
  auto callback = base::BindRepeating(
      [](AsanStruct* ptr) {
        // This will crash if `*protected_ptr` is not valid.
        [[maybe_unused]] raw_ptr<AsanStruct> copy_ptr = ptr;
      },
      base::Unretained(protected_ptr));

  // It is allowed to create a new `raw_ptr<T>` inside a callback while
  // `*protected_ptr` is still valid.
  callback.Run();

  delete protected_ptr.get();
  protected_ptr = nullptr;

  // It is not allowed to create a new `raw_ptr<T>` inside a callback once
  // `*protected_ptr` is no longer valid.
  EXPECT_DEATH_IF_SUPPORTED(std::move(callback).Run(),
                            kAsanBrpNotProtected_Instantiation);
}

TEST_F(AsanBackupRefPtrTest, BoundReferences) {
  auto ptr = ::std::make_unique<AsanStruct>();

  // This test is ensuring that reference parameters inside callbacks are
  // handled correctly.

  // We should not crash during unwrapping a reference parameter if the
  // parameter is not accessed inside the callback.
  auto no_crash_callback = base::BindOnce(
      [](AsanStruct& ref) {
        // There should be no crash here as we don't access ref.
      },
      std::reference_wrapper(*ptr));

  // `ref` is protected in this callback even after `*ptr` has been deleted,
  // since the allocation will be kept alive by the internal `raw_ref<T>` inside
  // base::UnretainedRefWrapper().
  auto callback = base::BindOnce(
      [](AsanStruct& ref) {
        // This will crash and should be detected as protected
        ref.func();
      },
      std::reference_wrapper(*ptr));

  ptr.reset();

  std::move(no_crash_callback).Run();

  EXPECT_DEATH_IF_SUPPORTED(std::move(callback).Run(),
                            kAsanBrpProtected_Callback);
}

TEST_F(AsanBackupRefPtrTest, FreeOnAnotherThread) {
  auto ptr = ::std::make_unique<AsanStruct>();
  raw_ptr<AsanStruct> protected_ptr = ptr.get();

  std::thread thread([&ptr] { ptr.reset(); });
  thread.join();

  EXPECT_DEATH_IF_SUPPORTED(protected_ptr->func(), kAsanBrpMaybeProtected_Race);
}

TEST_F(AsanBackupRefPtrTest, AccessOnThreadPoolThread) {
  auto ptr = ::std::make_unique<AsanStruct>();
  raw_ptr<AsanStruct> protected_ptr = ptr.get();

  test::TaskEnvironment env;
  RunLoop run_loop;

  ThreadPool::PostTaskAndReply(
      FROM_HERE, {}, base::BindLambdaForTesting([&ptr, &protected_ptr] {
        ptr.reset();
        EXPECT_DEATH_IF_SUPPORTED(protected_ptr->func(),
                                  kAsanBrpMaybeProtected_ThreadPool);
      }),
      base::BindLambdaForTesting([&run_loop]() { run_loop.Quit(); }));
  run_loop.Run();
}

#endif  // BUILDFLAG(USE_ASAN_BACKUP_REF_PTR)

#if defined(RAW_PTR_USE_MTE_CHECKED_PTR)

static constexpr size_t kTagOffsetForTest = 2;

struct MTECheckedPtrImplPartitionAllocSupportForTest {
  static bool EnabledForPtr(void* ptr) { return !!ptr; }

  static ALWAYS_INLINE void* TagPointer(uintptr_t ptr) {
    return reinterpret_cast<void*>(ptr - kTagOffsetForTest);
  }
};

using MTECheckedPtrImplForTest =
    MTECheckedPtrImpl<MTECheckedPtrImplPartitionAllocSupportForTest>;

TEST(MTECheckedPtrImpl, WrapAndSafelyUnwrap) {
  // Create a fake allocation, with first 2B for tag.
  // It is ok to use a fake allocation, instead of PartitionAlloc, because
  // MTECheckedPtrImplForTest fakes the functionality is enabled for this
  // pointer and points to the tag appropriately.
  unsigned char bytes[] = {0xBA, 0x42, 0x78, 0x89};
  void* ptr = bytes + kTagOffsetForTest;
  ASSERT_EQ(0x78, *static_cast<char*>(ptr));
  uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

  uintptr_t mask = 0xFFFFFFFFFFFFFFFF;
  if (sizeof(partition_alloc::PartitionTag) < 2)
    mask = 0x00FFFFFFFFFFFFFF;

  uintptr_t wrapped =
      reinterpret_cast<uintptr_t>(MTECheckedPtrImplForTest::WrapRawPtr(ptr));
  // The bytes before the allocation will be used as tag (in reverse
  // order due to little-endianness).
  ASSERT_EQ(wrapped, (addr | 0x42BA000000000000) & mask);
  ASSERT_EQ(MTECheckedPtrImplForTest::SafelyUnwrapPtrForDereference(
                reinterpret_cast<void*>(wrapped)),
            ptr);

  // Modify the tag in the fake allocation.
  bytes[0] |= 0x40;
  wrapped =
      reinterpret_cast<uintptr_t>(MTECheckedPtrImplForTest::WrapRawPtr(ptr));
  ASSERT_EQ(wrapped, (addr | 0x42FA000000000000) & mask);
  ASSERT_EQ(MTECheckedPtrImplForTest::SafelyUnwrapPtrForDereference(
                reinterpret_cast<void*>(wrapped)),
            ptr);
}

TEST(MTECheckedPtrImpl, SafelyUnwrapDisabled) {
  // Create a fake allocation, with first 2B for tag.
  // It is ok to use a fake allocation, instead of PartitionAlloc, because
  // MTECheckedPtrImplForTest fakes the functionality is enabled for this
  // pointer and points to the tag appropriately.
  unsigned char bytes[] = {0xBA, 0x42, 0x78, 0x89};
  unsigned char* ptr = bytes + kTagOffsetForTest;
  ASSERT_EQ(0x78, *ptr);
  ASSERT_EQ(MTECheckedPtrImplForTest::SafelyUnwrapPtrForDereference(ptr), ptr);
}

TEST(MTECheckedPtrImpl, CrashOnTagMismatch) {
  // Create a fake allocation, using the first two bytes for the tag.
  // It is ok to use a fake allocation, instead of PartitionAlloc, because
  // MTECheckedPtrImplForTest fakes the functionality is enabled for this
  // pointer and points to the tag appropriately.
  unsigned char bytes[] = {0xBA, 0x42, 0x78, 0x89};
  unsigned char* ptr =
      MTECheckedPtrImplForTest::WrapRawPtr(bytes + kTagOffsetForTest);
  EXPECT_EQ(*MTECheckedPtrImplForTest::SafelyUnwrapPtrForDereference(ptr),
            0x78);
  // Clobber the tag associated with the fake allocation.
  bytes[0] = 0;
  EXPECT_DEATH_IF_SUPPORTED(
      if (*MTECheckedPtrImplForTest::SafelyUnwrapPtrForDereference(ptr) ==
          0x78) return,
      "");
}

#if !defined(MEMORY_TOOL_REPLACES_ALLOCATOR) && \
    BUILDFLAG(USE_PARTITION_ALLOC_AS_MALLOC)

// This test works only when PartitionAlloc is used, when tags are enabled.
// Don't enable it when MEMORY_TOOL_REPLACES_ALLOCATOR is defined, because it
// makes PartitionAlloc take a different path that doesn't provide tags, thus no
// crash on UaF, thus missing the EXPECT_DEATH_IF_SUPPORTED expectation.
TEST(MTECheckedPtrImpl, CrashOnUseAfterFree) {
  int* unwrapped_ptr = new int;
  // Use the actual CheckedPtr implementation, not a test substitute, to
  // exercise real PartitionAlloc paths.
  raw_ptr<int> ptr = unwrapped_ptr;
  *ptr = 42;
  EXPECT_EQ(*ptr, 42);
  delete unwrapped_ptr;
  EXPECT_DEATH_IF_SUPPORTED(g_volatile_int_to_ignore = *ptr, "");
}

TEST(MTECheckedPtrImpl, CrashOnUseAfterFree_WithOffset) {
  const uint8_t kSize = 100;
  uint8_t* unwrapped_ptr = new uint8_t[kSize];
  // Use the actual CheckedPtr implementation, not a test substitute, to
  // exercise real PartitionAlloc paths.
  raw_ptr<uint8_t> ptrs[kSize];
  for (uint8_t i = 0; i < kSize; ++i) {
    ptrs[i] = static_cast<uint8_t*>(unwrapped_ptr) + i;
  }
  for (uint8_t i = 0; i < kSize; ++i) {
    *ptrs[i] = 42 + i;
    EXPECT_TRUE(*ptrs[i] == 42 + i);
  }
  delete[] unwrapped_ptr;
  for (uint8_t i = 0; i < kSize; i += 15) {
    EXPECT_DEATH_IF_SUPPORTED(g_volatile_int_to_ignore = *ptrs[i], "");
  }
}

TEST(MTECheckedPtrImpl, DirectMapCrashOnUseAfterFree) {
  // Alloc two super pages' worth of ints.
  constexpr size_t kIntsPerSuperPage =
      partition_alloc::kSuperPageSize / sizeof(int);
  constexpr size_t kAllocAmount = kIntsPerSuperPage * 2;
  int* unwrapped_ptr = new int[kAllocAmount];
  ASSERT_TRUE(partition_alloc::internal::IsManagedByDirectMap(
      partition_alloc::UntagPtr(unwrapped_ptr)));

  // Use the actual CheckedPtr implementation, not a test substitute, to
  // exercise real PartitionAlloc paths.
  raw_ptr<int> ptr = unwrapped_ptr;

  *ptr = 42;
  EXPECT_EQ(*ptr, 42);
  *(ptr + kIntsPerSuperPage) = 42;
  EXPECT_EQ(*(ptr + kIntsPerSuperPage), 42);
  *(ptr + kAllocAmount - 1) = 42;
  EXPECT_EQ(*(ptr + kAllocAmount - 1), 42);

  delete[] unwrapped_ptr;
  EXPECT_DEATH_IF_SUPPORTED(g_volatile_int_to_ignore = *ptr, "");
}

TEST(MTECheckedPtrImpl, AdvancedPointerShiftedAppropriately) {
  uint64_t* unwrapped_ptr = new uint64_t[6];
  CountingRawPtr<uint64_t> ptr = unwrapped_ptr;

  // This is a non-fixture test, so we need to unset all
  // counters manually.
  RawPtrCountingImpl::ClearCounters();

  // This is unwrapped, but still useful for ensuring that the
  // shift is sized in `uint64_t`s.
  auto original_addr = reinterpret_cast<uintptr_t>(ptr.get());
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 1,
              }),
              CountingRawPtrHasCounts());

  ptr += 5;
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr.get()) - original_addr,
            5 * sizeof(uint64_t));
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 2,
              }),
              CountingRawPtrHasCounts());
  delete[] unwrapped_ptr;

  EXPECT_DEATH_IF_SUPPORTED(*ptr, "");

  // We assert that no visible extraction actually took place.
  EXPECT_THAT((CountingRawPtrExpectations{
                  .get_for_dereference_cnt = 0,
                  .get_for_extraction_cnt = 2,
              }),
              CountingRawPtrHasCounts());
}

// Verifies that MTECheckedPtr allows the extraction of the raw pointee
// pointing just one byte beyond the end. (Dereference is still
// undefined behavior.)
TEST(MTECheckedPtrImpl, PointerBeyondAllocationCanBeExtracted) {
  // This test was most meaningful when MTECheckedPtr had the error
  // in its implementation (crbug.com/1364476), i.e. in cases where
  // the allocation end was flush with the slot end: the next byte
  // would lie outside said slot. When fixed with the extra byte
  // padding, this is never true.
  //
  // Without asserting any particular knowledge of PartitionAlloc's
  // internals (i.e. what allocation size will produce exactly
  // this situation), we perform the same test for a range of
  // allocation sizes. Note that this test doesn't do anything when
  // the PA cookie is present at slot's end.
  for (size_t size = 16; size <= 64; ++size) {
    char* unwrapped_ptr = new char[size];
    raw_ptr<char> wrapped_ptr = unwrapped_ptr;
    char unused = 0;

    // There are no real expectations here - we just don't expect the
    // test to crash when we get() the raw pointers.
    //
    // Getting the last allocated char definitely cannot crash.
    char* unwrapped_last_char = (wrapped_ptr + size - 1).get();

    // Trivial expectation to prevent "unused variable" warning.
    EXPECT_THAT(unwrapped_last_char, testing::NotNull());

    // Getting the char just beyond the allocation area must not crash.
    // Today, MTECheckedPtr is patched with a one-byte "extra" that
    // makes this not crash.
    char* unwrapped_char_beyond = (wrapped_ptr + size).get();

    // This is bad behavior (clients must not dereference beyond-the-end
    // pointers), but it is in the same slot, so MTECheckedPtr sees no
    // issue with the tag. Therefore, this must not crash.
    unused = *(wrapped_ptr + size);

    // Trivial statements to prevent "unused variable" warnings.
    unused = 0;
    EXPECT_EQ(unused, 0);
    EXPECT_THAT(unwrapped_char_beyond, testing::NotNull());

    delete[] unwrapped_ptr;
  }
}

#endif  // !defined(MEMORY_TOOL_REPLACES_ALLOCATOR) &&
        // BUILDFLAG(USE_PARTITION_ALLOC_AS_MALLOC)

#endif  // defined(RAW_PTR_USE_MTE_CHECKED_PTR)

}  // namespace internal
}  // namespace base
