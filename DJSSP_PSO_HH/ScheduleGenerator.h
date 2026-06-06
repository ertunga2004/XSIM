#pragma once

#include "RuleRegistry.h"
#include "Simulation.h"

namespace djssp {

class IScheduleGenerator {
public:
    virtual ~IScheduleGenerator() = default;

    virtual SimResult generate(
        const std::vector<Job>& base_jobs,
        const std::vector<Machine>& base_machines,
        const StateFeatures& feats,
        const std::vector<IRule*>& rules,
        GanttLogger* logger,
        const std::vector<double>& policy_w,
        const PSOHH& hh,
        uint64_t seed_seq_base,
        double current_eps
    ) const = 0;
};

class GtScheduleGenerator final : public IScheduleGenerator {
public:
    SimResult generate(
        const std::vector<Job>& base_jobs,
        const std::vector<Machine>& base_machines,
        const StateFeatures& feats,
        const std::vector<IRule*>& rules,
        GanttLogger* logger,
        const std::vector<double>& policy_w,
        const PSOHH& hh,
        uint64_t seed_seq_base,
        double current_eps
    ) const override;
};

class EventScheduleGenerator final : public IScheduleGenerator {
public:
    SimResult generate(
        const std::vector<Job>& base_jobs,
        const std::vector<Machine>& base_machines,
        const StateFeatures& feats,
        const std::vector<IRule*>& rules,
        GanttLogger* logger,
        const std::vector<double>& policy_w,
        const PSOHH& hh,
        uint64_t seed_seq_base,
        double current_eps
    ) const override;
};

inline std::unique_ptr<IScheduleGenerator> create_schedule_generator(bool use_gt) {
    if (use_gt) {
        return std::make_unique<GtScheduleGenerator>();
    }
    return std::make_unique<EventScheduleGenerator>();
}

}  // namespace djssp
