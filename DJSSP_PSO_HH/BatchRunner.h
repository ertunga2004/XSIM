#pragma once

#include "BatchConfig.h"

namespace djssp {

struct BatchRunSummaryRow {
    std::string batch_id;
    std::string suite_name;
    std::string run_id;
    std::string config_path;
    std::string instance;
    std::string status;
    std::string cmax;
    std::string runtime_sec;
    std::string feasibility_valid;
};

struct BatchRunResult {
    std::string batch_id;
    std::filesystem::path output_dir;
    std::vector<BatchRunSummaryRow> rows;
};

class BatchRunner {
public:
    static bool run(
        const std::filesystem::path& executable_path,
        const BatchConfig& config,
        BatchRunResult& out_result,
        std::string* error_message = nullptr
    );
};

}  // namespace djssp
