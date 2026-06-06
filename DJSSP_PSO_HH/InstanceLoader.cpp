#include "InstanceLoader.h"

namespace djssp {

bool InstanceLoader::load_orlib_file(
    const std::filesystem::path& path,
    const std::string& instance_name,
    Instance& out_instance,
    std::string* error_message
) const {
    std::vector<Job> jobs;
    int machine_count = 0;
    std::string load_error;

    if (!load_orlib_jobshop1_instance(path.string(), instance_name, jobs, machine_count, &load_error)) {
        if (error_message != nullptr) {
            *error_message = load_error;
        }
        return false;
    }

    out_instance.name = instance_name;
    out_instance.source = "orlib";
    out_instance.path = path;
    out_instance.jobs = std::move(jobs);
    out_instance.machine_count = machine_count;
    return true;
}

}  // namespace djssp
