#pragma once

#include "Common.h"

#include <filesystem>

namespace djssp {

struct BatchConfig {
    std::string schema_version = "1.0";
    std::string suite_name = "batch";
    std::vector<std::string> runs;
    std::filesystem::path source_path;
    std::string original_json;
};

}  // namespace djssp
