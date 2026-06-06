#pragma once

#include "FeasibilityChecker.h"
#include "RunContext.h"
#include "ScheduleResult.h"
#include "Simulation.h"

namespace djssp {

ScheduleResult build_schedule_result(
    const std::string& run_id,
    const std::string& instance,
    const std::vector<GanttLogger::Entry>& rows
);

bool write_run_contract_outputs(
    const RunContext& context,
    const ScheduleResult& schedule,
    const FeasibilityResult& feasibility,
    double objective_value,
    std::string* error_message = nullptr
);

}  // namespace djssp
