#pragma once

#include "Config.h"

namespace djssp {

class ConfigLoader {
public:
    static bool load_file(
        const fs::path& path,
        XSimConfig& out_config,
        std::string* error_message = nullptr
    );

    static std::string to_resolved_json(const XSimConfig& config);
};

}  // namespace djssp
