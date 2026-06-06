#pragma once

#include "InstanceGenerator.h"

#include <filesystem>

namespace djssp {

struct Instance {
    std::string name;
    std::string source = "orlib";
    std::filesystem::path path = "data/jobshop1.txt";
    std::vector<Job> jobs;
    int machine_count = 0;
};

class InstanceLoader {
public:
    bool load_orlib_file(
        const std::filesystem::path& path,
        const std::string& instance_name,
        Instance& out_instance,
        std::string* error_message = nullptr
    ) const;
};

}  // namespace djssp
