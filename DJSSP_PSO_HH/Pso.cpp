#include "Pso.h"

#include "RuleRegistry.h"
#include "ScheduleGenerator.h"

namespace djssp {

PSOHH::PSOHH(PSOParams params, int rule_count, RNG& rng_ref, int feature_count)
    : p(params), K(rule_count), F(feature_count), rng(rng_ref) {
    const int D = dimension();
    swarm.resize(p.swarm_size);

    for (auto& particle : swarm) {
        particle.x.resize(D);
        particle.v.resize(D);
        particle.pbest.resize(D);

        for (int d = 0; d < D; ++d) {
            particle.x[d] = (rng.u01() - 0.5) * 0.5;
            particle.v[d] = (rng.u01() - 0.5) * 0.1;
        }

        particle.pbest = particle.x;
    }

    if (!swarm.empty()) {
        gbest = swarm[0].x;
    }
}

int PSOHH::dimension() const {
    return K * (F + 1);
}

const std::vector<double>& PSOHH::particle_weights(int i) const {
    return swarm.at(i).x;
}

void PSOHH::update_fitness(int i, double fit) {
    auto& particle = swarm.at(i);
    if (fit < particle.pbest_fit) {
        particle.pbest_fit = fit;
        particle.pbest = particle.x;
    }
    if (fit < gbest_fit) {
        gbest_fit = fit;
        gbest = particle.x;
    }
}

void PSOHH::move() {
    const double XMAX = 6.0;
    const double VMAX = 1.0;
    const int D = dimension();

    for (auto& particle : swarm) {
        const double r1 = rng.u01();
        const double r2 = rng.u01();

        for (int d = 0; d < D; ++d) {
            particle.v[d] = p.w_inertia * particle.v[d]
                + p.c1 * r1 * (particle.pbest[d] - particle.x[d])
                + p.c2 * r2 * (gbest[d] - particle.x[d]);
            particle.v[d] = clampd(particle.v[d], -VMAX, VMAX);
            particle.x[d] += particle.v[d];
            particle.x[d] = clampd(particle.x[d], -XMAX, XMAX);
        }
    }
}

int PSOHH::select_rule(const std::vector<double>& theta, const StateVector& s, uint64_t seed, double eps) const {
    uint64_t x = seed ^ 0xD1B54A32D192ED03ULL;
    eps = std::clamp(eps, 0.0, 0.95);

    if (u01_from_u64(x) < eps) {
        return randint_from_u64(x, K);
    }

    int best_rule = 0;
    double best_score = -1e300;

    for (int k = 0; k < K; ++k) {
        const int base = k * (F + 1);
        double score = theta.at(base);
        for (int j = 0; j < F; ++j) {
            score += theta.at(base + 1 + j) * s.x.at(j);
        }

        const double noise =
            static_cast<double>(static_cast<int64_t>(splitmix64(x))) * (1.0 / 9.22e18) * 1e-12;
        score += noise;

        if (score > best_score) {
            best_score = score;
            best_rule = k;
        }
    }

    return best_rule;
}

double run_single_rule_episode(
    const std::vector<Job>& base_jobs,
    const std::vector<Machine>& base_machines,
    const StateFeatures& feats,
    GanttLogger* logger,
    const std::string& rule_name,
    uint64_t seq_base,
    bool use_gt
) {
    IRule* rule = get_rule_by_name(rule_name);
    if (rule == nullptr) {
        return 1e300;
    }

    std::vector<IRule*> rules;
    rules.push_back(rule);

    RNG dummy_rng(1);
    PSOParams params;
    params.swarm_size = 1;
    params.iters = 1;
    PSOHH dummy_hh(params, 1, dummy_rng, feats.feature_count());

    std::vector<double> weights(dummy_hh.dimension(), 0.0);
    weights[0] = 1.0;

    const auto generator = create_schedule_generator(use_gt);
    const SimResult result = generator->generate(
        base_jobs,
        base_machines,
        feats,
        rules,
        logger,
        weights,
        dummy_hh,
        seq_base,
        0.0
    );
    return result.Cmax;
}

double run_psohh_scenario(
    const std::vector<Job>& base_jobs,
    const std::vector<Machine>& base_machines,
    const StateFeatures& feats,
    GanttLogger* logger,
    RNG& rng_pso,
    uint64_t seq_base,
    int pso_iters,
    int swarm_size
) {
    PSOParams params;
    params.iters = pso_iters;
    params.swarm_size = swarm_size;

    const int K = static_cast<int>(build_rule_names().size());
    PSOHH hh(params, K, rng_pso, feats.feature_count());

    for (int iter = 0; iter < params.iters; ++iter) {
        const double frac = (params.iters <= 1)
            ? 1.0
            : (1.0 - static_cast<double>(iter) / static_cast<double>(params.iters - 1));
        const double current_eps = std::max(params.eps_min, params.eps0 * frac);

        for (int i = 0; i < params.swarm_size; ++i) {
            const auto generator = create_schedule_generator(true);
            const auto& weights = hh.particle_weights(i);
            const uint64_t stream = seq_base + static_cast<uint64_t>(iter) * 100000ULL +
                static_cast<uint64_t>(i) * 1000ULL;
            const SimResult result = generator->generate(
                base_jobs,
                base_machines,
                feats,
                build_rules(),
                logger,
                weights,
                hh,
                stream,
                current_eps
            );
            hh.update_fitness(i, result.Cmax);
        }

        hh.move();
    }

    return hh.gbest_fit;
}

}  // namespace djssp
