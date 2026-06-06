#pragma once

#include <filesystem>
#include <string>

namespace djssp {

struct InstanceRef {
    std::string source = "orlib";
    std::string name;
    std::filesystem::path path = "data/jobshop1.txt";
};

class InstanceCatalog {
public:
    explicit InstanceCatalog(std::filesystem::path default_orlib_path = "data/jobshop1.txt");

    InstanceRef resolve(const std::string& instance_name) const;

private:
    std::filesystem::path default_orlib_path_;
};

}  // namespace djssp
