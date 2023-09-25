#pragma once

#include <vector>

namespace audioLib {
namespace sharandom {
std::vector<float> generate(long long seed, size_t count);
} // namespace sharandom
} // namespace audioLib