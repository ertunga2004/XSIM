#include "FeasibilityChecker.h"

namespace djssp {

namespace {

constexpr double kTolerance = 1e-9;

void add_violation(FeasibilityResult& result, const std::string& message) {
    result.valid = false;
    result.violations.push_back(message);
}

bool nearly_equal(double a, double b) {
    return std::fabs(a - b) <= kTolerance;
}

}  // namespace

FeasibilityResult check_schedule_feasibility(
    const std::vector<Job>& jobs,
    int machine_count,
    const ScheduleResult& schedule,
    double expected_cmax
) {
    FeasibilityResult result;
    result.actual_operation_count = static_cast<int>(schedule.operations.size());

    for (const auto& job : jobs) {
        result.expected_operation_count += static_cast<int>(job.ops.size());
    }

    if (result.actual_operation_count != result.expected_operation_count) {
        result.operation_count_ok = false;
        add_violation(
            result,
            "operation count mismatch: expected " + std::to_string(result.expected_operation_count) +
                ", actual " + std::to_string(result.actual_operation_count)
        );
    }

    std::vector<std::vector<const ScheduleOperation*>> by_job(jobs.size());
    for (size_t j = 0; j < jobs.size(); ++j) {
        by_job[j].assign(jobs[j].ops.size(), nullptr);
    }

    std::vector<std::vector<const ScheduleOperation*>> by_machine(static_cast<size_t>(std::max(0, machine_count)));

    for (const auto& op : schedule.operations) {
        result.schedule_cmax = std::max(result.schedule_cmax, op.end);

        if (op.job_id < 0 || op.job_id >= static_cast<int>(jobs.size())) {
            result.start_end_valid = false;
            add_violation(result, "job id out of range in schedule: job " + std::to_string(op.job_id));
            continue;
        }

        const auto& job = jobs[op.job_id];
        if (op.operation_id < 0 || op.operation_id >= static_cast<int>(job.ops.size())) {
            result.start_end_valid = false;
            add_violation(
                result,
                "operation id out of range in schedule: job " + std::to_string(op.job_id) +
                    " operation " + std::to_string(op.operation_id)
            );
            continue;
        }

        if (by_job[op.job_id][op.operation_id] != nullptr) {
            result.duplicate_operation_ok = false;
            add_violation(
                result,
                "duplicate operation: job " + std::to_string(op.job_id) +
                    " operation " + std::to_string(op.operation_id)
            );
        }
        by_job[op.job_id][op.operation_id] = &op;

        const Operation& model_op = job.ops[op.operation_id];
        if (op.machine_id != model_op.machine_id) {
            result.start_end_valid = false;
            add_violation(
                result,
                "machine mismatch: job " + std::to_string(op.job_id) +
                    " operation " + std::to_string(op.operation_id)
            );
        }
        if (!nearly_equal(op.processing_time, model_op.proc_time)) {
            result.start_end_valid = false;
            add_violation(
                result,
                "processing time mismatch: job " + std::to_string(op.job_id) +
                    " operation " + std::to_string(op.operation_id)
            );
        }
        if (op.start < -kTolerance || op.end + kTolerance < op.start ||
            !nearly_equal(op.end - op.start, op.processing_time)) {
            result.start_end_valid = false;
            add_violation(
                result,
                "invalid start/end: job " + std::to_string(op.job_id) +
                    " operation " + std::to_string(op.operation_id)
            );
        }

        if (op.machine_id < 0 || op.machine_id >= machine_count) {
            result.start_end_valid = false;
            add_violation(result, "machine id out of range in schedule: machine " + std::to_string(op.machine_id));
        } else {
            by_machine[op.machine_id].push_back(&op);
        }
    }

    for (size_t j = 0; j < by_job.size(); ++j) {
        for (size_t k = 0; k < by_job[j].size(); ++k) {
            if (by_job[j][k] == nullptr) {
                result.duplicate_operation_ok = false;
                add_violation(
                    result,
                    "missing operation: job " + std::to_string(j) +
                        " operation " + std::to_string(k)
                );
            }
        }
    }

    for (const auto& job_ops : by_job) {
        for (size_t k = 1; k < job_ops.size(); ++k) {
            const ScheduleOperation* prev = job_ops[k - 1];
            const ScheduleOperation* cur = job_ops[k];
            if (prev == nullptr || cur == nullptr) {
                continue;
            }
            if (cur->start + kTolerance < prev->end) {
                result.precedence_ok = false;
                add_violation(
                    result,
                    "precedence violation: job " + std::to_string(cur->job_id) +
                        " operation " + std::to_string(cur->operation_id)
                );
            }
        }
    }

    for (auto& machine_ops : by_machine) {
        std::sort(machine_ops.begin(), machine_ops.end(), [](const ScheduleOperation* a, const ScheduleOperation* b) {
            if (a->start != b->start) return a->start < b->start;
            if (a->end != b->end) return a->end < b->end;
            if (a->job_id != b->job_id) return a->job_id < b->job_id;
            return a->operation_id < b->operation_id;
        });

        for (size_t i = 1; i < machine_ops.size(); ++i) {
            const ScheduleOperation* prev = machine_ops[i - 1];
            const ScheduleOperation* cur = machine_ops[i];
            if (cur->start + kTolerance < prev->end) {
                result.machine_capacity_ok = false;
                add_violation(
                    result,
                    "machine overlap: machine " + std::to_string(cur->machine_id) +
                        " between job " + std::to_string(prev->job_id) +
                        "/op " + std::to_string(prev->operation_id) +
                        " and job " + std::to_string(cur->job_id) +
                        "/op " + std::to_string(cur->operation_id)
                );
            }
        }
    }

    if (!nearly_equal(result.schedule_cmax, expected_cmax)) {
        result.cmax_consistent = false;
        add_violation(
            result,
            "cmax mismatch: schedule " + std::to_string(result.schedule_cmax) +
                ", expected " + std::to_string(expected_cmax)
        );
    }

    result.valid = result.operation_count_ok &&
        result.duplicate_operation_ok &&
        result.precedence_ok &&
        result.machine_capacity_ok &&
        result.start_end_valid &&
        result.cmax_consistent &&
        result.violations.empty();

    return result;
}

}  // namespace djssp
