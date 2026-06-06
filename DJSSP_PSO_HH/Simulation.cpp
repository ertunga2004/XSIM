#include "Simulation.h"

#include "Pso.h"

namespace djssp {

namespace {

void erase_ptr(std::deque<Operation*>& dq, Operation* op) {
    for (auto it = dq.begin(); it != dq.end(); ++it) {
        if (*it == op) {
            dq.erase(it);
            return;
        }
    }
}

struct SchEval {
    double Cmax = 0.0;
    std::vector<double> start;
    std::vector<int> pred;
    int last_node = -1;
    bool acyclic = true;
};

SchEval eval_machine_order_schedule(
    int J,
    int M,
    const std::vector<int>& machine_of,
    const std::vector<double>& proc,
    const std::vector<std::vector<int>>& order_by_m
) {
    const int N = J * M;
    std::vector<std::vector<int>> succ(N);
    std::vector<int> indeg(N, 0);

    auto add_edge = [&](int u, int v) {
        succ[u].push_back(v);
        indeg[v]++;
    };

    for (int j = 0; j < J; ++j) {
        for (int k = 0; k < M - 1; ++k) {
            add_edge(j * M + k, j * M + (k + 1));
        }
    }

    for (int m = 0; m < M; ++m) {
        const auto& order = order_by_m[m];
        for (int i = 0; i < static_cast<int>(order.size()) - 1; ++i) {
            add_edge(order[i], order[i + 1]);
        }
    }

    std::deque<int> queue;
    for (int i = 0; i < N; ++i) {
        if (indeg[i] == 0) {
            queue.push_back(i);
        }
    }

    std::vector<double> dist(N, 0.0);
    std::vector<int> pred(N, -1);
    int seen = 0;

    while (!queue.empty()) {
        const int u = queue.front();
        queue.pop_front();
        seen++;

        const double u_end = dist[u] + proc[u];
        for (int v : succ[u]) {
            if (dist[v] < u_end) {
                dist[v] = u_end;
                pred[v] = u;
            }
            indeg[v]--;
            if (indeg[v] == 0) {
                queue.push_back(v);
            }
        }
    }

    SchEval result;
    result.start = std::move(dist);
    result.pred = std::move(pred);

    if (seen != N) {
        result.acyclic = false;
        result.Cmax = 1e18;
        return result;
    }

    double cmax = 0.0;
    int last = -1;
    for (int u = 0; u < N; ++u) {
        const double end_time = result.start[u] + proc[u];
        if (end_time > cmax) {
            cmax = end_time;
            last = u;
        }
    }

    result.Cmax = cmax;
    result.last_node = last;
    return result;
}

std::vector<int> extract_critical_path_nodes(const SchEval& eval) {
    std::vector<int> path;
    int u = eval.last_node;
    while (u != -1) {
        path.push_back(u);
        u = eval.pred[u];
    }
    std::reverse(path.begin(), path.end());
    return path;
}

bool schedule_local_search_critical_blocks(
    int J,
    int M,
    const std::vector<int>& machine_of,
    const std::vector<double>& proc,
    std::vector<std::vector<int>>& order_by_m,
    int max_iters,
    double* out_best_Cmax,
    std::vector<double>* out_best_start
) {
    const int N = J * M;
    SchEval current = eval_machine_order_schedule(J, M, machine_of, proc, order_by_m);
    if (!current.acyclic) {
        return false;
    }

    double best = current.Cmax;

    for (int it = 0; it < max_iters; ++it) {
        const auto crit = extract_critical_path_nodes(current);
        if (crit.size() < 2) {
            break;
        }

        std::vector<std::vector<int>> pos(M, std::vector<int>(N, -1));
        for (int m = 0; m < M; ++m) {
            for (int i = 0; i < static_cast<int>(order_by_m[m].size()); ++i) {
                pos[m][order_by_m[m][i]] = i;
            }
        }

        struct Block {
            int m;
            int lpos;
            int rpos;
        };

        std::vector<Block> blocks;
        for (size_t i = 0; i < crit.size();) {
            const int u = crit[i];
            const int machine = machine_of[u];
            const int left = pos[machine][u];
            int right = left;

            size_t j = i + 1;
            while (j < crit.size()) {
                const int v = crit[j];
                if (machine_of[v] != machine) {
                    break;
                }
                const int position = pos[machine][v];
                if (position != right + 1) {
                    break;
                }
                right = position;
                j++;
            }

            if (right - left + 1 >= 2) {
                blocks.push_back(Block{machine, left, right});
            }
            i = j;
        }

        double best_neighbour = best;
        std::vector<std::vector<int>> best_order;
        SchEval best_eval;

        auto try_swap = [&](int machine, int index) {
            if (index < 0 || index + 1 >= static_cast<int>(order_by_m[machine].size())) {
                return;
            }

            auto candidate = order_by_m;
            std::swap(candidate[machine][index], candidate[machine][index + 1]);
            SchEval eval = eval_machine_order_schedule(J, M, machine_of, proc, candidate);
            if (eval.acyclic && eval.Cmax < best_neighbour) {
                best_neighbour = eval.Cmax;
                best_order = std::move(candidate);
                best_eval = std::move(eval);
            }
        };

        for (const auto& block : blocks) {
            for (int i = block.lpos; i <= block.rpos - 1; ++i) {
                try_swap(block.m, i);
            }
            try_swap(block.m, block.lpos - 1);
            try_swap(block.m, block.rpos);
        }

        if (best_neighbour + 1e-9 < best) {
            best = best_neighbour;
            order_by_m = std::move(best_order);
            current = std::move(best_eval);
        } else {
            break;
        }
    }

    if (out_best_Cmax != nullptr) {
        *out_best_Cmax = best;
    }
    if (out_best_start != nullptr) {
        *out_best_start = current.start;
    }
    return true;
}

bool schedule_tabu_search_critical_blocks(
    int J,
    int M,
    const std::vector<int>& machine_of,
    const std::vector<double>& proc,
    std::vector<std::vector<int>>& order_by_m,
    int ts_iters,
    int tabu_tenure,
    int tsmove_mode,
    double* out_best_Cmax,
    std::vector<double>* out_best_start
) {
    if (ts_iters <= 0) {
        return false;
    }

    const int N = J * M;
    SchEval current = eval_machine_order_schedule(J, M, machine_of, proc, order_by_m);
    if (!current.acyclic) {
        return false;
    }

    double best = current.Cmax;
    std::vector<std::vector<int>> best_order = order_by_m;
    std::vector<double> best_start = current.start;
    std::unordered_map<long long, int> tabu_until;

    auto key_swap = [&](int machine, int index) {
        return (1LL << 62) ^ (static_cast<long long>(machine) << 32) ^
            static_cast<unsigned long long>(index);
    };

    auto key_insert = [&](int machine, int from, int to) {
        return (2LL << 62) ^ (static_cast<long long>(machine) << 40) ^
            (static_cast<long long>(from) << 20) ^
            static_cast<unsigned long long>(to);
    };

    for (int it = 0; it < ts_iters; ++it) {
        const auto crit = extract_critical_path_nodes(current);
        if (crit.size() < 2) {
            break;
        }

        std::vector<std::vector<int>> pos(M, std::vector<int>(N, -1));
        for (int m = 0; m < M; ++m) {
            for (int i = 0; i < static_cast<int>(order_by_m[m].size()); ++i) {
                pos[m][order_by_m[m][i]] = i;
            }
        }

        struct Block {
            int m;
            int lpos;
            int rpos;
        };

        std::vector<Block> blocks;
        for (size_t i = 0; i < crit.size();) {
            const int u = crit[i];
            const int machine = machine_of[u];
            const int left = pos[machine][u];
            int right = left;

            size_t j = i + 1;
            while (j < crit.size()) {
                const int v = crit[j];
                if (machine_of[v] != machine) {
                    break;
                }
                const int position = pos[machine][v];
                if (position != right + 1) {
                    break;
                }
                right = position;
                j++;
            }

            if (right - left + 1 >= 2) {
                blocks.push_back(Block{machine, left, right});
            }
            i = j;
        }

        if (blocks.empty()) {
            for (int u : crit) {
                const int machine = machine_of[u];
                const int position = pos[machine][u];
                if (position >= 0 && position + 1 < static_cast<int>(order_by_m[machine].size())) {
                    blocks.push_back(Block{machine, position, position + 1});
                }
            }
        }

        double best_candidate_cmax = 1e300;
        SchEval best_candidate_eval;
        int best_candidate_type = -1;
        int best_candidate_machine = -1;
        int best_candidate_i = -1;
        int best_candidate_j = -1;
        bool found = false;

        auto consider_swap = [&](int machine, int index) {
            if (index < 0 || index + 1 >= static_cast<int>(order_by_m[machine].size())) {
                return;
            }

            const long long key = key_swap(machine, index);
            const auto tabu_it = tabu_until.find(key);
            const bool is_tabu = tabu_it != tabu_until.end() && tabu_it->second > it;

            std::swap(order_by_m[machine][index], order_by_m[machine][index + 1]);
            SchEval eval = eval_machine_order_schedule(J, M, machine_of, proc, order_by_m);
            std::swap(order_by_m[machine][index], order_by_m[machine][index + 1]);

            if (!eval.acyclic) {
                return;
            }
            if (is_tabu && !(eval.Cmax + 1e-9 < best)) {
                return;
            }

            if (eval.Cmax < best_candidate_cmax) {
                best_candidate_cmax = eval.Cmax;
                best_candidate_eval = std::move(eval);
                best_candidate_type = 0;
                best_candidate_machine = machine;
                best_candidate_i = index;
                best_candidate_j = -1;
                found = true;
            }
        };

        auto consider_insert = [&](int machine, int from, int to) {
            if (from < 0 || from >= static_cast<int>(order_by_m[machine].size())) {
                return;
            }
            if (to < 0) {
                to = 0;
            }
            if (to > static_cast<int>(order_by_m[machine].size())) {
                to = static_cast<int>(order_by_m[machine].size());
            }
            if (to == from || to == from + 1) {
                return;
            }

            const long long key = key_insert(machine, from, to);
            const auto tabu_it = tabu_until.find(key);
            const bool is_tabu = tabu_it != tabu_until.end() && tabu_it->second > it;

            auto& order = order_by_m[machine];
            const int node = order[from];
            order.erase(order.begin() + from);
            int adjusted_to = to;
            if (from < to) {
                adjusted_to = to - 1;
            }
            order.insert(order.begin() + adjusted_to, node);

            SchEval eval = eval_machine_order_schedule(J, M, machine_of, proc, order_by_m);

            order.erase(order.begin() + adjusted_to);
            order.insert(order.begin() + from, node);

            if (!eval.acyclic) {
                return;
            }
            if (is_tabu && !(eval.Cmax + 1e-9 < best)) {
                return;
            }

            if (eval.Cmax < best_candidate_cmax) {
                best_candidate_cmax = eval.Cmax;
                best_candidate_eval = std::move(eval);
                best_candidate_type = 1;
                best_candidate_machine = machine;
                best_candidate_i = from;
                best_candidate_j = to;
                found = true;
            }
        };

        for (const auto& block : blocks) {
            if (tsmove_mode == 0 || tsmove_mode == 2) {
                for (int i = block.lpos; i <= block.rpos - 1; ++i) {
                    consider_swap(block.m, i);
                }
                consider_swap(block.m, block.lpos - 1);
                consider_swap(block.m, block.rpos);
            }

            if (tsmove_mode == 1 || tsmove_mode == 2) {
                consider_insert(block.m, block.lpos, block.rpos + 1);
                consider_insert(block.m, block.rpos, block.lpos);
                consider_insert(block.m, block.lpos, block.lpos - 1);
                consider_insert(block.m, block.rpos, block.rpos + 2);
            }
        }

        if (!found) {
            break;
        }

        if (best_candidate_type == 0) {
            std::swap(
                order_by_m[best_candidate_machine][best_candidate_i],
                order_by_m[best_candidate_machine][best_candidate_i + 1]
            );
            current = std::move(best_candidate_eval);
            tabu_until[key_swap(best_candidate_machine, best_candidate_i)] =
                it + std::max(1, tabu_tenure);
        } else {
            auto& order = order_by_m[best_candidate_machine];
            int from = best_candidate_i;
            int to = best_candidate_j;
            if (to < 0) {
                to = 0;
            }
            if (to > static_cast<int>(order.size())) {
                to = static_cast<int>(order.size());
            }

            const int node = order[from];
            order.erase(order.begin() + from);
            int adjusted_to = to;
            if (from < to) {
                adjusted_to = to - 1;
            }
            order.insert(order.begin() + adjusted_to, node);

            current = std::move(best_candidate_eval);
            tabu_until[key_insert(best_candidate_machine, from, to)] =
                it + std::max(1, tabu_tenure);
        }

        if (current.Cmax + 1e-9 < best) {
            best = current.Cmax;
            best_order = order_by_m;
            best_start = current.start;
        }
    }

    order_by_m = std::move(best_order);
    if (out_best_Cmax != nullptr) {
        *out_best_Cmax = best;
    }
    if (out_best_start != nullptr) {
        *out_best_start = std::move(best_start);
    }
    return true;
}

}  // namespace

GanttLogger::GanttLogger(std::string output_path) : path(std::move(output_path)) {}

void GanttLogger::logOp(
    int machine_id,
    int job_id,
    int op_index,
    double start,
    double end,
    double proc_time
) {
    rows.push_back(Entry{machine_id, job_id, op_index, start, end, proc_time});
}

void GanttLogger::write_sorted() const {
    std::ofstream out(path);
    out << "machine_id,job_id,op_index,start,end,proc_time\n";

    std::vector<Entry> sorted = rows;
    std::sort(sorted.begin(), sorted.end(), [](const Entry& a, const Entry& b) {
        if (a.machine_id != b.machine_id) {
            return a.machine_id < b.machine_id;
        }
        if (a.start != b.start) {
            return a.start < b.start;
        }
        if (a.end != b.end) {
            return a.end < b.end;
        }
        if (a.job_id != b.job_id) {
            return a.job_id < b.job_id;
        }
        return a.op_index < b.op_index;
    });

    out.setf(std::ios::fixed);
    out.precision(0);

    for (const auto& row : sorted) {
        out << row.machine_id << "," << row.job_id << "," << row.op_index << ","
            << row.start << "," << row.end << "," << row.proc_time << "\n";
    }
}

Simulation::Simulation(
    std::vector<Job> jobs,
    std::vector<Machine> machines,
    std::vector<IRule*> rules,
    StateFeatures feats,
    GanttLogger* logger,
    bool use_gt
) : jobs_(std::move(jobs)),
    machines_(std::move(machines)),
    rules_(std::move(rules)),
    feats_(std::move(feats)),
    logger_(logger),
    use_gt_(use_gt) {}

void Simulation::reset_runtime() {
    for (auto& machine : machines_) {
        machine.busy_until = 0.0;
        machine.running = nullptr;
        machine.ready_q.clear();
    }

    for (auto& job : jobs_) {
        job.next_op = 0;
        for (auto& op : job.ops) {
            op.done = false;
            op.start_time = std::numeric_limits<double>::quiet_NaN();
            op.end_time = std::numeric_limits<double>::quiet_NaN();
        }
    }
}

void Simulation::on_arrival(JobId jid) {
    Job& job = jobs_.at(jid);
    if (!job.completed()) {
        Operation& op = job.ops.at(job.next_op);
        machines_.at(op.machine_id).ready_q.push_back(&op);
    }
}

void Simulation::on_complete(MachineId mid, Operation* op, double t, double& Cmax) {
    Machine& machine = machines_.at(mid);
    op->done = true;
    op->end_time = t;
    Cmax = std::max(Cmax, t);

    if (logger_ != nullptr) {
        logger_->logOp(mid, op->job_id, op->op_index, op->start_time, op->end_time, op->proc_time);
    }

    machine.running = nullptr;
    machine.busy_until = t;

    Job& job = jobs_.at(op->job_id);
    job.next_op++;
    if (!job.completed()) {
        Operation& next_op = job.ops.at(job.next_op);
        machines_.at(next_op.machine_id).ready_q.push_back(&next_op);
    }
}

void Simulation::dispatch_machine(
    EventQueue& eq,
    uint64_t& seq,
    double t,
    Machine& machine,
    const std::vector<double>& policy_w,
    const PSOHH& hh,
    double current_eps,
    double& Cmax
) {
    (void)Cmax;
    const StateVector state = feats_.extract(machines_, jobs_, machine, t);
    const int rule_index = hh.select_rule(policy_w, state, seq, current_eps);
    Operation* chosen = rules_.at(rule_index)->select(machine, jobs_, t);
    if (chosen == nullptr) {
        return;
    }

    erase_ptr(machine.ready_q, chosen);
    chosen->start_time = t;
    machine.running = chosen;
    machine.busy_until = t + chosen->proc_time;
    eq.push(Event{machine.busy_until, EventType::OP_COMPLETE, chosen->job_id, machine.id, chosen, seq++});
}

void Simulation::dispatch_all(
    EventQueue& eq,
    uint64_t& seq,
    double t,
    const std::vector<double>& policy_w,
    const PSOHH& hh,
    double current_eps,
    double& Cmax
) {
    bool progressed = true;
    while (progressed) {
        progressed = false;
        for (auto& machine : machines_) {
            if (machine.isIdle(t) && !machine.ready_q.empty()) {
                dispatch_machine(eq, seq, t, machine, policy_w, hh, current_eps, Cmax);
                progressed = true;
            }
        }
    }
}

SimResult Simulation::run_episode_gt(
    const std::vector<double>& policy_w,
    const PSOHH& hh,
    uint64_t seed_seq_base,
    double current_eps
) {
    reset_runtime();
    uint64_t seq = seed_seq_base;

    const int J = static_cast<int>(jobs_.size());
    const int M = static_cast<int>(machines_.size());
    std::vector<double> job_ready(J, 0.0);
    std::vector<double> machine_ready(M, 0.0);

    for (int j = 0; j < J; ++j) {
        job_ready[j] = jobs_[j].release_time;
    }

    int done_ops = 0;
    int total_ops = 0;
    for (const auto& job : jobs_) {
        total_ops += static_cast<int>(job.ops.size());
    }

    double Cmax = 0.0;

    auto rebuild_ready_queues = [&](double t) {
        for (auto& machine : machines_) {
            machine.ready_q.clear();
        }

        for (int j = 0; j < J; ++j) {
            Job& job = jobs_[j];
            if (job.completed()) {
                continue;
            }

            Operation& op = job.ops[job.next_op];
            if (t >= job.release_time && job_ready[j] <= t + 1e-12) {
                machines_[op.machine_id].ready_q.push_back(&op);
            }
        }
    };

    while (done_ops < total_ops) {
        double best_completion = 1e300;
        Operation* best_op = nullptr;
        double best_est = 0.0;
        int best_machine = -1;

        for (int j = 0; j < J; ++j) {
            Job& job = jobs_[j];
            if (job.completed()) {
                continue;
            }

            Operation& op = job.ops[job.next_op];
            double est = std::max(job_ready[j], machine_ready[op.machine_id]);
            est = std::max(est, job.release_time);
            const double completion = est + op.proc_time;

            uint64_t x = seq ^ (static_cast<uint64_t>(j) * 0x9e3779b97f4a7c15ULL);
            const double noise =
                static_cast<double>(static_cast<int64_t>(splitmix64(x))) * (1.0 / 9.22e18) * 1e-15;

            if (completion + noise < best_completion) {
                best_completion = completion + noise;
                best_op = &op;
                best_est = est;
                best_machine = op.machine_id;
            }
        }

        if (best_op == nullptr || best_machine < 0) {
            break;
        }

        double decision_time = best_est;
        rebuild_ready_queues(decision_time);

        Machine& focus_machine = machines_[best_machine];
        if (focus_machine.ready_q.empty()) {
            double next_time = 1e300;
            for (int j = 0; j < J; ++j) {
                if (jobs_[j].completed()) {
                    continue;
                }

                Operation& op = jobs_[j].ops[jobs_[j].next_op];
                double est = std::max(job_ready[j], machine_ready[op.machine_id]);
                est = std::max(est, jobs_[j].release_time);
                next_time = std::min(next_time, est);
            }

            if (next_time >= 1e299) {
                break;
            }
            rebuild_ready_queues(next_time);
            decision_time = next_time;
        }

        std::deque<Operation*> conflict;
        const double C_star = best_est + best_op->proc_time;
        for (auto* op : focus_machine.ready_q) {
            const int j = op->job_id;
            double est = std::max(job_ready[j], machine_ready[best_machine]);
            est = std::max(est, jobs_[j].release_time);
            if (est < C_star - 1e-12) {
                conflict.push_back(op);
            }
        }

        if (conflict.empty()) {
            conflict.push_back(best_op);
        }

        std::deque<Operation*> saved_queue = focus_machine.ready_q;
        focus_machine.ready_q = conflict;
        const StateVector state = feats_.extract(machines_, jobs_, focus_machine, decision_time);
        const int rule_index = hh.select_rule(policy_w, state, seq, current_eps);
        Operation* chosen = rules_[rule_index]->select(focus_machine, jobs_, decision_time);
        if (chosen == nullptr) {
            chosen = conflict.front();
        }
        focus_machine.ready_q = saved_queue;

        const int j = chosen->job_id;
        double start = std::max(job_ready[j], machine_ready[best_machine]);
        start = std::max(start, jobs_[j].release_time);
        const double end = start + chosen->proc_time;

        chosen->start_time = start;
        chosen->end_time = end;
        chosen->done = true;

        if (logger_ != nullptr) {
            logger_->logOp(best_machine, chosen->job_id, chosen->op_index, start, end, chosen->proc_time);
        }

        machine_ready[best_machine] = end;
        job_ready[j] = end;
        jobs_[j].next_op++;
        done_ops++;
        Cmax = std::max(Cmax, end);
        seq++;
    }

    return SimResult{Cmax};
}

SimResult Simulation::run_episode(
    const std::vector<double>& policy_w,
    const PSOHH& hh,
    uint64_t seed_seq_base,
    double current_eps
) {
    if (use_gt_) {
        return run_episode_gt(policy_w, hh, seed_seq_base, current_eps);
    }

    reset_runtime();
    EventQueue eq;
    uint64_t seq = seed_seq_base;

    for (const auto& job : jobs_) {
        eq.push(Event{job.release_time, EventType::JOB_ARRIVAL, job.id, -1, nullptr, seq++});
    }

    double t = 0.0;
    double Cmax = 0.0;

    while (!eq.empty()) {
        const Event event = eq.pop();
        t = event.time;

        if (event.type == EventType::JOB_ARRIVAL) {
            on_arrival(event.job_id);
            dispatch_all(eq, seq, t, policy_w, hh, current_eps, Cmax);
        } else {
            on_complete(event.machine_id, event.op, t, Cmax);
            dispatch_all(eq, seq, t, policy_w, hh, current_eps, Cmax);
        }
    }

    return SimResult{Cmax};
}

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
) {
    if (sls_iters <= 0 && ts_iters <= 0) {
        return false;
    }

    const int N = J * M;
    std::vector<int> machine_of(N, -1);
    std::vector<double> proc(N, 0.0);
    for (int j = 0; j < J; ++j) {
        for (int k = 0; k < M; ++k) {
            const int id = j * M + k;
            machine_of[id] = jobs_model[j].ops[k].machine_id;
            proc[id] = jobs_model[j].ops[k].proc_time;
        }
    }

    std::vector<std::vector<std::pair<double, int>>> tmp(M);
    for (const auto& entry : rows) {
        const int node = entry.job_id * M + entry.op_index;
        if (entry.machine_id < 0 || entry.machine_id >= M) {
            continue;
        }
        tmp[entry.machine_id].push_back({entry.start, node});
    }

    std::vector<std::vector<int>> order_by_m(M);
    for (int m = 0; m < M; ++m) {
        auto& order = tmp[m];
        std::sort(order.begin(), order.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        order_by_m[m].reserve(order.size());
        for (const auto& pair : order) {
            order_by_m[m].push_back(pair.second);
        }

        if (static_cast<int>(order_by_m[m].size()) != J) {
            return false;
        }
    }

    double bestC = 0.0;
    std::vector<double> best_start;
    bool ok = false;

    if (ts_iters > 0) {
        ok = schedule_tabu_search_critical_blocks(
            J,
            M,
            machine_of,
            proc,
            order_by_m,
            ts_iters,
            tabu_tenure,
            tsmove_mode,
            &bestC,
            &best_start
        );
    } else {
        ok = schedule_local_search_critical_blocks(
            J,
            M,
            machine_of,
            proc,
            order_by_m,
            sls_iters,
            &bestC,
            &best_start
        );
    }

    if (!ok) {
        return false;
    }

    std::vector<GanttLogger::Entry> new_rows;
    new_rows.reserve(rows.size());
    for (int j = 0; j < J; ++j) {
        for (int k = 0; k < M; ++k) {
            const int id = j * M + k;
            const int machine = machine_of[id];
            const double start = best_start[id];
            new_rows.push_back(GanttLogger::Entry{
                machine,
                j,
                k,
                start,
                start + proc[id],
                proc[id]
            });
        }
    }

    rows.swap(new_rows);
    if (out_new_Cmax != nullptr) {
        *out_new_Cmax = bestC;
    }
    return true;
}

}  // namespace djssp
