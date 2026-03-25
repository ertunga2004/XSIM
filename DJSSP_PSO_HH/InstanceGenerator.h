#pragma once

#include "Job.h"
#include "Machine.h"

namespace djssp {

std::vector<Job> build_toy_jssp_jobs(int n_jobs, int n_machines, RNG& rng, double lambda);
std::vector<Job> build_toy_jobs_no_release(int n_jobs, int n_machines, RNG& rng);
std::vector<Machine> build_machines(int machine_count);
void assign_release_times_poisson(std::vector<Job>& jobs, RNG& rng, double lambda);

bool load_orlib_jobshop1_instance(
    const std::string& filepath,
    const std::string& instance_name,
    std::vector<Job>& out_jobs,
    int& out_n_machines,
    std::string* err_msg = nullptr
);

}  // namespace djssp
