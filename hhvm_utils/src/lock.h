/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-present Facebook, Inc. (http://www.facebook.com)  |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#pragma once

#include "hphp/util/mutex.h"
#include "hphp/util/synchronizable.h"
#include "hphp/util/synchronizable-multi.h"

namespace HPHP {
///////////////////////////////////////////////////////////////////////////////

/**
 * Lock instrumentation for mutex stats.
 */
struct LockProfiler {
  typedef void (*PFUNC_PROFILE)(const std::string &stack, int64_t elapsed_us);
  static PFUNC_PROFILE s_pfunc_profile;
  static bool s_profile;
  static int s_profile_sampling;

  explicit LockProfiler(bool profile);
  ~LockProfiler();

private:
  bool m_profiling;
  timespec m_lockTime;
};

///////////////////////////////////////////////////////////////////////////////

template <typename MutexT>
struct BaseConditionalLock {
  BaseConditionalLock(MutexT &mutex, bool condition, bool profile = true)
    : m_profiler(profile), m_mutex(mutex), m_acquired(false) {
    if (condition) {
      m_mutex.lock(); // must not throw
      m_acquired = true;
    }
  }
  ~BaseConditionalLock() {
    if (m_acquired) {
      m_mutex.unlock(); // must not throw
    }
  }
private:
  LockProfiler m_profiler;
  MutexT&      m_mutex;
  bool         m_acquired;
};

struct ConditionalLock : BaseConditionalLock<Mutex> {
  ConditionalLock(Mutex &mutex,
                  bool condition, bool profile = true)
    : BaseConditionalLock<Mutex>(mutex, condition, profile)
  {}
  ConditionalLock(Synchronizable *obj,
                  bool condition, bool profile = true)
    : BaseConditionalLock<Mutex>(obj->getMutex(), condition, profile)
  {}
  ConditionalLock(SynchronizableMulti *obj,
                  bool condition, bool profile = true)
    : BaseConditionalLock<Mutex>(obj->getMutex(), condition, profile)
  {}
};

/**
 * Just a helper class that automatically unlocks a mutex when it goes out of
 * scope.
 *
 * {
 *   Lock lock(mutex);
 *   // inside lock
 * } // unlock here
 */
struct Lock : ConditionalLock {
  explicit Lock(Mutex &mutex, bool profile = true)
    : ConditionalLock(mutex, true, profile) {}
  explicit Lock(Synchronizable *obj, bool profile = true)
    : ConditionalLock(obj, true, profile) {}
  explicit Lock(SynchronizableMulti *obj, bool profile = true)
    : ConditionalLock(obj, true, profile) {}
};

struct ScopedUnlock {
  explicit ScopedUnlock(Mutex &mutex) : m_mutex(mutex) {
    m_mutex.unlock();
  }
  explicit ScopedUnlock(Synchronizable *obj) : m_mutex(obj->getMutex()) {
    m_mutex.unlock();
  }
  explicit ScopedUnlock(SynchronizableMulti *obj) : m_mutex(obj->getMutex()) {
    m_mutex.unlock();
  }

  ~ScopedUnlock() {
    m_mutex.lock();
  }

private:
  Mutex &m_mutex;
};

struct SimpleConditionalLock : BaseConditionalLock<SimpleMutex> {
  SimpleConditionalLock(SimpleMutex &mutex,
                        bool condition, bool profile = true)
    : BaseConditionalLock<SimpleMutex>(mutex, condition, profile)
  {
    if (condition) {
      mutex.assertOwnedBySelf();
    }
  }
};

struct SimpleLock : SimpleConditionalLock {
  explicit SimpleLock(SimpleMutex &mutex, bool profile = true)
    : SimpleConditionalLock(mutex, true, profile) {}
};

///////////////////////////////////////////////////////////////////////////////

struct ReadLock {
  explicit ReadLock(ReadWriteMutex& mutex, bool profile = true)
    : m_profiler(profile)
    , m_mutex(mutex)
  {
    m_mutex.acquireRead();
  }

  ReadLock(const ReadLock&) = delete;
  ReadLock& operator=(const ReadLock&) = delete;

  ~ReadLock() {
    m_mutex.release();
  }

private:
  LockProfiler m_profiler;
  ReadWriteMutex& m_mutex;
};

struct WriteLock {
  explicit WriteLock(ReadWriteMutex& mutex, bool profile = true)
    : m_profiler(profile)
    , m_mutex(mutex)
  {
    m_mutex.acquireWrite();
  }

  WriteLock(const WriteLock&) = delete;
  WriteLock& operator=(const WriteLock&) = delete;

  ~WriteLock() {
    m_mutex.release();
  }

private:
  LockProfiler m_profiler;
  ReadWriteMutex& m_mutex;
};

///////////////////////////////////////////////////////////////////////////////
}

