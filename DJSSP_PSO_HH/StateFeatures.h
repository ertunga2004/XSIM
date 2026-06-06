#pragma once

#include "FeatureVectorBuilder.h"

namespace djssp {

struct StateVector {
    std::vector<double> x;
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
    FeatureVectorBuilder builder;

    explicit StateFeatures(FeatureConfig config) : cfg(config), builder(config) {}

    StateFeatures(FeatureConfig config, std::vector<std::string> feature_names)
        : cfg(config), builder(config, std::move(feature_names)) {}

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
};

}  // namespace djssp
