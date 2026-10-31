#pragma once
#include <cstddef>
#include <string>

namespace weasel {

struct P2cComposition {
  std::string keys;
  std::string prefix;
  std::string identity;
};

inline bool P2cNormalizeKeys(const std::string& input, std::string* keys) {
  keys->clear();
  for (char c : input) {
    if (c >= 'a' && c <= 'z')
      keys->push_back(c);
    else if (c != '\'' && c != ' ')
      return false;
  }
  return !keys->empty();
}

// Rime preedit 的 sel_start 是已转换前缀后的 UTF-8 字节位置。
// 仅接受能与原始输入末尾严格对齐的全拼；不能对齐时不重新猜前缀。
inline bool P2cReadComposition(const std::string& raw,
                              const std::string& preedit,
                              size_t selected_start,
                              size_t caret,
                              P2cComposition* out) {
  if (caret != raw.size() || selected_start > preedit.size())
    return false;
  P2cComposition result;
  std::string all;
  std::string remaining = preedit.substr(selected_start);
  const std::string soft_cursor = "\xe2\x80\xb8";
  if (remaining.size() >= soft_cursor.size() &&
      remaining.compare(remaining.size() - soft_cursor.size(),
                        soft_cursor.size(), soft_cursor) == 0)
    remaining.resize(remaining.size() - soft_cursor.size());
  if (!P2cNormalizeKeys(raw, &all) ||
      !P2cNormalizeKeys(remaining, &result.keys) ||
      result.keys.size() > all.size() ||
      all.compare(all.size() - result.keys.size(), result.keys.size(),
                  result.keys) != 0)
    return false;
  result.prefix = preedit.substr(0, selected_start);
  if (selected_start == 0 && result.keys != all)
    return false;
  result.identity = raw + "\n" + result.prefix + "\n" + result.keys;
  *out = result;
  return true;
}

}  // namespace weasel
