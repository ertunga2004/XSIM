#pragma once

#include "Operation.h"

namespace djssp {

struct Machine {
    MachineId id{};
    double busy_until = 0.0;
    Operation* running = nullptr;
    std::deque<Operation*> ready_q;

    bool isIdle(double t) const {
        return running == nullptr && t >= busy_until;
    }
};

}  // namespace djssp
