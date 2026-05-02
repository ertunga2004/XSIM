#include "InstanceGenerator.h"
#include "Pso.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>

namespace djssp {
namespace {

namespace fs = std::filesystem;

struct Stats {
    double mean = 0.0;
    double std = 0.0;
    double mn = 0.0;
    double mx = 0.0;
};

struct ConvergenceRow {
    int iteration = 0;
    double iter_best = 0.0;
    double iter_avg = 0.0;
    double iter_worst = 0.0;
    double best_cmax = 0.0;
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

std::string html_escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '"': escaped += "&quot;"; break;
            case '\'': escaped += "&#39;"; break;
            default: escaped += ch; break;
        }
    }
    return escaped;
}

std::string current_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t raw = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &raw);
#else
    localtime_r(&raw, &local);
#endif
    std::ostringstream out;
    out << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

std::string job_color(int job_id) {
    static const std::vector<std::string> colors = {
        "#2563eb", "#dc2626", "#16a34a", "#9333ea", "#ea580c",
        "#0891b2", "#be123c", "#4f46e5", "#65a30d", "#b45309",
        "#0f766e", "#7c3aed", "#c2410c", "#0369a1", "#a21caf"
    };
    return colors[static_cast<size_t>(std::max(0, job_id)) % colors.size()];
}

void write_gantt_html(
    const fs::path& output_path,
    const std::string& instance,
    const std::vector<GanttLogger::Entry>& rows
) {
    std::vector<GanttLogger::Entry> sorted = rows;
    std::sort(sorted.begin(), sorted.end(), [](const GanttLogger::Entry& a, const GanttLogger::Entry& b) {
        if (a.machine_id != b.machine_id) return a.machine_id < b.machine_id;
        if (a.start != b.start) return a.start < b.start;
        if (a.end != b.end) return a.end < b.end;
        if (a.job_id != b.job_id) return a.job_id < b.job_id;
        return a.op_index < b.op_index;
    });

    std::vector<int> machines;
    double cmax = 0.0;
    for (const auto& row : sorted) {
        cmax = std::max(cmax, row.end);
        if (std::find(machines.begin(), machines.end(), row.machine_id) == machines.end()) {
            machines.push_back(row.machine_id);
        }
    }
    std::sort(machines.begin(), machines.end());

    std::ofstream out(output_path);
    out << "<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n"
        << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        << "<title>Gantt " << html_escape(instance) << "</title>\n"
        << "<style>\n"
        << ":root{color-scheme:light;font-family:Segoe UI,Arial,sans-serif;background:#f6f7f9;color:#151922;}\n"
        << "body{margin:0;padding:24px;background:#f6f7f9;}\n"
        << ".wrap{max-width:1280px;margin:0 auto;}\n"
        << "header{display:flex;justify-content:space-between;gap:16px;align-items:flex-end;margin-bottom:18px;}\n"
        << "h1{font-size:24px;line-height:1.2;margin:0;color:#111827;}\n"
        << ".meta{font-size:13px;color:#4b5563;text-align:right;}\n"
        << ".chart{background:#fff;border:1px solid #d8dde6;border-radius:8px;overflow:auto;box-shadow:0 1px 2px rgba(15,23,42,.06);}\n"
        << ".axis{display:grid;grid-template-columns:86px minmax(760px,1fr);border-bottom:1px solid #e5e7eb;background:#f9fafb;position:sticky;top:0;z-index:2;}\n"
        << ".axis-label{padding:10px 12px;font-size:12px;font-weight:600;color:#4b5563;border-right:1px solid #e5e7eb;}\n"
        << ".ticks{position:relative;height:38px;min-width:760px;}\n"
        << ".tick{position:absolute;top:0;bottom:0;border-left:1px solid #e5e7eb;font-size:11px;color:#64748b;padding-left:5px;}\n"
        << ".row{display:grid;grid-template-columns:86px minmax(760px,1fr);border-bottom:1px solid #edf0f5;}\n"
        << ".row:last-child{border-bottom:0;}\n"
        << ".label{padding:18px 12px;font-size:13px;font-weight:600;color:#334155;border-right:1px solid #e5e7eb;background:#fbfcfe;}\n"
        << ".track{position:relative;height:58px;min-width:760px;background:linear-gradient(to right,#eef2f7 1px,transparent 1px);background-size:10% 100%;}\n"
        << ".bar{position:absolute;top:12px;height:34px;border-radius:5px;color:#fff;font-size:12px;line-height:34px;text-align:center;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;box-shadow:inset 0 -1px 0 rgba(0,0,0,.18);}\n"
        << ".bar:hover{filter:brightness(.92);z-index:3;}\n"
        << ".empty{padding:28px;color:#64748b;}\n"
        << "</style>\n</head>\n<body>\n<div class=\"wrap\">\n<header>\n"
        << "<div><h1>Gantt Chart - " << html_escape(instance) << "</h1></div>\n"
        << "<div class=\"meta\">Cmax: " << std::fixed << std::setprecision(0) << cmax
        << "<br>Generated: " << html_escape(current_timestamp()) << "</div>\n"
        << "</header>\n<div class=\"chart\">\n";

    if (sorted.empty() || cmax <= 0.0) {
        out << "<div class=\"empty\">No operations found.</div>\n";
    } else {
        out << "<div class=\"axis\"><div class=\"axis-label\">Machine</div><div class=\"ticks\">";
        for (int i = 0; i <= 4; ++i) {
            const double pct = i * 25.0;
            const double t = cmax * pct / 100.0;
            out << "<div class=\"tick\" style=\"left:" << std::setprecision(4) << pct << "%\">"
                << std::fixed << std::setprecision(0) << t << "</div>";
        }
        out << "</div></div>\n";

        out << std::fixed << std::setprecision(4);
        for (int machine_id : machines) {
            out << "<div class=\"row\"><div class=\"label\">M" << machine_id << "</div><div class=\"track\">";
            for (const auto& row : sorted) {
                if (row.machine_id != machine_id) continue;
                const double left = 100.0 * row.start / cmax;
                const double width = std::max(0.08, 100.0 * (row.end - row.start) / cmax);
                std::ostringstream title;
                title << "Machine " << row.machine_id << " | Job " << row.job_id
                      << " | Op " << row.op_index << " | Start " << std::fixed << std::setprecision(0)
                      << row.start << " | End " << row.end << " | Proc " << row.proc_time;
                out << "<div class=\"bar\" title=\"" << html_escape(title.str()) << "\" style=\"left:"
                    << left << "%;width:max(2px," << width << "%);background:" << job_color(row.job_id)
                    << "\">J" << row.job_id << " O" << row.op_index << "</div>";
            }
            out << "</div></div>\n";
        }
    }

    out << "</div>\n</div>\n</body>\n</html>\n";
}

void write_convergence_csv(const fs::path& output_path, const std::vector<ConvergenceRow>& rows) {
    std::ofstream out(output_path);
    out << "Iteration,IterBest,IterAvg,IterWorst,BestCmax\n";
    out << std::fixed << std::setprecision(6);
    for (const auto& row : rows) {
        out << row.iteration << "," << row.iter_best << "," << row.iter_avg << ","
            << row.iter_worst << "," << row.best_cmax << "\n";
    }
}

struct MethodComparison {
    std::vector<std::string> methods;
    std::vector<uint64_t> seeds;
    std::unordered_map<std::string, std::vector<double>> cmax_by_method;
};

MethodComparison evaluate_methods(
    const std::vector<Job>& base_jobs,
    const std::vector<Machine>& base_machines,
    const StateFeatures& feats,
    const std::vector<double>& best_theta,
    const PSOHH& hh,
    uint64_t seed,
    int report_runs,
    bool use_gt_sgs
) {
    MethodComparison result;
    result.methods = build_rule_names();
    result.methods.push_back("PSO-HH");

    for (const auto& method : result.methods) {
        result.cmax_by_method[method] = {};
    }

    for (int rep = 0; rep < report_runs; ++rep) {
        const uint64_t seq_base =
            (seed * 11400714819323198485ULL) ^
            0xC6BC279692B5C323ULL ^
            (static_cast<uint64_t>(rep) * 97ULL);
        result.seeds.push_back(seq_base);

        for (const auto& rule_name : build_rule_names()) {
            const double cmax = run_single_rule_episode(
                base_jobs,
                base_machines,
                feats,
                nullptr,
                rule_name,
                seq_base,
                use_gt_sgs
            );
            result.cmax_by_method[rule_name].push_back(cmax);
        }

        Simulation sim(base_jobs, base_machines, build_rules(), feats, nullptr, use_gt_sgs);
        const SimResult pso_result = sim.run_episode(best_theta, hh, seq_base, 0.0);
        result.cmax_by_method["PSO-HH"].push_back(pso_result.Cmax);
    }

    return result;
}

void write_results_csv(const fs::path& output_path, const MethodComparison& comparison) {
    std::ofstream out(output_path);
    out << "Run,Seed";
    for (const auto& method : comparison.methods) {
        out << "," << method;
    }
    out << "\n";

    out << std::fixed << std::setprecision(6);
    for (size_t i = 0; i < comparison.seeds.size(); ++i) {
        out << (i + 1) << "," << comparison.seeds[i];
        for (const auto& method : comparison.methods) {
            out << "," << comparison.cmax_by_method.at(method).at(i);
        }
        out << "\n";
    }
}

void write_summary_csv(const fs::path& output_path, const MethodComparison& comparison) {
    std::ofstream out(output_path);
    out << "Method,Best,Mean,Worst,StdDev\n";
    out << std::fixed << std::setprecision(6);
    for (const auto& method : comparison.methods) {
        const Stats stats = compute_stats(comparison.cmax_by_method.at(method));
        out << method << "," << stats.mn << "," << stats.mean << ","
            << stats.mx << "," << stats.std << "\n";
    }
}

bool write_single_run_reports(
    const std::string& instance,
    const std::string& report_dir,
    const std::vector<ConvergenceRow>& convergence,
    const MethodComparison& comparison,
    const GanttLogger& logger
) {
    std::error_code ec;
    const fs::path instance_dir = fs::path(report_dir) / instance;
    fs::create_directories(instance_dir, ec);
    if (ec) {
        std::cerr << "Could not create report directory '" << instance_dir.string()
                  << "': " << ec.message() << "\n";
        return false;
    }

    const fs::path convergence_path = instance_dir / ("convergence_" + instance + ".csv");
    const fs::path results_path = instance_dir / ("results_" + instance + ".csv");
    const fs::path summary_path = instance_dir / ("summary_" + instance + ".csv");
    const fs::path gantt_csv_path = instance_dir / ("gantt_" + instance + ".csv");
    const fs::path gantt_html_path = instance_dir / ("gantt_" + instance + ".html");

    write_convergence_csv(convergence_path, convergence);
    write_results_csv(results_path, comparison);
    write_summary_csv(summary_path, comparison);

    GanttLogger report_logger(gantt_csv_path.string());
    report_logger.rows = logger.rows;
    report_logger.write_sorted();
    write_gantt_html(gantt_html_path, instance, logger.rows);
    return true;
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
    bool write_reports = true;
    int report_runs = 30;
    std::string report_dir = "reports";

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
        } else if (key == "--no-report") {
            write_reports = false;
        } else if (key == "--report-runs") {
            const std::string value = need(key);
            if (!parse_int(value, report_runs) || report_runs < 1) {
                std::cerr << "Bad --report-runs value\n";
                return 1;
            }
        } else if (key == "--report-dir") {
            report_dir = need(key);
            if (report_dir.empty()) {
                std::cerr << "Bad --report-dir value\n";
                return 1;
            }
        } else if (key == "--help" || key == "-h") {
            std::cout
                << "Usage: xsim.exe [instance] [--eps0 <v>] [--epsmin <v>] [--iters <n>] "
                << "[--swarm <n>] [--seed <n>] [--evalk <n>] [--finalk <n>] [--fitavg] "
                << "[--sgs gt|event] [--lsiters <n>] [--lsstep <v>] [--traindet] "
                << "[--slsiters <n>] [--tsiters <n>] [--tabu <tenure>] "
                << "[--tsmove swap|insert|mixed] [--no-report] [--report-runs <n>] "
                << "[--report-dir <path>]\n";
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
    std::vector<ConvergenceRow> convergence_rows;

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

        const double iter_avg = iter_sum / p.swarm_size;
        convergence_rows.push_back(ConvergenceRow{
            iter + 1,
            iter_best,
            iter_avg,
            iter_worst,
            hh.gbest_fit
        });

        hh.move();
        std::cout << "iter=" << iter
            << "  iter_best=" << static_cast<long long>(iter_best)
            << "  iter_avg=" << iter_avg
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

    if (write_reports) {
        const MethodComparison comparison = evaluate_methods(
            base_jobs,
            base_machines,
            feats,
            best_theta,
            hh,
            seed,
            report_runs,
            use_gt_sgs
        );
        if (write_single_run_reports(instance, report_dir, convergence_rows, comparison, logger)) {
            std::cout << "Reports written to " << (fs::path(report_dir) / instance).string() << "\n";
        }
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
