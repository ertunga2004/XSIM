#include "InstanceCatalog.h"

#include <utility>

namespace djssp {

InstanceCatalog::InstanceCatalog(std::filesystem::path default_orlib_path)
    : default_orlib_path_(std::move(default_orlib_path)) {}

InstanceRef InstanceCatalog::resolve(const std::string& instance_name) const {
    InstanceRef ref;
    ref.source = "orlib";
    ref.name = instance_name;
    ref.path = default_orlib_path_;
    return ref;
}

}  // namespace djssp
