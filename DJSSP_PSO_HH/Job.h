#pragma once

#include "Operation.h"

namespace djssp {

struct Job {
    JobId id{};
    double release_time{};
    std::vector<Operation> ops;
    int next_op = 0;

    bool completed() const {
        return next_op >= static_cast<int>(ops.size());
    }
};

}  // namespace djssp
