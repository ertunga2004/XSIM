#pragma once

#include "Event.h"
<<<<<<< HEAD
=======
#include "IRule.h"
>>>>>>> e2a2af7 (Checkpoint: P0-P3 refactor completed)
#include "StateFeatures.h"

namespace djssp {

class PSOHH;

struct SimResult {
    double Cmax = 0.0;
};

class GanttLogger {
public:
    struct Entry {
        int machine_id;
        int job_id;
        int op_index;
        double start;
        double end;
        double proc_time;
    };

    explicit GanttLogger(std::string output_path);

    void logOp(int machine_id, int job_id, int op_index, double start, double end, double proc_time);
    void write_sorted() const;

    std::string path;
    std::vector<Entry> rows;
};

<<<<<<< HEAD
class Rule {
public:
    virtual ~Rule() = default;
    virtual std::string name() const = 0;
    virtual Operation* select(const Machine& machine, const std::vector<Job>& jobs, double t) const = 0;
};

std::vector<std::string> build_rule_names();
std::unique_ptr<Rule> create_rule_by_name(const std::string& rule_name);
std::vector<std::unique_ptr<Rule>> build_rules();

=======
>>>>>>> e2a2af7 (Checkpoint: P0-P3 refactor completed)
class Simulation {
public:
    Simulation(
        std::vector<Job> jobs,
        std::vector<Machine> machines,
<<<<<<< HEAD
        std::vector<std::unique_ptr<Rule>> rules,
=======
        std::vector<IRule*> rules,
>>>>>>> e2a2af7 (Checkpoint: P0-P3 refactor completed)
        StateFeatures feats,
        GanttLogger* logger,
        bool use_gt
    );

    SimResult run_episode(
        const std::vector<double>& policy_w,
        const PSOHH& hh,
        uint64_t seed_seq_base,
        double current_eps
    );

private:
    void reset_runtime();
    void on_arrival(JobId jid);
    void on_complete(MachineId mid, Operation* op, double t, double& Cmax);
    void dispatch_machine(
        EventQueue& eq,
        uint64_t& seq,
        double t,
        Machine& machine,
        const std::vector<double>& policy_w,
        const PSOHH& hh,
        double current_eps,
        double& Cmax
    );
    void dispatch_all(
        EventQueue& eq,
        uint64_t& seq,
        double t,
        const std::vector<double>& policy_w,
        const PSOHH& hh,
        double current_eps,
        double& Cmax
    );
    SimResult run_episode_gt(
        const std::vector<double>& policy_w,
        const PSOHH& hh,
        uint64_t seed_seq_base,
        double current_eps
    );

    std::vector<Job> jobs_;
    std::vector<Machine> machines_;
<<<<<<< HEAD
    std::vector<std::unique_ptr<Rule>> rules_;
=======
    std::vector<IRule*> rules_;
>>>>>>> e2a2af7 (Checkpoint: P0-P3 refactor completed)
    StateFeatures feats_;
    GanttLogger* logger_;
    bool use_gt_;
};

bool improve_gantt_with_schedule_ls(
    const std::vector<Job>& jobs_model,
    int J,
    int M,
    std::vector<GanttLogger::Entry>& rows,
    int sls_iters,
    int ts_iters,
    int tabu_tenure,
    int tsmove_mode,
    double* out_new_Cmax
);

}  // namespace djssp
