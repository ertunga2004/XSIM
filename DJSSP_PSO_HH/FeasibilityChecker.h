#pragma once

#include "Job.h"
#include "ScheduleResult.h"

namespace djssp {

struct FeasibilityResult {
    bool valid = true;
    bool operation_count_ok = true;
    bool duplicate_operation_ok = true;
    bool precedence_ok = true;
    bool machine_capacity_ok = true;
    bool start_end_valid = true;
    bool cmax_consistent = true;
    std::vector<std::string> violations;
    int expected_operation_count = 0;
    int actual_operation_count = 0;
    double schedule_cmax = 0.0;
};

FeasibilityResult check_schedule_feasibility(
    const std::vector<Job>& jobs,
    int machine_count,
    const ScheduleResult& schedule,
    double expected_cmax
);

}  // namespace djssp
