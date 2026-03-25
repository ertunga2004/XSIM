#pragma once

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <deque>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <queue>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace djssp {

using JobId = int;
using MachineId = int;

struct RNG {
    std::mt19937_64 eng;

    explicit RNG(uint64_t seed) : eng(seed) {}

    double u01() {
        return std::uniform_real_distribution<double>(0.0, 1.0)(eng);
    }

    double exp_rv(double lambda) {
        if (lambda <= 0.0) {
            return 0.0;
        }
        return std::exponential_distribution<double>(lambda)(eng);
    }
};

inline double clampd(double value, double lo, double hi) {
    return (value < lo) ? lo : ((value > hi) ? hi : value);
}

inline uint64_t splitmix64(uint64_t& x) {
    uint64_t z = (x += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

inline double u01_from_u64(uint64_t& x) {
    return (splitmix64(x) >> 11) * (1.0 / 9007199254740992.0);
}

inline int randint_from_u64(uint64_t& x, int hi_exclusive) {
    if (hi_exclusive <= 0) {
        return 0;
    }
    return static_cast<int>(splitmix64(x) % static_cast<uint64_t>(hi_exclusive));
}

}  // namespace djssp
