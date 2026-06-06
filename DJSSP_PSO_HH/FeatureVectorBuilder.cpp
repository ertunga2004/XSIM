#include "FeatureVectorBuilder.h"

namespace djssp {

namespace {

const std::vector<std::string>& known_feature_names() {
    static const std::vector<std::string> names = {
        "utilization",
        "queue_length",
        "wip",
        "remaining_work_avg"
    };
    return names;
}

bool is_known_feature(const std::string& name) {
    const auto& known = known_feature_names();
    return std::find(known.begin(), known.end(), name) != known.end();
}

double machine_utilization(const std::vector<Machine>& machines, double t) {
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

double queue_length_feature(const Machine& focus_m, const FeatureConfig& cfg) {
    return std::min(
        1.0,
        static_cast<double>(focus_m.ready_q.size()) / static_cast<double>(std::max(1, cfg.q_max))
    );
}

double wip_feature(const std::vector<Job>& jobs, double t, const FeatureConfig& cfg) {
    int wip = 0;
    for (const auto& job : jobs) {
        if (t >= job.release_time && !job.completed()) {
            wip++;
        }
    }

    return std::min(
        1.0,
        static_cast<double>(wip) / static_cast<double>(std::max(1, cfg.wip_max))
    );
}

double remaining_work_for_features(const Job& job) {
    double total = 0.0;
    for (int k = job.next_op; k < static_cast<int>(job.ops.size()); ++k) {
        total += job.ops[k].proc_time;
    }
    return total;
}

double remaining_work_avg_feature(
    const std::vector<Job>& jobs,
    const Machine& focus_m,
    const FeatureConfig& cfg
) {
    double rw_sum = 0.0;
    int count = 0;
    for (auto* op : focus_m.ready_q) {
        rw_sum += remaining_work_for_features(jobs[op->job_id]);
        count++;
    }

    double rw_avg = (count > 0) ? (rw_sum / static_cast<double>(count)) : 0.0;
    rw_avg = std::min(1.0, rw_avg / std::max(1e-9, cfg.rw_scale));
    return rw_avg;
}

}  // namespace

std::vector<std::string> default_feature_names() {
    return known_feature_names();
}

bool validate_feature_names(const std::vector<std::string>& names, std::string* error_message) {
    if (names.empty()) {
        if (error_message != nullptr) {
            *error_message = "Active feature set must contain at least one feature.";
        }
        return false;
    }

    for (const auto& name : names) {
        if (name.empty()) {
            if (error_message != nullptr) {
                *error_message = "Active feature set must not contain empty feature names.";
            }
            return false;
        }
        if (!is_known_feature(name)) {
            if (error_message != nullptr) {
                *error_message = "Unknown feature name in active feature set: " + name;
            }
            return false;
        }
    }

    if (error_message != nullptr) {
        error_message->clear();
    }
    return true;
}

FeatureVectorBuilder::FeatureVectorBuilder(FeatureConfig config)
    : FeatureVectorBuilder(config, default_feature_names()) {}

FeatureVectorBuilder::FeatureVectorBuilder(FeatureConfig config, std::vector<std::string> feature_names)
    : cfg_(config), feature_names_(std::move(feature_names)) {
    std::string error;
    if (!validate_feature_names(feature_names_, &error)) {
        throw std::invalid_argument(error);
    }
}

const std::vector<std::string>& FeatureVectorBuilder::feature_names() const {
    return feature_names_;
}

int FeatureVectorBuilder::feature_count() const {
    return static_cast<int>(feature_names_.size());
}

FeatureVector FeatureVectorBuilder::build(
    const std::vector<Machine>& machines,
    const std::vector<Job>& jobs,
    const Machine& focus_m,
    double t
) const {
    FeatureVector result;
    result.names = feature_names_;
    result.values.reserve(feature_names_.size());

    for (const auto& name : feature_names_) {
        if (name == "utilization") {
            result.values.push_back(machine_utilization(machines, t));
        } else if (name == "queue_length") {
            result.values.push_back(queue_length_feature(focus_m, cfg_));
        } else if (name == "wip") {
            result.values.push_back(wip_feature(jobs, t, cfg_));
        } else if (name == "remaining_work_avg") {
            result.values.push_back(remaining_work_avg_feature(jobs, focus_m, cfg_));
        }
    }

    return result;
}

}  // namespace djssp
