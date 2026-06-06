#pragma once

#include "FeatureVector.h"
#include "Job.h"
#include "Machine.h"

namespace djssp {

struct FeatureConfig {
    int q_max = 50;
    int wip_max = 50;
    double rw_scale = 200.0;
};

std::vector<std::string> default_feature_names();
bool validate_feature_names(const std::vector<std::string>& names, std::string* error_message = nullptr);

class FeatureVectorBuilder {
public:
    explicit FeatureVectorBuilder(FeatureConfig config);
    FeatureVectorBuilder(FeatureConfig config, std::vector<std::string> feature_names);

    const std::vector<std::string>& feature_names() const;
    int feature_count() const;

    FeatureVector build(
        const std::vector<Machine>& machines,
        const std::vector<Job>& jobs,
        const Machine& focus_m,
        double t
    ) const;

private:
    FeatureConfig cfg_;
    std::vector<std::string> feature_names_;
};

}  // namespace djssp
