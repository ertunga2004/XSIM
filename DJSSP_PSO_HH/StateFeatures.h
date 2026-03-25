#pragma once

#include "Job.h"
#include "Machine.h"

namespace djssp {

struct StateVector {
    std::vector<double> x;
};

struct FeatureConfig {
    int q_max = 50;
    int wip_max = 50;
    double rw_scale = 200.0;
};

inline double remaining_work(const Job& job) {
    double total = 0.0;
    for (int k = job.next_op; k < static_cast<int>(job.ops.size()); ++k) {
        total += job.ops[k].proc_time;
    }
    return total;
}

struct StateFeatures {
    FeatureConfig cfg;

    explicit StateFeatures(FeatureConfig config) : cfg(config) {}

    static double utilization(const std::vector<Machine>& machines, double t) {
        if (machines.empty()) {
            return 0.0;
        }

        int busy = 0;
        for (const auto& machine : machines) {
            if (machine.running != nullptr || t < machine.busy_until) {
                busy++;
            }
        }
        return static_cast<double>(busy) / static_cast<double>(machines.size());
    }

    StateVector extract(
        const std::vector<Machine>& machines,
        const std::vector<Job>& jobs,
        const Machine& focus_m,
        double t
    ) const {
        const double U = utilization(machines, t);
        const double Qm = std::min(
            1.0,
            static_cast<double>(focus_m.ready_q.size()) / static_cast<double>(std::max(1, cfg.q_max))
        );

        int wip = 0;
        for (const auto& job : jobs) {
            if (t >= job.release_time && !job.completed()) {
                wip++;
            }
        }
        const double WIP = std::min(
            1.0,
            static_cast<double>(wip) / static_cast<double>(std::max(1, cfg.wip_max))
        );

        double rw_sum = 0.0;
        int count = 0;
        for (auto* op : focus_m.ready_q) {
            rw_sum += remaining_work(jobs[op->job_id]);
            count++;
        }

        double RWavg = (count > 0) ? (rw_sum / static_cast<double>(count)) : 0.0;
        RWavg = std::min(1.0, RWavg / std::max(1e-9, cfg.rw_scale));

        StateVector state;
        state.x = {U, Qm, WIP, RWavg};
        return state;
    }
};

}  // namespace djssp
