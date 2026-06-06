#pragma once

<<<<<<< HEAD
#include "Job.h"
#include "Machine.h"
=======
#include "FeatureVectorBuilder.h"
>>>>>>> e2a2af7 (Checkpoint: P0-P3 refactor completed)

namespace djssp {

struct StateVector {
    std::vector<double> x;
};

<<<<<<< HEAD
struct FeatureConfig {
    int q_max = 50;
    int wip_max = 50;
    double rw_scale = 200.0;
};

=======
>>>>>>> e2a2af7 (Checkpoint: P0-P3 refactor completed)
inline double remaining_work(const Job& job) {
    double total = 0.0;
    for (int k = job.next_op; k < static_cast<int>(job.ops.size()); ++k) {
        total += job.ops[k].proc_time;
    }
    return total;
}

struct StateFeatures {
    FeatureConfig cfg;
<<<<<<< HEAD

    explicit StateFeatures(FeatureConfig config) : cfg(config) {}
=======
    FeatureVectorBuilder builder;

    explicit StateFeatures(FeatureConfig config) : cfg(config), builder(config) {}

    StateFeatures(FeatureConfig config, std::vector<std::string> feature_names)
        : cfg(config), builder(config, std::move(feature_names)) {}
>>>>>>> e2a2af7 (Checkpoint: P0-P3 refactor completed)

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
<<<<<<< HEAD
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
=======
        const FeatureVector feature_vector = builder.build(machines, jobs, focus_m, t);
        StateVector state;
        state.x = feature_vector.values;
        return state;
    }

    const std::vector<std::string>& feature_names() const {
        return builder.feature_names();
    }

    int feature_count() const {
        return builder.feature_count();
    }
>>>>>>> e2a2af7 (Checkpoint: P0-P3 refactor completed)
};

}  // namespace djssp
