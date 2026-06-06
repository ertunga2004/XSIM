#pragma once

#include "Common.h"

namespace djssp {

struct ScheduleOperation {
    std::string run_id;
    std::string instance;
    int job_id = 0;
    int operation_id = 0;
    int machine_id = 0;
    double start = 0.0;
    double end = 0.0;
    double processing_time = 0.0;
    int sequence_index = 0;
};

struct ScheduleResult {
    std::vector<ScheduleOperation> operations;
    double cmax = 0.0;
};

}  // namespace djssp
