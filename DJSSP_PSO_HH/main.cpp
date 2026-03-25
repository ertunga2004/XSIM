#include "InstanceGenerator.h"
#include "Pso.h"

namespace djssp {
namespace {

struct Stats {
    double mean = 0.0;
    double std = 0.0;
    double mn = 0.0;
    double mx = 0.0;
};

Stats compute_stats(const std::vector<double>& values) {
    Stats stats;
    if (values.empty()) {
        return stats;
    }

    stats.mn = *std::min_element(values.begin(), values.end());
    stats.mx = *std::max_element(values.begin(), values.end());
    stats.mean = std::accumulate(values.begin(), values.end(), 0.0) /
        static_cast<double>(values.size());

    double var = 0.0;
    for (double value : values) {
        var += (value - stats.mean) * (value - stats.mean);
    }
    var /= static_cast<double>(values.size());
    stats.std = std::sqrt(var);
    return stats;
}

void run_experiments(const std::string& orlib_path) {
    const std::vector<std::string> instances = {"ft10", "la16", "la21", "abz6", "abz7"};
    const std::vector<double> lambdas = {0.2, 0.5, 1.0};
    const int replications = 30;

    FeatureConfig fc;
    fc.q_max = 50;
    fc.wip_max = 200;
    fc.rw_scale = 200.0;
    StateFeatures feats(fc);

    const std::vector<std::string> baselines = {"SPT", "LPT", "MWKR", "MOR", "FIFO"};
    std::vector<std::string> methods = {"PSOHH"};
    methods.insert(methods.end(), baselines.begin(), baselines.end());

    std::ofstream raw("raw_results.csv");
    raw << "instance,n_jobs,n_machines,lambda,seed,method,Cmax\n";

    std::ofstream summary("summary_results.csv");
    summary << "instance,n_jobs,n_machines,lambda,method,mean,std,min,max\n";

    std::ofstream comparison_avg("comparison_avg.csv");
    std::ofstream comparison_best("comparison_best.csv");

    comparison_avg << "instance,lambda";
    comparison_best << "instance,lambda";
    for (const auto& method : methods) {
        comparison_avg << "," << method;
        comparison_best << "," << method;
    }
    comparison_avg << "\n";
    comparison_best << "\n";

    for (const auto& instance : instances) {
        for (double lambda : lambdas) {
            std::unordered_map<std::string, std::vector<double>> cmax_by_method;
            for (const auto& method : methods) {
                cmax_by_method[method] = {};
            }

            int n_jobs = 0;
            int n_machines = 0;

            for (int rep = 1; rep <= replications; ++rep) {
                RNG rng_scn(1000ULL * static_cast<uint64_t>(rep) + 17ULL);
                RNG rng_pso(9000ULL * static_cast<uint64_t>(rep) + 99ULL);

                std::vector<Job> jobs;
                std::string err;
                if (!load_orlib_jobshop1_instance(orlib_path, instance, jobs, n_machines, &err)) {
                    throw std::runtime_error("[ORLIB] " + err);
                }

                n_jobs = static_cast<int>(jobs.size());
                assign_release_times_poisson(jobs, rng_scn, lambda);
                const auto machines = build_machines(n_machines);
                GanttLogger* logger = nullptr;

                {
                    const double Cmax = run_psohh_scenario(
                        jobs,
                        machines,
                        feats,
                        logger,
                        rng_pso,
                        static_cast<uint64_t>(rep) * 1000000ULL,
                        25,
                        15
                    );

                    cmax_by_method["PSOHH"].push_back(Cmax);
                    raw << instance << "," << n_jobs << "," << n_machines << ","
                        << lambda << "," << rep << ",PSOHH," << Cmax << "\n";
                }

                for (size_t i = 0; i < baselines.size(); ++i) {
                    const double Cmax = run_single_rule_episode(
                        jobs,
                        machines,
                        feats,
                        logger,
                        baselines[i],
                        static_cast<uint64_t>(rep) * 2000000ULL + static_cast<uint64_t>(i + 1)
                    );

                    cmax_by_method[baselines[i]].push_back(Cmax);
                    raw << instance << "," << n_jobs << "," << n_machines << ","
                        << lambda << "," << rep << "," << baselines[i] << "," << Cmax << "\n";
                }
            }

            comparison_avg << instance << "," << lambda;
            comparison_best << instance << "," << lambda;

            for (const auto& method : methods) {
                const Stats stats = compute_stats(cmax_by_method[method]);
                summary << instance << "," << n_jobs << "," << n_machines << ","
                    << lambda << "," << method << ","
                    << stats.mean << "," << stats.std << "," << stats.mn << "," << stats.mx << "\n";
                comparison_avg << "," << stats.mean;
                comparison_best << "," << stats.mn;
            }

            comparison_avg << "\n";
            comparison_best << "\n";
        }
    }

    std::cerr << "Experiments finished. Wrote raw_results.csv, summary_results.csv, "
        "comparison_avg.csv, comparison_best.csv\n";
}

}  // namespace
}  // namespace djssp

int main(int argc, char** argv) {
    using namespace djssp;

    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    const std::string orlib_path = "data/jobshop1.txt";

    if (argc >= 2 && std::string(argv[1]) == "experiments") {
        run_experiments(orlib_path);
        return 0;
    }

    std::string instance = "ft06";
    int ai = 1;
    if (ai < argc && std::string(argv[ai]).rfind("--", 0) != 0) {
        instance = argv[ai];
        ++ai;
    }

    auto parse_double = [](const std::string& value, double& out) {
        try {
            out = std::stod(value);
            return true;
        } catch (...) {
            return false;
        }
    };

    auto parse_int = [](const std::string& value, int& out) {
        try {
            out = std::stoi(value);
            return true;
        } catch (...) {
            return false;
        }
    };

    PSOParams p;
    uint64_t seed = 777;
    int eval_k = 5;
    int final_k = 200;
    int sls_iters = 0;
    int ts_iters = 0;
    int tabu_tenure = 10;
    int tsmove_mode = 2;
    bool fitness_take_min = true;
    bool use_gt_sgs = true;
    int ls_iters = 0;
    double ls_step = 0.25;
    bool train_deterministic = false;

    while (ai < argc) {
        const std::string key = argv[ai++];
        auto need = [&](const std::string& what) {
            if (ai >= argc) {
                std::cerr << "Missing value after " << what << "\n";
                std::exit(1);
            }
            return std::string(argv[ai++]);
        };

        if (key == "--eps0") {
            const std::string value = need(key);
            if (!parse_double(value, p.eps0)) {
                std::cerr << "Bad --eps0 value\n";
                return 1;
            }
        } else if (key == "--epsmin") {
            const std::string value = need(key);
            if (!parse_double(value, p.eps_min)) {
                std::cerr << "Bad --epsmin value\n";
                return 1;
            }
        } else if (key == "--iters") {
            const std::string value = need(key);
            if (!parse_int(value, p.iters)) {
                std::cerr << "Bad --iters value\n";
                return 1;
            }
        } else if (key == "--swarm") {
            const std::string value = need(key);
            if (!parse_int(value, p.swarm_size)) {
                std::cerr << "Bad --swarm value\n";
                return 1;
            }
        } else if (key == "--seed") {
            const std::string value = need(key);
            try {
                seed = static_cast<uint64_t>(std::stoull(value));
            } catch (...) {
                std::cerr << "Bad --seed value\n";
                return 1;
            }
        } else if (key == "--evalk") {
            const std::string value = need(key);
            if (!parse_int(value, eval_k) || eval_k < 1) {
                std::cerr << "Bad --evalk value\n";
                return 1;
            }
        } else if (key == "--finalk") {
            const std::string value = need(key);
            if (!parse_int(value, final_k) || final_k < 1) {
                std::cerr << "Bad --finalk value\n";
                return 1;
            }
        } else if (key == "--slsiters") {
            const std::string value = need(key);
            if (!parse_int(value, sls_iters) || sls_iters < 0) {
                std::cerr << "Bad --slsiters value\n";
                return 1;
            }
        } else if (key == "--tsiters") {
            const std::string value = need(key);
            if (!parse_int(value, ts_iters) || ts_iters < 0) {
                std::cerr << "Bad --tsiters value\n";
                return 1;
            }
        } else if (key == "--tabu") {
            const std::string value = need(key);
            if (!parse_int(value, tabu_tenure) || tabu_tenure < 0) {
                std::cerr << "Bad --tabu value\n";
                return 1;
            }
        } else if (key == "--tsmove") {
            const std::string value = need(key);
            if (value == "swap") {
                tsmove_mode = 0;
            } else if (value == "insert") {
                tsmove_mode = 1;
            } else if (value == "mixed") {
                tsmove_mode = 2;
            } else {
                std::cerr << "Bad --tsmove value (use swap|insert|mixed)\n";
                return 1;
            }
        } else if (key == "--fitavg") {
            fitness_take_min = false;
        } else if (key == "--sgs") {
            const std::string value = need(key);
            if (value == "gt" || value == "GT") {
                use_gt_sgs = true;
            } else if (value == "event" || value == "EVENT") {
                use_gt_sgs = false;
            } else {
                std::cerr << "Bad --sgs value (use gt|event)\n";
                return 1;
            }
        } else if (key == "--lsiters") {
            const std::string value = need(key);
            if (!parse_int(value, ls_iters) || ls_iters < 0) {
                std::cerr << "Bad --lsiters value\n";
                return 1;
            }
        } else if (key == "--lsstep") {
            const std::string value = need(key);
            if (!parse_double(value, ls_step) || ls_step <= 0.0) {
                std::cerr << "Bad --lsstep value\n";
                return 1;
            }
        } else if (key == "--traindet") {
            train_deterministic = true;
        } else if (key == "--help" || key == "-h") {
            std::cout
                << "Usage: xsim.exe [instance] [--eps0 <v>] [--epsmin <v>] [--iters <n>] "
                << "[--swarm <n>] [--seed <n>] [--evalk <n>] [--finalk <n>] [--fitavg] "
                << "[--sgs gt|event] [--lsiters <n>] [--lsstep <v>] [--traindet] "
                << "[--slsiters <n>] [--tsiters <n>] [--tabu <tenure>] "
                << "[--tsmove swap|insert|mixed]\n";
            return 0;
        } else {
            std::cerr << "Unknown option: " << key << "\n";
            std::cerr << "Try: xsim.exe --help\n";
            return 1;
        }
    }

    std::vector<Job> base_jobs;
    int n_machines = 0;
    std::string err;
    if (!load_orlib_jobshop1_instance(orlib_path, instance, base_jobs, n_machines, &err)) {
        std::cerr << "[ORLIB] " << err << "\n";
        std::cerr << "Make sure '" << orlib_path << "' exists\n";
        return 1;
    }

    const int n_jobs = static_cast<int>(base_jobs.size());
    const auto base_machines = build_machines(n_machines);

    FeatureConfig fcfg;
    fcfg.q_max = 30;
    fcfg.wip_max = n_jobs;
    fcfg.rw_scale = 200.0;
    StateFeatures feats(fcfg);

    RNG rng_pso(seed);
    p.w_inertia = 0.7;
    p.c1 = 1.4;
    p.c2 = 1.4;

    const int K = static_cast<int>(build_rule_names().size());
    PSOHH hh(p, K, rng_pso);

    for (int iter = 0; iter < p.iters; ++iter) {
        const double frac_eps = (p.iters <= 1)
            ? 1.0
            : static_cast<double>(iter) / static_cast<double>(p.iters - 1);
        const double current_eps = std::max(p.eps_min, p.eps0 * (1.0 - frac_eps));
        const double eval_eps = train_deterministic ? 0.0 : current_eps;

        double iter_best = 1e300;
        double iter_worst = -1e300;
        double iter_sum = 0.0;

        for (int i = 0; i < p.swarm_size; ++i) {
            Simulation sim(base_jobs, base_machines, build_rules(), feats, nullptr, use_gt_sgs);
            const auto& weights = hh.particle_weights(i);

            double fit = 1e300;
            double fit_sum = 0.0;
            for (int rep = 0; rep < eval_k; ++rep) {
                const uint64_t seq_base =
                    (seed * 1315423911ULL) ^
                    (static_cast<uint64_t>(iter) * 100000ULL) ^
                    (static_cast<uint64_t>(i) * 1000ULL) ^
                    (static_cast<uint64_t>(rep) * 17ULL);
                const SimResult result = sim.run_episode(weights, hh, seq_base, eval_eps);
                if (fitness_take_min) {
                    fit = std::min(fit, result.Cmax);
                } else {
                    fit_sum += result.Cmax;
                }
            }

            if (!fitness_take_min) {
                fit = fit_sum / std::max(1, eval_k);
            }

            hh.update_fitness(i, fit);
            iter_best = std::min(iter_best, fit);
            iter_worst = std::max(iter_worst, fit);
            iter_sum += fit;
        }

        hh.move();
        std::cout << "iter=" << iter
            << "  iter_best=" << static_cast<long long>(iter_best)
            << "  iter_avg=" << (iter_sum / p.swarm_size)
            << "  iter_worst=" << static_cast<long long>(iter_worst)
            << "  gbest_Cmax=" << hh.gbest_fit
            << "\n";
    }

    auto eval_theta = [&](const std::vector<double>& theta, uint64_t seed_base) {
        Simulation sim(base_jobs, base_machines, build_rules(), feats, nullptr, use_gt_sgs);
        double fit = 1e300;
        double sum = 0.0;

        for (int rep = 0; rep < eval_k; ++rep) {
            const uint64_t seq_base =
                (seed_base * 2654435761ULL) ^
                (static_cast<uint64_t>(rep) * 97ULL) ^
                0x9E3779B97F4A7C15ULL;
            const SimResult result = sim.run_episode(theta, hh, seq_base, 0.0);
            if (fitness_take_min) {
                fit = std::min(fit, result.Cmax);
            } else {
                sum += result.Cmax;
            }
        }

        if (!fitness_take_min) {
            fit = sum / std::max(1, eval_k);
        }
        return fit;
    };

    std::vector<double> best_theta = hh.gbest;
    double best_theta_fit = eval_theta(best_theta, seed ^ 0xC0FFEEULL);

    for (size_t i = 0; i < hh.swarm.size(); ++i) {
        if (hh.swarm[i].pbest.empty()) {
            continue;
        }

        const double fit = eval_theta(
            hh.swarm[i].pbest,
            (seed ^ 0xD00DFEEDULL) + static_cast<uint64_t>(i) * 1315423911ULL
        );
        if (fit + 1e-9 < best_theta_fit) {
            best_theta_fit = fit;
            best_theta = hh.swarm[i].pbest;
        }
    }

    if (ls_iters > 0) {
        std::mt19937_64 ls_rng(seed ^ 0xA5A5A5A5ULL);
        std::normal_distribution<double> nd(0.0, ls_step);
        const double XMAX = 6.0;
        const int D = static_cast<int>(best_theta.size());

        for (int it = 0; it < ls_iters; ++it) {
            std::vector<double> candidate = best_theta;
            const int n_mut = 1 + static_cast<int>(ls_rng() % 3ULL);

            for (int mm = 0; mm < n_mut; ++mm) {
                const int d = static_cast<int>(ls_rng() % static_cast<uint64_t>(D));
                candidate[d] = clampd(candidate[d] + nd(ls_rng), -XMAX, XMAX);
            }

            const double candidate_fit =
                eval_theta(candidate, (seed + static_cast<uint64_t>(it) + 1ULL) ^ 0xBADC0DEULL);
            if (candidate_fit + 1e-9 < best_theta_fit) {
                best_theta_fit = candidate_fit;
                best_theta = std::move(candidate);
            }
        }
    }

    double best_final = 1e300;
    uint64_t best_seq = 999000000ULL;
    {
        Simulation sim(base_jobs, base_machines, build_rules(), feats, nullptr, use_gt_sgs);
        for (int rep = 0; rep < final_k; ++rep) {
            const uint64_t seq_base =
                (seed * 2654435761ULL) ^
                999000000ULL ^
                (static_cast<uint64_t>(rep) * 97ULL);
            const SimResult result = sim.run_episode(best_theta, hh, seq_base, 0.0);
            if (result.Cmax < best_final) {
                best_final = result.Cmax;
                best_seq = seq_base;
            }
        }
    }

    GanttLogger logger("gantt_" + instance + ".csv");
    SimResult final_res;
    {
        Simulation sim(base_jobs, base_machines, build_rules(), feats, &logger, use_gt_sgs);
        final_res = sim.run_episode(best_theta, hh, best_seq, 0.0);

        if (sls_iters > 0 || ts_iters > 0) {
            double new_cmax = final_res.Cmax;
            const bool ok = improve_gantt_with_schedule_ls(
                base_jobs,
                static_cast<int>(base_jobs.size()),
                static_cast<int>(base_machines.size()),
                logger.rows,
                sls_iters,
                ts_iters,
                tabu_tenure,
                tsmove_mode,
                &new_cmax
            );
            if (ok) {
                final_res.Cmax = new_cmax;
            }
        }

        logger.write_sorted();
    }

    std::cout << "Done. Best Cmax=" << static_cast<long long>(final_res.Cmax)
        << " (train_best=" << static_cast<long long>(hh.gbest_fit)
        << ", det_screen_best=" << static_cast<long long>(best_theta_fit)
        << ", best_seq_Cmax=" << static_cast<long long>(best_final)
        << ", best_seq=" << static_cast<unsigned long long>(best_seq) << ")\n";

    const auto rule_names = build_rule_names();
    for (size_t i = 0; i < rule_names.size(); ++i) {
        std::cout << rule_names[i] << (i + 1 == rule_names.size() ? '\n' : ' ');
    }

    std::cout << "Gantt written to gantt_" << instance << ".csv\n";
    return 0;
}
