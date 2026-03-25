#pragma once

#include "Common.h"

namespace djssp {

struct Operation {
    JobId job_id{};
    int op_index{};
    MachineId machine_id{};
    double proc_time{};
    double start_time = std::numeric_limits<double>::quiet_NaN();
    double end_time = std::numeric_limits<double>::quiet_NaN();
    bool done = false;
};

}  // namespace djssp
