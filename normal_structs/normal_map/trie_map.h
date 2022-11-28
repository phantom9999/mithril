#pragma once

#include <string>
#include <vector>
#include <algorithm>

template<typename Type>
class TrieMap {
 public:

  /***** 增/改 *****/

  // 在 Map 中添加 key
  void put(std::string key, Type val);

  /***** 删 *****/

  // 删除键 key 以及对应的值
  void remove(std::string key);

  /***** 查 *****/

  // 搜索 key 对应的值，不存在则返回 null
  // get("the") -> 4
  // get("tha") -> null
  Type get(std::string key);

  // 判断 key 是否存在在 Map 中
  // containsKey("tea") -> false
  // containsKey("team") -> true
  bool containsKey(std::string key);

  // 在 Map 的所有键中搜索 query 的最短前缀
  // shortestPrefixOf("themxyz") -> "the"
  std::string shortestPrefixOf(std::string query);

  // 在 Map 的所有键中搜索 query 的最长前缀
  // longestPrefixOf("themxyz") -> "them"
  std::string longestPrefixOf(std::string query);

  // 搜索所有前缀为 prefix 的键
  // keysWithPrefix("th") -> ["that", "the", "them"]
  std::vector<std::string> keysWithPrefix(std::string prefix);

  // 判断是和否存在前缀为 prefix 的键
  // hasKeyWithPrefix("tha") -> true
  // hasKeyWithPrefix("apple") -> false
  bool hasKeyWithPrefix(std::string prefix);

  // 通配符 . 匹配任意字符，搜索所有匹配的键
  // keysWithPattern("t.a.") -> ["team", "that"]
  std::vector<std::string> keysWithPattern(std::string pattern);

  // 通配符 . 匹配任意字符，判断是否存在匹配的键
  // hasKeyWithPattern(".ip") -> true
  // hasKeyWithPattern(".i") -> false
  bool hasKeyWithPattern(std::string pattern);

  // 返回 Map 中键值对的数量
  int size();
};

