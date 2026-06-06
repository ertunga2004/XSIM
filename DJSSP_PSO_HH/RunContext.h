#pragma once

#include "Common.h"

#include <chrono>
#include <filesystem>

namespace djssp {

namespace fs = std::filesystem;

struct LocalTimestamp {
    std::string compact;
    std::string iso8601;
};

struct SolverRunConfig {
    std::string sgs = "gt";
    int iters = 0;
    int swarm = 0;
    int evalk = 0;
    int finalk = 0;
    double eps0 = 0.0;
    double epsmin = 0.0;
    bool fitavg = false;
    bool traindet = false;
};

struct ImprovementRunConfig {
    bool enabled = false;
    std::string type;
    int slsiters = 0;
    int tsiters = 0;
    int tabu = 0;
    std::string tsmove;
};

struct RunContext {
    std::string schema_version = "1.0";
    std::string run_id;
    std::string instance;
    std::string method = "PSO-HH";
    std::string status = "success";
    std::string objective = "cmax";
    uint64_t seed = 0;
    SolverRunConfig solver;
    ImprovementRunConfig improvement;
    double runtime_sec = 0.0;
    fs::path output_dir;
    std::string timestamp_local;
    std::string xsim_version = "dev";
    std::string git_commit = "unknown";
    std::string compiler = "g++";
    std::string cpp_standard = "c++17";
    std::string build_type = "Release";
    std::string os = "unknown";
    std::string cpu = "unknown";
    int threads = 1;
    std::string instance_source = "data/jobshop1.txt";
    std::string config_source = "unknown";
    std::vector<std::string> active_rules;
    std::vector<std::string> active_features;
    std::string config_original_json;
    std::string config_resolved_json;
    std::string command;
    std::string best_cmax_line;
};

LocalTimestamp make_local_timestamp();
std::string build_command_line(int argc, char** argv);
std::string build_config_fingerprint(
    const std::string& instance,
    uint64_t seed,
    const SolverRunConfig& solver,
    const ImprovementRunConfig& improvement,
    const std::vector<std::string>& active_rules = {},
    const std::vector<std::string>& active_features = {}
);
std::string make_run_id(
    const LocalTimestamp& timestamp,
    const std::string& instance,
    uint64_t seed,
    const std::string& short_hash
);
std::string detect_os_name();

}  // namespace djssp
