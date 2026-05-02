#pragma once

#include "Particle.h"
#include "Simulation.h"

namespace djssp {

class PSOHH {
public:
    PSOParams p;
    int K;
    int F;
    RNG& rng;
    std::vector<Particle> swarm;
    std::vector<double> gbest;
    double gbest_fit = 1e300;

    explicit PSOHH(PSOParams params, int rule_count, RNG& rng, int feature_count = 4);

    int dimension() const;
    const std::vector<double>& particle_weights(int i) const;
    void update_fitness(int i, double fit);
    void move();
    int select_rule(const std::vector<double>& theta, const StateVector& s, uint64_t seed, double eps) const;
};

double run_single_rule_episode(
    const std::vector<Job>& base_jobs,
    const std::vector<Machine>& base_machines,
    const StateFeatures& feats,
    GanttLogger* logger,
    const std::string& rule_name,
    uint64_t seq_base,
    bool use_gt = true
);

double run_psohh_scenario(
    const std::vector<Job>& base_jobs,
    const std::vector<Machine>& base_machines,
    const StateFeatures& feats,
    GanttLogger* logger,
    RNG& rng_pso,
    uint64_t seq_base,
    int pso_iters,
    int swarm_size
);

}  // namespace djssp
