#include "InstanceGenerator.h"

namespace djssp {

std::vector<Job> build_toy_jssp_jobs(int n_jobs, int n_machines, RNG& rng, double lambda) {
    std::vector<Job> jobs(n_jobs);
    double release_time = 0.0;

    for (int j = 0; j < n_jobs; ++j) {
        release_time += rng.exp_rv(lambda);
        jobs[j].id = j;
        jobs[j].release_time = release_time;

        std::vector<int> route(n_machines);
        std::iota(route.begin(), route.end(), 0);
        std::shuffle(route.begin(), route.end(), rng.eng);

        jobs[j].ops.reserve(n_machines);
        for (int k = 0; k < n_machines; ++k) {
            const double proc_time = 5.0 + 20.0 * rng.u01();
            jobs[j].ops.push_back(Operation{j, k, route[k], proc_time});
        }
    }

    return jobs;
}

std::vector<Job> build_toy_jobs_no_release(int n_jobs, int n_machines, RNG& rng) {
    std::vector<Job> jobs(n_jobs);

    for (int j = 0; j < n_jobs; ++j) {
        jobs[j].id = j;
        jobs[j].release_time = 0.0;

        std::vector<int> route(n_machines);
        std::iota(route.begin(), route.end(), 0);
        std::shuffle(route.begin(), route.end(), rng.eng);

        jobs[j].ops.reserve(n_machines);
        for (int k = 0; k < n_machines; ++k) {
            const double proc_time = 5.0 + 20.0 * rng.u01();
            jobs[j].ops.push_back(Operation{j, k, route[k], proc_time});
        }
    }

    return jobs;
}

std::vector<Machine> build_machines(int machine_count) {
    std::vector<Machine> machines(machine_count);
    for (int i = 0; i < machine_count; ++i) {
        machines[i].id = i;
    }
    return machines;
}

void assign_release_times_poisson(std::vector<Job>& jobs, RNG& rng, double lambda) {
    if (lambda <= 0.0) {
        for (auto& job : jobs) {
            job.release_time = 0.0;
        }
        return;
    }

    double t = 0.0;
    for (auto& job : jobs) {
        t += rng.exp_rv(lambda);
        job.release_time = t;
    }
}

bool load_orlib_jobshop1_instance(
    const std::string& filepath,
    const std::string& instance_name,
    std::vector<Job>& out_jobs,
    int& out_n_machines,
    std::string* err_msg
) {
    std::ifstream in(filepath);
    if (!in) {
        if (err_msg != nullptr) {
            *err_msg = "Cannot open file: " + filepath;
        }
        return false;
    }

    auto to_lower = [](std::string s) {
        for (auto& c : s) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return s;
    };

    const std::string target = to_lower(instance_name);
    std::string line;
    bool found = false;

    while (std::getline(in, line)) {
        bool all_ws = true;
        for (char c : line) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                all_ws = false;
                break;
            }
        }
        if (all_ws) {
            continue;
        }

        std::istringstream iss(line);
        std::string w1;
        std::string w2;
        iss >> w1 >> w2;
        if (to_lower(w1) == "instance" && to_lower(w2) == target) {
            found = true;
            break;
        }
    }

    if (!found) {
        if (err_msg != nullptr) {
            *err_msg = "Instance not found in file: " + instance_name;
        }
        return false;
    }

    int nJobs = 0;
    int nMach = 0;
    while (std::getline(in, line)) {
        bool all_ws = true;
        for (char c : line) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                all_ws = false;
                break;
            }
        }
        if (all_ws) {
            continue;
        }

        std::istringstream iss(line);
        if (iss >> nJobs >> nMach) {
            break;
        }
    }

    if (nJobs <= 0 || nMach <= 0) {
        if (err_msg != nullptr) {
            *err_msg = "Invalid (nJobs, nMachines) header for instance: " + instance_name;
        }
        return false;
    }

    std::vector<Job> jobs;
    jobs.reserve(nJobs);

    for (int j = 0; j < nJobs; ++j) {
        while (std::getline(in, line)) {
            bool all_ws = true;
            for (char c : line) {
                if (!std::isspace(static_cast<unsigned char>(c))) {
                    all_ws = false;
                    break;
                }
            }
            if (!all_ws) {
                break;
            }
        }

        if (!in) {
            if (err_msg != nullptr) {
                *err_msg = "Unexpected EOF while reading jobs for instance: " + instance_name;
            }
            return false;
        }

        std::istringstream iss(line);
        std::vector<int> values;
        values.reserve(2 * nMach);
        int value = 0;
        while (iss >> value) {
            values.push_back(value);
        }

        if (static_cast<int>(values.size()) != 2 * nMach) {
            if (err_msg != nullptr) {
                *err_msg = "Bad line for job " + std::to_string(j) +
                    " (expected " + std::to_string(2 * nMach) +
                    " integers, got " + std::to_string(values.size()) + ")";
            }
            return false;
        }

        Job job;
        job.id = j;
        job.release_time = 0.0;
        job.ops.reserve(nMach);

        for (int k = 0; k < nMach; ++k) {
            const int machine_id = values[2 * k];
            const int proc_time = values[2 * k + 1];

            if (machine_id < 0 || machine_id >= nMach) {
                if (err_msg != nullptr) {
                    *err_msg = "Machine id out of range in instance data.";
                }
                return false;
            }

            job.ops.push_back(Operation{j, k, machine_id, static_cast<double>(proc_time)});
        }

        jobs.push_back(std::move(job));
    }

    out_jobs = std::move(jobs);
    out_n_machines = nMach;
    return true;
}

}  // namespace djssp
