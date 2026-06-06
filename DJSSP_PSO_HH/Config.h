#pragma once

#include "RunContext.h"

namespace djssp {

struct RunConfigBlock {
    std::string name = "default";
    uint64_t seed = 777;
    std::string output_root = "runs";
    bool write_reports = true;
};

struct InstanceConfigBlock {
    std::string source = "orlib";
    std::string name = "ft06";
    std::string path = "data/jobshop1.txt";
};

struct SolverConfigBlock {
    std::string method = "pso_hh";
    std::string sgs = "gt";
    int iters = 30;
    int swarm = 15;
    int evalk = 5;
    int finalk = 200;
    double eps0 = 0.25;
    double epsmin = 0.05;
    bool fitavg = false;
    bool traindet = false;
};

struct OutputConfigBlock {
    bool write_result_json = true;
    bool write_schedule_csv = true;
    bool write_metadata_json = true;
    bool write_convergence_csv = true;
    bool write_gantt_html = true;
};

struct XSimConfig {
    std::string schema_version = "1.0";
    RunConfigBlock run;
    InstanceConfigBlock instance;
    std::string objective = "cmax";
    SolverConfigBlock solver;
    std::vector<std::string> rules;
    bool rules_specified = false;
    std::vector<std::string> features;
    bool features_specified = false;
    ImprovementRunConfig improvement;
    OutputConfigBlock outputs;
    std::string source_path = "unknown";
    std::string original_json;
};

}  // namespace djssp
