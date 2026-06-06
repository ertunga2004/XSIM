#include "ScheduleGenerator.h"

namespace djssp {

SimResult GtScheduleGenerator::generate(
    const std::vector<Job>& base_jobs,
    const std::vector<Machine>& base_machines,
    const StateFeatures& feats,
    const std::vector<IRule*>& rules,
    GanttLogger* logger,
    const std::vector<double>& policy_w,
    const PSOHH& hh,
    uint64_t seed_seq_base,
    double current_eps
) const {
    Simulation sim(base_jobs, base_machines, rules, feats, logger, true);
    return sim.run_episode(policy_w, hh, seed_seq_base, current_eps);
}

}  // namespace djssp
