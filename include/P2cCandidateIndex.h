#pragma once

namespace weasel {

// 生成候选不属于 librime；其余候选保留原有相对顺序。
constexpr int P2cLibrimeIndex(int visual, int generated) {
  return visual == generated ? -1 : visual - (visual > generated ? 1 : 0);
}

}  // namespace weasel
