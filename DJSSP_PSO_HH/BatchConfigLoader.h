#pragma once

#include "BatchConfig.h"

namespace djssp {

class BatchConfigLoader {
public:
    static bool load_file(
        const std::filesystem::path& path,
        BatchConfig& out_config,
        std::string* error_message = nullptr
    );
};

}  // namespace djssp
