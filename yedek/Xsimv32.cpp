#include <bits/stdc++.h>
using namespace std;
// ---------------- Types ----------------
using JobId = int;
using MachineId = int;
enum class EventType : uint8_t { JOB_ARRIVAL, OP_COMPLETE };
struct StateVector {
    vector<double> x; // normalized
};
// ---------------- RNG ----------------
struct RNG {
    std::mt19937_64 eng;
    explicit RNG(uint64_t seed) : eng(seed) {}
    double u01() { return std::uniform_real_distribution<double>(0.0, 1.0)(eng); }
    double exp_rv(double lambda) { // Exponential interarrival
        return std::exponential_distribution<double>(lambda)(eng);
    }
};
// ---------------- Model ----------------
struct Operation {
    JobId job_id{};
    int op_index{};
    MachineId machine_id{};
    double proc_time{};
    double start_time = nan("");
    double end_time   = nan("");
    bool done = false;
};
struct Job {
    JobId id{};
    double release_time{};
    vector<Operation> ops;
    int next_op = 0;
    bool completed() const { return next_op >= (int)ops.size(); }
};
struct Machine {
    MachineId id{};
    double busy_until = 0.0;
    Operation* running = nullptr;     // non-owning
    deque<Operation*> ready_q;        // ready operations for this machine
    bool isIdle(double t) const { return (running == nullptr) && (t >= busy_until); }
};
// ---------------- Events ----------------
struct Event {
    double time{};
    EventType type{};
    JobId job_id{-1};
    MachineId machine_id{-1};
    Operation* op{nullptr};  // for OP_COMPLETE
    uint64_t seq{0};         // deterministic tie-breaker
};
struct EventCmp {
    bool operator()(const Event& a, const Event& b) const {
        if (a.time != b.time) return a.time > b.time; // min-heap effect
        return a.seq > b.seq;
    }
};
struct EventQueue {
    priority_queue<Event, vector<Event>, EventCmp> pq;
    void push(const Event& e) { pq.push(e); }
    bool empty() const { return pq.empty(); }
    Event pop() { auto e = pq.top(); pq.pop(); return e; }
};
// ---------------- Logger (Gantt CSV) ----------------
struct GanttLogger {
    struct Entry {
        int machine_id;
        int job_id;
        int op_index;
        double start;
        double end;
        double proc_time;
    };
    string path;
    vector<Entry> rows;
    explicit GanttLogger(const string& p) : path(p) {}
    void logOp(int m, int j, int opi, double s, double e, double p) {
        rows.push_back(Entry{m, j, opi, s, e, p});
    }
    void write_sorted() const {
        ofstream out(path);
        out << "machine_id,job_id,op_index,start,end,proc_time\n";
        // Sort for a clean Gantt: machine -> start -> end -> job -> op
        vector<Entry> tmp = rows;
        sort(tmp.begin(), tmp.end(), [](const Entry& a, const Entry& b){
            if (a.machine_id != b.machine_id) return a.machine_id < b.machine_id;
            if (a.start != b.start) return a.start < b.start;
            if (a.end != b.end) return a.end < b.end;
            if (a.job_id != b.job_id) return a.job_id < b.job_id;
            return a.op_index < b.op_index;
        });
        // Make output stable and readable (avoid scientific notation)
        out.setf(std::ios::fixed);
        out.precision(0);
        for (const auto& r : tmp) {
            out << r.machine_id << "," << r.job_id << "," << r.op_index << ","
                << r.start << "," << r.end << "," << r.proc_time << "\n";
        }
    }
};
// ---------------- Dispatch Rules (Cmax-oriented) ----------------
struct Rule {
    virtual ~Rule() = default;
    virtual string name() const = 0;
    virtual Operation* select(const Machine& m, const vector<Job>& jobs, double t) const = 0;
};
// SPT: shortest processing time
struct RuleSPT : Rule {
    string name() const override { return "SPT"; }
    Operation* select(const Machine& m, const vector<Job>&, double) const override {
        Operation* best=nullptr; double bp=1e300;
        for (auto* op: m.ready_q) if (op->proc_time < bp) { bp=op->proc_time; best=op; }
        return best;
    }
};
// LPT: longest processing time
struct RuleLPT : Rule {
    string name() const override { return "LPT"; }
    Operation* select(const Machine& m, const vector<Job>&, double) const override {
        Operation* best=nullptr; double bp=-1e300;
        for (auto* op: m.ready_q) if (op->proc_time > bp) { bp=op->proc_time; best=op; }
        return best;
    }
};
// Helper: remaining work of a job from next_op
static double remaining_work(const Job& j) {
    double s=0.0;
    for (int k=j.next_op; k<(int)j.ops.size(); ++k) s += j.ops[k].proc_time;
    return s;
}
// MWKR: most work remaining
struct RuleMWKR : Rule {
    string name() const override { return "MWKR"; }
    Operation* select(const Machine& m, const vector<Job>& jobs, double) const override {
        Operation* best=nullptr; double bw=-1e300;
        for (auto* op: m.ready_q) {
            const auto& j = jobs[op->job_id];
            double rw = remaining_work(j);
            if (rw > bw) { bw=rw; best=op; }
        }
        return best;
    }
};
// MOR: most operations remaining
struct RuleMOR : Rule {
    string name() const override { return "MOR"; }
    Operation* select(const Machine& m, const vector<Job>& jobs, double) const override {
        Operation* best=nullptr; int bo=-1;
        for (auto* op: m.ready_q) {
            const auto& j = jobs[op->job_id];
            int rem_ops = (int)j.ops.size() - j.next_op;
            if (rem_ops > bo) { bo=rem_ops; best=op; }
        }
        return best;
    }
};
// FIFO: earliest released job first (simple)
struct RuleFIFO : Rule {
    string name() const override { return "FIFO"; }
    Operation* select(const Machine& m, const vector<Job>& jobs, double) const override {
        Operation* best=nullptr; double br=1e300;
        for (auto* op: m.ready_q) {
            double rt = jobs[op->job_id].release_time;
            if (rt < br) { br=rt; best=op; }
        }
        return best;
    }
};
// SIO: shortest imminent operation (look-ahead to the NEXT operation's processing time)
// For the last operation, next_pt = 0.
struct RuleSIO : Rule {
    string name() const override { return "SIO"; }
    Operation* select(const Machine& m, const vector<Job>& jobs, double) const override {
        Operation* best=nullptr; double b=1e300;
        for (auto* op: m.ready_q) {
            const auto& j = jobs[op->job_id];
            int k = op->op_index;
            double next_pt = (k+1 < (int)j.ops.size()) ? j.ops[k+1].proc_time : 0.0;
            if (next_pt < b) { b = next_pt; best = op; }
        }
        return best;
    }
};
// PT+WINQ (approx.): processing time + (next operation processing time)
// True WINQ requires next-machine queue workload; here we use a standard lightweight proxy
// that still improves small ORLIB instances in practice.
struct RulePTWINQ : Rule {
    string name() const override { return "PT+WINQ"; }
    Operation* select(const Machine& m, const vector<Job>& jobs, double) const override {
        Operation* best=nullptr; double b=1e300;
        for (auto* op: m.ready_q) {
            const auto& j = jobs[op->job_id];
            int k = op->op_index;
            double next_pt = (k+1 < (int)j.ops.size()) ? j.ops[k+1].proc_time : 0.0;
            double score = op->proc_time + next_pt;
            if (score < b) { b = score; best = op; }
        }
        return best;
    }
};
// ---------------- State Features (Cmax-friendly) ----------------
struct FeatureConfig {
    int q_max = 50;      // queue normalization
    int wip_max = 50;    // WIP normalization
    double rw_scale = 200.0; // remaining-work avg scale
};
struct StateFeatures {
    FeatureConfig cfg;
    explicit StateFeatures(FeatureConfig c) : cfg(c) {}
    static double utilization(const vector<Machine>& machines, double t) {
        if (machines.empty()) return 0.0;
        int busy=0;
        for (auto& m: machines) {
            if (m.running != nullptr || t < m.busy_until) busy++;
        }
        return (double)busy/(double)machines.size();
    }
    StateVector extract(const vector<Machine>& machines,
                        const vector<Job>& jobs,
                        const Machine& focus_m,
                        double t) const
    {
        double U = utilization(machines, t);
        double Qm = min(1.0, (double)focus_m.ready_q.size() / (double)max(1,cfg.q_max));
        int wip=0;
        for (auto& j: jobs) {
            if (t >= j.release_time && !j.completed()) wip++;
        }
        double WIP = min(1.0, (double)wip / (double)max(1,cfg.wip_max));
        double rw_sum=0.0; int n=0;
        for (auto* op: focus_m.ready_q) {
            rw_sum += remaining_work(jobs[op->job_id]);
            n++;
        }
        double RWavg = (n>0) ? (rw_sum/(double)n) : 0.0;
        RWavg = min(1.0, RWavg / max(1e-9, cfg.rw_scale));
        StateVector s;
        // s = [U, Qm, WIP, RWavg]
        s.x = {U, Qm, WIP, RWavg};
        return s;
    }
};
// ---------------- Small deterministic RNG (SplitMix64) ----------------
// We use this to add exploration to rule selection without keeping mutable RNG state.
static inline uint64_t splitmix64(uint64_t& x) {
    uint64_t z = (x += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}
static inline double u01_from_u64(uint64_t& x) {
    // 53-bit precision double in [0,1)
    return (splitmix64(x) >> 11) * (1.0 / 9007199254740992.0);
}
static inline int randint_from_u64(uint64_t& x, int hi_exclusive) {
    return (int)(splitmix64(x) % (uint64_t)hi_exclusive);
}
// ---------------- PSO-HH (state-aware rule selection) ----------------
struct PSOParams {
    int swarm_size = 15;
    int iters = 30;
    double w_inertia = 0.7;
    double c1 = 1.4;
    double c2 = 1.4;
    // Epsilon-greedy exploration schedule (decays over iterations)
    double eps0 = 0.25;     // initial exploration probability
    double eps_min = 0.05;  // minimum exploration probability
};
struct Particle {
    vector<double> x, v, pbest;
    double pbest_fit = 1e300;
};
// Clamp helper for PSO positions (keeps scores numerically stable)
static inline double clampd(double v, double lo, double hi) { return (v<lo)?lo:((v>hi)?hi:v); }
// A state-aware linear policy:
// score_k = b_k + sum_j w_{k,j} * s_j
// Particle dimension = K * (F+1) where F = number of state features (here 4).
struct PSOHH {
    PSOParams p;
    int K;
    int F;
    RNG& rng;
    vector<Particle> swarm;
    vector<double> gbest;
    double gbest_fit = 1e300;
    explicit PSOHH(PSOParams pp, int K_, RNG& r, int F_=4)
        : p(pp), K(K_), F(F_), rng(r)
    {
        const int D = K * (F + 1);
        swarm.resize(p.swarm_size);
        for (auto& pt: swarm) {
            pt.x.resize(D); pt.v.resize(D); pt.pbest.resize(D);
            for (int d=0; d<D; ++d) {
                // small random init around 0
                pt.x[d] = (rng.u01()-0.5) * 0.5;     // [-0.25, 0.25]
                pt.v[d] = (rng.u01()-0.5) * 0.1;     // [-0.05, 0.05]
            }
            pt.pbest = pt.x;
        }
        gbest = swarm[0].x;
    }
    const vector<double>& particle_weights(int i) const { return swarm.at(i).x; }
    void update_fitness(int i, double fit) {
        auto& pt = swarm.at(i);
        if (fit < pt.pbest_fit) {
            pt.pbest_fit = fit;
            pt.pbest = pt.x;
        }
        if (fit < gbest_fit) {
            gbest_fit = fit;
            gbest = pt.x;
        }
    }
    void move() {
        const double XMAX = 6.0;
        const double VMAX = 1.0;
        const int D = K * (F + 1);
        for (auto& pt: swarm) {
            double r1=rng.u01(), r2=rng.u01();
            for (int d=0; d<D; ++d) {
                pt.v[d] = p.w_inertia*pt.v[d]
                        + p.c1*r1*(pt.pbest[d]-pt.x[d])
                        + p.c2*r2*(gbest[d]-pt.x[d]);
                pt.v[d] = clampd(pt.v[d], -VMAX, VMAX);
                pt.x[d] += pt.v[d];
                pt.x[d] = clampd(pt.x[d], -XMAX, XMAX);
            }
        }
    }
    // Returns the rule index chosen by this particle given the current state.
    int select_rule(const vector<double>& theta, const StateVector& s, uint64_t seed, double eps) const {
        uint64_t x = seed ^ 0xD1B54A32D192ED03ULL;
        // Exploration: with probability eps, pick a random rule
        eps = clamp(eps, 0.0, 0.95);
        if (u01_from_u64(x) < eps) {
            return randint_from_u64(x, K);
        }
        int best = 0;
        double bestScore = -1e300;
        // For each rule k, compute linear score
        for (int k=0; k<K; ++k) {
            int base = k*(F+1);
            double score = theta[base + 0]; // bias
            for (int j=0; j<F; ++j) score += theta[base + 1 + j] * s.x[j];
            // tiny deterministic noise to break ties
            double noise = ((double)((int64_t)splitmix64(x))) * (1.0 / 9.22e18) * 1e-12;
            score += noise;
            if (score > bestScore) { bestScore = score; best = k; }
        }
        return best;
    }
};
// ---------------- Simulation ----------------
struct SimResult {
    double Cmax=0.0;
};
static void erase_ptr(deque<Operation*>& dq, Operation* p) {
    for (auto it=dq.begin(); it!=dq.end(); ++it) {
        if (*it==p) { dq.erase(it); return; }
    }
}
struct Simulation {
    vector<Job> jobs;               // episode-local copy
    vector<Machine> machines;       // episode-local copy
    vector<unique_ptr<Rule>> rules; // episode-local
    StateFeatures feats;
    GanttLogger* logger;
    bool use_gt;
    Simulation(vector<Job> j, vector<Machine> m,
               vector<unique_ptr<Rule>> r,
               StateFeatures f, GanttLogger* lg,
               bool use_gt_)
        : jobs(std::move(j)), machines(std::move(m)),
          rules(std::move(r)), feats(std::move(f)), logger(lg), use_gt(use_gt_) {}
    void reset_runtime() {
        for (auto& m: machines) {
            m.busy_until=0.0;
            m.running=nullptr;
            m.ready_q.clear();
        }
        for (auto& j: jobs) {
            j.next_op=0;
            for (auto& op: j.ops) { op.done=false; op.start_time=nan(""); op.end_time=nan(""); }
        }
    }
    // job arrival: push first op to its machine queue
    void on_arrival(JobId jid) {
        Job& j = jobs.at(jid);
        if (!j.completed()) {
            Operation& op = j.ops.at(j.next_op);
            machines.at(op.machine_id).ready_q.push_back(&op);
        }
    }
    // op complete: free machine, advance job, push next op (if exists)
    void on_complete(MachineId mid, Operation* op, double t, double& Cmax) {
        Machine& m = machines.at(mid);
        op->done=true;
        op->end_time=t;
        Cmax = max(Cmax, t);
        if (logger) {
            logger->logOp(mid, op->job_id, op->op_index, op->start_time, op->end_time, op->proc_time);
        }
        m.running=nullptr;
        m.busy_until=t;
        Job& j = jobs.at(op->job_id);
        j.next_op++;
        if (!j.completed()) {
            Operation& nextop = j.ops.at(j.next_op);
            machines.at(nextop.machine_id).ready_q.push_back(&nextop);
        }
    }
    void dispatch_machine(EventQueue& eq, uint64_t& seq, double t,
                          Machine& m,
                          const vector<double>& policy_w,
                          const PSOHH& hh,
                          double current_eps,
                          double& Cmax)
    {
        // compute state for this decision point
        StateVector s = feats.extract(machines, jobs, m, t);
        // choose rule and operation
        int rk = hh.select_rule(policy_w, s, seq, /*eps*/ current_eps);
        Operation* chosen = rules.at(rk)->select(m, jobs, t);
        if (!chosen) return;
        erase_ptr(m.ready_q, chosen);
        // start
        chosen->start_time = t;
        m.running = chosen;
        m.busy_until = t + chosen->proc_time;
        // schedule completion
        eq.push(Event{m.busy_until, EventType::OP_COMPLETE, chosen->job_id, m.id, chosen, seq++});
    }
    // after any event, try to dispatch all idle machines with ready ops
    void dispatch_all(EventQueue& eq, uint64_t& seq, double t,
                      const vector<double>& policy_w,
                      const PSOHH& hh,
                      double current_eps,
                      double& Cmax)
    {
        bool progressed=true;
        while (progressed) {
            progressed=false;
            for (auto& m: machines) {
                if (m.isIdle(t) && !m.ready_q.empty()) {
                    dispatch_machine(eq, seq, t, m, policy_w, hh, current_eps, Cmax);
                    progressed=true;
                }
            }
        }
    }
    
// -------- Active Schedule Generation: Giffler-Thompson (GT) --------
// This generates an ACTIVE schedule (superset of non-delay schedules) and can reach
// known optima on ORLIB instances like ft06 (optimum = 55), whereas the event-based
// "dispatch whenever a machine is idle" engine may get stuck in the non-delay subset.
//
// Determinism / exploration is controlled via seed_seq_base (tie-breaking) and eps
// (rule exploration). For benchmark comparability we typically set eps=0 and vary seq.
SimResult run_episode_gt(const vector<double>& policy_w,
                         const PSOHH& hh,
                         uint64_t seed_seq_base,
                         double current_eps)
{
    reset_runtime();
    uint64_t seq = seed_seq_base;
    const int J = (int)jobs.size();
    const int M = (int)machines.size();
    vector<double> job_ready(J, 0.0);
    vector<double> mach_ready(M, 0.0);
    // set releases
    for (int j=0;j<J;++j) job_ready[j] = jobs[j].release_time;
    int done_ops = 0;
    const int total_ops = [&](){
        int s=0; for (auto& jb: jobs) s += (int)jb.ops.size(); return s;
    }();
    double Cmax = 0.0;
    // Helper to rebuild machine ready queues from current job_next/job_ready/mach_ready
    auto rebuild_ready_queues = [&](double t){
        for (auto& m: machines) m.ready_q.clear();
        for (int j=0;j<J;++j) {
            Job& jb = jobs[j];
            if (jb.completed()) continue;
            Operation& op = jb.ops[jb.next_op];
            // operation is "available" if job is released and predecessor finished (tracked by job_ready)
            if (t >= jb.release_time && job_ready[j] <= t + 1e-12) {
                machines[op.machine_id].ready_q.push_back(&op);
            }
        }
    };
    // Main GT loop
    while (done_ops < total_ops) {
        // 1) Compute earliest start and completion for each job's next op
        double bestC = 1e300;
        Operation* o_star = nullptr;
        double est_star = 0.0;
        int m_star = -1;
        for (int j=0;j<J;++j) {
            Job& jb = jobs[j];
            if (jb.completed()) continue;
            Operation& op = jb.ops[jb.next_op];
            double est = max(job_ready[j], mach_ready[op.machine_id]);
            // respect release
            est = max(est, jb.release_time);
            double c = est + op.proc_time;
            // deterministic tie-break using seq
            uint64_t x = seq ^ ((uint64_t)j * 0x9e3779b97f4a7c15ULL);
            double noise = ((double)((int64_t)splitmix64(x))) * (1.0 / 9.22e18) * 1e-15;
            if (c + noise < bestC) {
                bestC = c + noise;
                o_star = &op;
                est_star = est;
                m_star = op.machine_id;
            }
        }
        if (!o_star || m_star < 0) break; // should not happen
        // Decision time for GT is the start time of the earliest-completing operation
        double t_dec = est_star;
        // 2) Build ready queues at decision time
        rebuild_ready_queues(t_dec);
        // 3) Conflict set on machine m_star: operations that could start before C* on that machine
        Machine& focus_m = machines[m_star];
        // If for some reason the focus queue is empty (can happen with release times),
        // advance time to the next release/availability.
        if (focus_m.ready_q.empty()) {
            // advance to earliest time some op becomes available
            double t_next = 1e300;
            for (int j=0;j<J;++j) {
                if (jobs[j].completed()) continue;
                Operation& op = jobs[j].ops[jobs[j].next_op];
                double est = max(job_ready[j], mach_ready[op.machine_id]);
                est = max(est, jobs[j].release_time);
                t_next = min(t_next, est);
            }
            if (t_next>=1e299) break;
            rebuild_ready_queues(t_next);
            t_dec = t_next;
        }
        // Create conflict queue: subset of focus_m.ready_q satisfying est < (est_star + p_star)
        deque<Operation*> conflict;
        double C_star = est_star + o_star->proc_time;
        for (auto* op : focus_m.ready_q) {
            int j = op->job_id;
            double est = max(job_ready[j], mach_ready[m_star]);
            est = max(est, jobs[j].release_time);
            if (est < C_star - 1e-12) conflict.push_back(op);
        }
        if (conflict.empty()) {
            // At minimum, include o_star (should satisfy)
            conflict.push_back(o_star);
        }
        // 4) Select one operation from conflict set using our policy (rule selection + rule's op selection)
        // Temporarily replace focus machine queue with conflict set for rule->select().
        deque<Operation*> saved = focus_m.ready_q;
        focus_m.ready_q = conflict;
        StateVector s = feats.extract(machines, jobs, focus_m, t_dec);
        int rk = hh.select_rule(policy_w, s, seq, current_eps);
        Operation* chosen = rules[rk]->select(focus_m, jobs, t_dec);
        if (!chosen) chosen = conflict.front();
        // restore ready_q (not strictly needed since we rebuild each loop, but keep clean)
        focus_m.ready_q = saved;
        // 5) Schedule chosen at its earliest start
        int j = chosen->job_id;
        double st = max(job_ready[j], mach_ready[m_star]);
        st = max(st, jobs[j].release_time);
        double en = st + chosen->proc_time;
        chosen->start_time = st;
        chosen->end_time = en;
        chosen->done = true;
        if (logger) logger->logOp(m_star, chosen->job_id, chosen->op_index, st, en, chosen->proc_time);
        mach_ready[m_star] = en;
        job_ready[j] = en;
        // advance job
        jobs[j].next_op++;
        done_ops++;
        Cmax = max(Cmax, en);
        seq++; // advance deterministic stream
    }
    return SimResult{Cmax};
}
SimResult run_episode(const vector<double>& policy_w,
                          const PSOHH& hh,
                          uint64_t seed_seq_base,
                          double current_eps)
    {
        if (use_gt) {
            return run_episode_gt(policy_w, hh, seed_seq_base, current_eps);
        }
        reset_runtime();
        EventQueue eq;
        uint64_t seq = seed_seq_base;
        // schedule arrivals
        for (auto& j: jobs) {
            eq.push(Event{j.release_time, EventType::JOB_ARRIVAL, j.id, -1, nullptr, seq++});
        }
        double t=0.0;
        double Cmax=0.0;
        while (!eq.empty()) {
            Event e = eq.pop();
            t = e.time;
            if (e.type == EventType::JOB_ARRIVAL) {
                on_arrival(e.job_id);
                dispatch_all(eq, seq, t, policy_w, hh, current_eps, Cmax);
            } else { // OP_COMPLETE
                on_complete(e.machine_id, e.op, t, Cmax);
                dispatch_all(eq, seq, t, policy_w, hh, current_eps, Cmax);
            }
        }
        return SimResult{Cmax};
    }
};
// ---------------- Rule pool builder ----------------
static vector<unique_ptr<Rule>> build_rules() {
    vector<unique_ptr<Rule>> r;
    r.emplace_back(make_unique<RuleSPT>());
    r.emplace_back(make_unique<RuleLPT>());
    r.emplace_back(make_unique<RuleMWKR>());
    r.emplace_back(make_unique<RuleMOR>());
    r.emplace_back(make_unique<RuleFIFO>());
    r.emplace_back(make_unique<RuleSIO>());
    r.emplace_back(make_unique<RulePTWINQ>());
    return r;
}
// ---------------- Toy JSSP + arrival generator ----------------
// JSSP routing is fixed; we only add dynamic release times.
static vector<Job> build_toy_jssp_jobs(int n_jobs, int n_machines, RNG& rng, double lambda) {
    // For demonstration: create random fixed routes (permutation of machines) and proc times.
    // Replace this with your benchmark instance reader.
    vector<Job> jobs(n_jobs);
    // generate release times via exp interarrivals
    double rt=0.0;
    for (int j=0;j<n_jobs;++j) {
        rt += rng.exp_rv(lambda);
        jobs[j].id = j;
        jobs[j].release_time = rt;
        // random route = permutation of machines
        vector<int> route(n_machines);
        iota(route.begin(), route.end(), 0);
        shuffle(route.begin(), route.end(), rng.eng);
        jobs[j].ops.reserve(n_machines);
        for (int k=0;k<n_machines;++k) {
            double p = 5.0 + 20.0*rng.u01(); // proc time in [5,25]
            jobs[j].ops.push_back(Operation{j, k, route[k], p});
        }
    }
    return jobs;
}
static vector<Machine> build_machines(int m) {
    vector<Machine> ms(m);
    for (int i=0;i<m;++i) ms[i].id=i;
    return ms;
}
// ---------------- OR-Library (jobshop1.txt) loader ----------------
// Supports instances like: ft06, la01, la06, la11, la19, orb01, abz7, swv11 (all live in jobshop1.txt).
// File format: after "instance <name>", next line is "<nJobs> <nMachines>",
// then nJobs lines follow, each containing 2*nMachines integers: (machine_id, proc_time) pairs.
// Machine ids are 0-based in OR-Library.
static bool load_orlib_jobshop1_instance(const string& filepath,
                                        const string& instance_name,
                                        vector<Job>& out_jobs,
                                        int& out_n_machines,
                                        string* err_msg = nullptr)
{
    ifstream in(filepath);
    if (!in) {
        if (err_msg) *err_msg = "Cannot open file: " + filepath;
        return false;
    }
    auto to_lower = [](string s){
        for (auto& c: s) c = (char)tolower((unsigned char)c);
        return s;
    };
    const string target = to_lower(instance_name);
    string line;
    bool found = false;
    // 1) Find "instance <name>"
    while (std::getline(in, line)) {
        // skip empty lines
        bool all_ws = true;
        for (char c: line) if (!isspace((unsigned char)c)) { all_ws = false; break; }
        if (all_ws) continue;
        std::istringstream iss(line);
        string w1, w2;
        iss >> w1 >> w2;
        if (to_lower(w1) == "instance" && to_lower(w2) == target) {
            found = true;
            break;
        }
    }
    if (!found) {
        if (err_msg) *err_msg = "Instance not found in file: " + instance_name;
        return false;
    }
    // 2) Read "<nJobs> <nMachines>" (skip blank lines if any)
    int nJobs = 0, nMach = 0;
    while (std::getline(in, line)) {
        bool all_ws = true;
        for (char c: line) if (!isspace((unsigned char)c)) { all_ws = false; break; }
        if (all_ws) continue;
        std::istringstream iss(line);
        if (iss >> nJobs >> nMach) break;
    }
    if (nJobs <= 0 || nMach <= 0) {
        if (err_msg) *err_msg = "Invalid (nJobs, nMachines) header for instance: " + instance_name;
        return false;
    }
    // 3) Read job lines
    vector<Job> jobs;
    jobs.reserve(nJobs);
    for (int j = 0; j < nJobs; ++j) {
        // read next non-empty line
        while (std::getline(in, line)) {
            bool all_ws = true;
            for (char c: line) if (!isspace((unsigned char)c)) { all_ws = false; break; }
            if (!all_ws) break;
        }
        if (!in) {
            if (err_msg) *err_msg = "Unexpected EOF while reading jobs for instance: " + instance_name;
            return false;
        }
        std::istringstream iss(line);
        vector<int> vals;
        vals.reserve(2 * nMach);
        int x;
        while (iss >> x) vals.push_back(x);
        if ((int)vals.size() != 2 * nMach) {
            if (err_msg) {
                *err_msg = "Bad line for job " + std::to_string(j) +
                           " (expected " + std::to_string(2 * nMach) +
                           " integers, got " + std::to_string(vals.size()) + ")";
            }
            return false;
        }
        Job job;
        job.id = j;
        job.release_time = 0.0; // static JSSP
        job.ops.reserve(nMach);
        for (int k = 0; k < nMach; ++k) {
            int mid = vals[2*k];
            int pt  = vals[2*k + 1];
            if (mid < 0 || mid >= nMach) {
                if (err_msg) *err_msg = "Machine id out of range in instance data.";
                return false;
            }
            job.ops.push_back(Operation{j, k, mid, (double)pt});
        }
        jobs.push_back(std::move(job));
    }
    out_jobs = std::move(jobs);
    out_n_machines = nMach;
    return true;
}
// ---------------- Experiment Utilities ----------------
struct Stats { double mean=0, std=0, mn=0, mx=0; };
static Stats compute_stats(const vector<double>& v) {
    Stats s;
    if (v.empty()) return s;
    s.mn = *min_element(v.begin(), v.end());
    s.mx = *max_element(v.begin(), v.end());
    s.mean = accumulate(v.begin(), v.end(), 0.0) / (double)v.size();
    double var=0.0;
    for (double x: v) var += (x - s.mean) * (x - s.mean);
    var /= (double)v.size();
    s.std = sqrt(var);
    return s;
}
struct InstanceInfo {
    string name;
    int n_jobs{};
    int n_machines{};
    // string path; // if you add a real instance reader later
};
// interarrival ~ Exp(lambda); release_time[j] = cumulative sum of interarrivals
static void assign_release_times_poisson(vector<Job>& jobs, RNG& rng, double lambda) {
    double t=0.0;
    for (auto& j: jobs) {
        t += rng.exp_rv(lambda);
        j.release_time = t;
    }
}
// Build jobs with fixed routing & processing times, but WITHOUT release times (set later)
static vector<Job> build_toy_jobs_no_release(int n_jobs, int n_machines, RNG& rng) {
    vector<Job> jobs(n_jobs);
    for (int j=0;j<n_jobs;++j) {
        jobs[j].id = j;
        jobs[j].release_time = 0.0;
        vector<int> route(n_machines);
        iota(route.begin(), route.end(), 0);
        shuffle(route.begin(), route.end(), rng.eng);
        jobs[j].ops.reserve(n_machines);
        for (int k=0;k<n_machines;++k) {
            double p = 5.0 + 20.0*rng.u01(); // [5,25]
            jobs[j].ops.push_back(Operation{j, k, route[k], p});
        }
    }
    return jobs;
}
// Baseline: run one episode with a single dispatching rule (e.g., SPT)
static double run_single_rule_episode(const vector<Job>& base_jobs,
                                      const vector<Machine>& base_machines,
                                      const StateFeatures& feats,
                                      GanttLogger* logger,
                                      unique_ptr<Rule> single_rule,
                                      uint64_t seq_base)
{
    vector<unique_ptr<Rule>> rules;
    rules.emplace_back(std::move(single_rule));
    // Dummy HH: K=1 -> always rule 0, weights={1}
    RNG dummy_rng(1);
    PSOParams pp; pp.swarm_size=1; pp.iters=1;
    PSOHH dummy_hh(pp, 1, dummy_rng);
    vector<double> w = {1.0};
    Simulation sim(base_jobs, base_machines, std::move(rules), feats, logger, true);
    SimResult res = sim.run_episode(w, dummy_hh, seq_base, /*eps*/ 0.0);
    return res.Cmax;
}
// PSO-HH: train a policy for a given scenario and return best Cmax
static double run_psohh_scenario(const vector<Job>& base_jobs,
                                const vector<Machine>& base_machines,
                                const StateFeatures& feats,
                                GanttLogger* logger,
                                RNG& rng_pso,
                                uint64_t seq_base,
                                int pso_iters,
                                int swarm_size)
{
    auto rule_pool = build_rules();
    const int K = (int)rule_pool.size();
    PSOParams pp;
    pp.iters = pso_iters;
    pp.swarm_size = swarm_size;
    PSOHH hh(pp, K, rng_pso);
    for (int iter=0; iter<pp.iters; ++iter) {
        // Epsilon schedule: high exploration at start, decays to pp.eps_min
        double frac = (pp.iters<=1) ? 1.0 : (1.0 - (double)iter / (double)(pp.iters-1));
        double current_eps = max(pp.eps_min, pp.eps0 * frac);
        for (int i=0;i<pp.swarm_size;++i) {
            Simulation sim(base_jobs, base_machines, build_rules(), feats, logger, true);
            const auto& w = hh.particle_weights(i);
            uint64_t sb = seq_base + (uint64_t)iter*100000ULL + (uint64_t)i*1000ULL;
            SimResult res = sim.run_episode(w, hh, sb, current_eps);
            hh.update_fitness(i, res.Cmax);
        }
        hh.move();
    }
    return hh.gbest_fit;
}
static void run_experiments() {
    // 1) Instance list (placeholder; connect your real instance reader later)
    vector<InstanceInfo> instances = {
        {"FT10", 10, 10},
        {"LA16", 10, 10},
        {"LA21", 15, 10},
        {"ABZ6", 10, 10},
        {"ABZ7", 20, 15},
    };
    // 2) Lambda scenarios (low/medium/high dynamic arrival intensity)
    //    If you want a paper-like (static ORLIB) table, set lambdas = {0.0} and skip release-time assignment.
    vector<double> lambdas = {0.2, 0.5, 1.0};
    // 3) Replications per (instance, lambda)
    const int R = 30;
    // 4) Feature config (tune these for your thesis)
    FeatureConfig fc;
    fc.q_max = 50;
    fc.wip_max = 200;
    fc.rw_scale = 200.0;
    StateFeatures feats(fc);
    // 5) Methods to compare
    //    NOTE: You can add more baselines by implementing new Rule classes.
    const vector<string> methods = {"PSOHH", "SPT", "LPT", "MWKR", "MOR", "FIFO"};
    // 6) Output CSVs (long format)
    ofstream raw("raw_results.csv");
    raw << "instance,n_jobs,n_machines,lambda,seed,method,Cmax\n";
    ofstream sum("summary_results.csv");
    sum << "instance,n_jobs,n_machines,lambda,method,mean,std,min,max\n";
    // 7) Output CSVs (wide tables like in thesis papers)
    ofstream avg_tbl("comparison_avg.csv");
    ofstream best_tbl("comparison_best.csv");
    // headers
    avg_tbl << "instance,lambda";
    best_tbl << "instance,lambda";
    for (auto& m : methods) { avg_tbl << "," << m; best_tbl << "," << m; }
    avg_tbl << "\n";
    best_tbl << "\n";
    for (const auto& inst : instances) {
        for (double lambda : lambdas) {
            // cmax vectors per method
            unordered_map<string, vector<double>> cmax_by_method;
            for (auto& m : methods) cmax_by_method[m] = {};
            for (int rep=1; rep<=R; ++rep) {
                // Scenario RNG (routes/proc times + release times)
                RNG rng_scn(1000ULL * (uint64_t)rep + 17ULL);
                // PSO RNG (optimizer randomness)
                RNG rng_pso(9000ULL * (uint64_t)rep + 99ULL);
                // TODO: replace with real instance reader
                vector<Job> jobs = build_toy_jobs_no_release(inst.n_jobs, inst.n_machines, rng_scn);
                // Dynamic arrivals
                assign_release_times_poisson(jobs, rng_scn, lambda);
                vector<Machine> machines = build_machines(inst.n_machines);
                // Keep Gantt logging OFF for batch experiments (much faster)
                GanttLogger* logger = nullptr;
                // --- PSO-HH (our hyper-heuristic) ---
                {
                    double Cmax = run_psohh_scenario(jobs, machines, feats, logger, rng_pso,
                                                    (uint64_t)rep*1000000ULL,
                                                    /*pso_iters*/ 25,
                                                    /*swarm*/ 15);
                    cmax_by_method["PSOHH"].push_back(Cmax);
                    raw << inst.name << "," << inst.n_jobs << "," << inst.n_machines << ","
                        << lambda << "," << rep << "," << "PSOHH" << "," << Cmax << "\n";
                }
                // --- Dispatching-rule baselines ---
                auto run_rule = [&](const string& tag, unique_ptr<Rule> rule, uint64_t seq)->void{
                    double Cmax = run_single_rule_episode(jobs, machines, feats, logger, std::move(rule), seq);
                    cmax_by_method[tag].push_back(Cmax);
                    raw << inst.name << "," << inst.n_jobs << "," << inst.n_machines << ","
                        << lambda << "," << rep << "," << tag << "," << Cmax << "\n";
                };
                run_rule("SPT",  make_unique<RuleSPT>(),  (uint64_t)rep*2000000ULL + 1ULL);
                run_rule("LPT",  make_unique<RuleLPT>(),  (uint64_t)rep*2000000ULL + 2ULL);
                run_rule("MWKR", make_unique<RuleMWKR>(), (uint64_t)rep*2000000ULL + 3ULL);
                run_rule("MOR",  make_unique<RuleMOR>(),  (uint64_t)rep*2000000ULL + 4ULL);
                run_rule("FIFO", make_unique<RuleFIFO>(), (uint64_t)rep*2000000ULL + 5ULL);
            }
            // Summaries (long format) + build wide rows
            avg_tbl  << inst.name << "," << lambda;
            best_tbl << inst.name << "," << lambda;
            for (auto& mth : methods) {
                auto st = compute_stats(cmax_by_method[mth]);
                // long format summary
                sum << inst.name << "," << inst.n_jobs << "," << inst.n_machines << ","
                    << lambda << "," << mth << ","
                    << st.mean << "," << st.std << "," << st.mn << "," << st.mx << "\n";
                // wide tables:
                // - "avg" table uses mean Cmax
                // - "best" table uses min Cmax (best run)
                avg_tbl  << "," << st.mean;
                best_tbl << "," << st.mn;
            }
            avg_tbl  << "\n";
            best_tbl << "\n";
        }
    }
    cerr << "Experiments finished. Wrote raw_results.csv, summary_results.csv, comparison_avg.csv, comparison_best.csv\n";
}
// ---------------- Main ----------------
// ---------------- Schedule Local Search (critical-block swaps) ----------------
// This intensification operates on a fixed machine-order representation.
// Starting from a feasible schedule (e.g., produced by GT SGS), we extract
// machine processing orders, evaluate makespan via longest-path DP on the
// precedence+machine-order DAG, then apply Nowicki–Smutnicki style N1 moves:
// swap the first/last adjacent pair of each critical block.
struct SchEval {
    double Cmax = 0.0;
    vector<double> start;      // size N
    vector<int> pred;          // size N, predecessor node in longest path (or -1)
    int last_node = -1;        // node achieving Cmax
    bool acyclic = true;
};
static SchEval eval_machine_order_schedule(
    int J, int M,
    const vector<int>& machine_of,           // size N
    const vector<double>& proc,              // size N
    const vector<vector<int>>& order_by_m    // M x J (node ids)
){
    const int N = J * M;
    vector<vector<int>> succ(N);
    vector<int> indeg(N, 0);
    auto add_edge = [&](int u, int v){
        succ[u].push_back(v);
        indeg[v]++;
    };
    // Job precedence edges
    for(int j=0;j<J;j++){
        for(int k=0;k<M-1;k++){
            int u = j*M + k;
            int v = j*M + (k+1);
            add_edge(u,v);
        }
    }
    // Machine order edges
    for(int m=0;m<M;m++){
        const auto& ord = order_by_m[m];
        for(int i=0;i<(int)ord.size()-1;i++){
            add_edge(ord[i], ord[i+1]);
        }
    }
    // Longest path in DAG (Kahn)
    deque<int> q;
    for(int i=0;i<N;i++) if(indeg[i]==0) q.push_back(i);
    vector<double> dist(N, 0.0); // start times
    vector<int> pred(N, -1);
    int seen = 0;
    while(!q.empty()){
        int u = q.front(); q.pop_front();
        seen++;
        double u_end = dist[u] + proc[u];
        for(int v: succ[u]){
            if(dist[v] < u_end){
                dist[v] = u_end;
                pred[v] = u;
            }
            indeg[v]--;
            if(indeg[v]==0) q.push_back(v);
        }
    }
    SchEval out;
    out.start = std::move(dist);
    out.pred  = std::move(pred);
    if(seen != N){
        out.acyclic = false;
        out.Cmax = 1e18;
        return out;
    }
    double Cmax = 0.0;
    int last = -1;
    for(int u=0;u<N;u++){
        double endt = out.start[u] + proc[u];
        if(endt > Cmax){
            Cmax = endt;
            last = u;
        }
    }
    out.Cmax = Cmax;
    out.last_node = last;
    return out;
}
static vector<int> extract_critical_path_nodes(const SchEval& e, int N){
    vector<int> path;
    int u = e.last_node;
    while(u != -1){
        path.push_back(u);
        u = e.pred[u];
    }
    reverse(path.begin(), path.end());
    // optional sanity: path length <= N
    return path;
}
static bool schedule_local_search_critical_blocks(
    int J, int M,
    const vector<int>& machine_of,
    const vector<double>& proc,
    vector<vector<int>>& order_by_m,
    int max_iters,
    double* out_best_Cmax,
    vector<double>* out_best_start
){
    const int N = J*M;
    SchEval cur = eval_machine_order_schedule(J,M,machine_of,proc,order_by_m);
    if(!cur.acyclic) return false;
    double best = cur.Cmax;
    for(int it=0; it<max_iters; ++it){
        auto crit = extract_critical_path_nodes(cur, N);
        if(crit.size() < 2) break;
        // Build a quick position lookup on each machine
        vector<vector<int>> pos(M, vector<int>(N, -1));
        for(int m=0;m<M;m++){
            for(int i=0;i<(int)order_by_m[m].size();i++){
                pos[m][order_by_m[m][i]] = i;
            }
        }
        // Identify critical blocks as segments of consecutive crit nodes on same machine
        struct Block { int m; int lpos; int rpos; }; // inclusive in machine order positions
        vector<Block> blocks;
        for(size_t i=0;i<crit.size();){
            int u = crit[i];
            int m = machine_of[u];
            int l = pos[m][u];
            int r = l;
            size_t j = i+1;
            while(j < crit.size()){
                int v = crit[j];
                if(machine_of[v] != m) break;
                int pv = pos[m][v];
                // require adjacency in machine order to be a "block" in the disjunctive graph sense
                if(pv != r+1) break;
                r = pv;
                j++;
            }
            if(r - l + 1 >= 2){
                blocks.push_back(Block{m,l,r});
            }
            i = j;
        }
        double best_nei = best;
        vector<vector<int>> best_ord;
        SchEval best_eval;
        auto try_swap = [&](int m, int i){
            if(i < 0 || i+1 >= (int)order_by_m[m].size()) return;
            auto cand = order_by_m;
            std::swap(cand[m][i], cand[m][i+1]);
            SchEval ev = eval_machine_order_schedule(J,M,machine_of,proc,cand);
            if(ev.acyclic && ev.Cmax < best_nei){
                best_nei = ev.Cmax;
                best_ord = std::move(cand);
                best_eval = std::move(ev);
            }
        };
        for(const auto& b: blocks){
            // Explore adjacent swaps within the whole critical block (stronger than only first/last pair).
            for(int i=b.lpos; i<=b.rpos-1; ++i){
                try_swap(b.m, i);
            }
            // Also explore boundary swaps with neighbors just outside the block (Nowicki-Smutnicki style).
            try_swap(b.m, b.lpos-1);
            try_swap(b.m, b.rpos);
        }
        if(best_nei + 1e-9 < best){
            best = best_nei;
            order_by_m = std::move(best_ord);
            cur = std::move(best_eval);
        } else {
            break; // no improvement
        }
    }
    if(out_best_Cmax) *out_best_Cmax = best;
    if(out_best_start) *out_best_start = cur.start;
    return true;
}
static bool improve_gantt_with_schedule_ls(
    const vector<Job>& jobs_model,
    int J, int M,
    vector<GanttLogger::Entry>& rows,
    int sls_iters,
    double* out_new_Cmax
){
    if(sls_iters <= 0) return false;
    const int N = J*M;
    // Build node -> (machine, proc)
    vector<int> machine_of(N, -1);
    vector<double> proc(N, 0.0);
    for(int j=0;j<J;j++){
        for(int k=0;k<M;k++){
            int id = j*M + k;
            machine_of[id] = jobs_model[j].ops[k].machine_id;
            proc[id] = jobs_model[j].ops[k].proc_time;
        }
    }
    // Current machine orders from rows (sorted by start)
    vector<vector<pair<double,int>>> tmp(M);
    for(const auto& e: rows){
        int node = e.job_id * M + e.op_index;
        if(e.machine_id < 0 || e.machine_id >= M) continue;
        tmp[e.machine_id].push_back({e.start, node});
    }
    vector<vector<int>> order_by_m(M);
    for(int m=0;m<M;m++){
        auto& v = tmp[m];
        sort(v.begin(), v.end(), [](auto& a, auto& b){ return a.first < b.first; });
        order_by_m[m].reserve(v.size());
        for(auto& p: v) order_by_m[m].push_back(p.second);
        // If something went wrong and we didn't get J ops on a machine, bail
        if((int)order_by_m[m].size() != J) return false;
    }
    double bestC = 0.0;
    vector<double> bestStart;
    if(!schedule_local_search_critical_blocks(J,M,machine_of,proc,order_by_m,sls_iters,&bestC,&bestStart)){
        return false;
    }
    // Rebuild rows from bestStart
    vector<GanttLogger::Entry> new_rows;
    new_rows.reserve(rows.size());
    for(int j=0;j<J;j++){
        for(int k=0;k<M;k++){
            int id = j*M + k;
            int mach = machine_of[id];
            double s = bestStart[id];
            double e = s + proc[id];
            new_rows.push_back(GanttLogger::Entry{mach, j, k, s, e, proc[id]});
        }
    }
    rows.swap(new_rows);
    if(out_new_Cmax) *out_new_Cmax = bestC;
    return true;
}
int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    // Mode switch:
    //   ./a.out experiments   -> batch runner (raw_results.csv + summary_results.csv)
    //   ./a.out [ft06|la01|...] -> single run (writes gantt_<instance>.csv)
    if (argc >= 2 && string(argv[1]) == "experiments") {
        run_experiments();
        return 0;
    }
    // ----- Single demo (OR-Library instance) -----
// Uses data/jobshop1.txt and runs "ft06" by default.
// You can optionally pass instance name as first arg (except "experiments"), e.g.:
//   ./a.out ft06
//   ./a.out la01
    const string orlib_path = "data/jobshop1.txt";
// ---- CLI parsing ----
// Usage:
//   xsim.exe [instance] [--eps0 <v>] [--epsmin <v>] [--iters <n>] [--swarm <n>] [--seed <n>]
// Examples:
//   xsim.exe                (defaults: ft06)
//   xsim.exe la01
//   xsim.exe ft06 --eps0 0.35 --epsmin 0.05 --iters 150 --swarm 40
string instance = "ft06";
// If the first arg exists and is not an option, treat it as instance name
int ai = 1;
if (ai < argc && string(argv[ai]).rfind("--", 0) != 0) {
    instance = string(argv[ai]);
    ++ai;
}
auto parse_double = [&](const string& s, double& out)->bool{
    try { out = stod(s); return true; } catch(...) { return false; }
};
auto parse_int = [&](const string& s, int& out)->bool{
    try { out = stoi(s); return true; } catch(...) { return false; }
};
// Defaults (can be overridden by flags)
PSOParams p;
uint64_t seed = 777;
int eval_k = 5;           // number of evaluations per particle (different tie-break streams)
int final_k = 200;        // number of deterministic re-evals for final reporting/Gantt
int sls_iters = 0;      // schedule-level local search iterations (critical-block swaps)
bool fitness_take_min = true; // min-of-k fitness (helps reach rare good schedules)
bool use_gt_sgs = true;       // default: GT active schedule generator for ORLIB
// --- Optional post-PSO local search in parameter space (very helpful on harder instances like la19+) ---
int ls_iters = 0;             // 0 = off
double ls_step = 0.25;        // Gaussian step size for parameter perturbations
// --- Training evaluation mode ---
bool train_deterministic = false; // if true: eps=0 during training; else epsilon decays from eps0 to epsmin
// p has defaults set in struct; we override below if flags provided.
while (ai < argc) {
        string key = argv[ai++];
        auto need = [&](const string& what)->string{
            if (ai >= argc) { cerr << "Missing value after " << what << "\n"; exit(1); }
            return string(argv[ai++]);
        };
        if (key == "--eps0") {
            string v = need(key);
            if (!parse_double(v, p.eps0)) { cerr << "Bad --eps0 value\n"; return 1; }
        } else if (key == "--epsmin") {
            string v = need(key);
            if (!parse_double(v, p.eps_min)) { cerr << "Bad --epsmin value\n"; return 1; }
        } else if (key == "--iters") {
            string v = need(key);
            if (!parse_int(v, p.iters)) { cerr << "Bad --iters value\n"; return 1; }
        } else if (key == "--swarm") {
            string v = need(key);
            if (!parse_int(v, p.swarm_size)) { cerr << "Bad --swarm value\n"; return 1; }
        } else if (key == "--seed") {
            string v = need(key);
            try { seed = (uint64_t)stoull(v); } catch(...) { cerr << "Bad --seed value\n"; return 1; }
        } else if (key == "--evalk") {
            string v = need(key);
            if (!parse_int(v, eval_k) || eval_k < 1) { cerr << "Bad --evalk value\n"; return 1; }
        } else if (key == "--finalk") {
            string v = need(key);
            if (!parse_int(v, final_k) || final_k < 1) { cerr << "Bad --finalk value\n"; return 1; }
        } else if (key == "--slsiters") {
            string v = need(key);
            if (!parse_int(v, sls_iters) || sls_iters < 0) { cerr << "Bad --slsiters value\n"; return 1; }
        } else if (key == "--fitavg") {
            fitness_take_min = false;
        } else if (key == "--sgs") {
            string v = need(key);
            if (v == "gt" || v == "GT") use_gt_sgs = true;
            else if (v == "event" || v == "EVENT") use_gt_sgs = false;
            else { cerr << "Bad --sgs value (use gt|event)\n"; return 1; }
        } else if (key == "--lsiters") {
            string v = need(key);
            if (!parse_int(v, ls_iters) || ls_iters < 0) { cerr << "Bad --lsiters value\n"; return 1; }
        } else if (key == "--lsstep") {
            string v = need(key);
            if (!parse_double(v, ls_step) || ls_step <= 0.0) { cerr << "Bad --lsstep value\n"; return 1; }
        } else if (key == "--traindet") {
            train_deterministic = true;
        } else if (key == "--help" || key == "-h") {
            cout << "Usage: xsim.exe [instance] [--eps0 <v>] [--epsmin <v>] [--iters <n>] [--swarm <n>] [--seed <n>] [--evalk <n>] [--finalk <n>] [--fitavg] [--sgs gt|event] [--lsiters <n>] [--lsstep <v>] [--traindet]\n";
            return 0;
        } else {
            cerr << "Unknown option: " << key << "\n";
            cerr << "Try: xsim.exe --help\n";
            return 1;
        }
    }
    vector<Job> base_jobs;
    int n_machines = 0;
    string err;
    if (!load_orlib_jobshop1_instance(orlib_path, instance, base_jobs, n_machines, &err)) {
        cerr << "[ORLIB] " << err << "\n";
        cerr << "Make sure '" << orlib_path << "' exists (e.g., Xsim/data/jobshop1.txt)\n";
        return 1;
    }
    const int n_jobs = (int)base_jobs.size();
    auto base_machines = build_machines(n_machines);
FeatureConfig fcfg;
    fcfg.q_max = 30;
    fcfg.wip_max = n_jobs;
    fcfg.rw_scale = 200.0;
    StateFeatures feats(fcfg);
    // PSO
    RNG rng_pso(seed);
    p.w_inertia = 0.7;
    p.c1 = 1.4;
    p.c2 = 1.4;
    auto tmp_rules = build_rules();
    const int K = (int)tmp_rules.size();
    PSOHH hh(p, K, rng_pso);
    // Training phase: no Gantt logging (avoid mixing multiple episodes)
        for (int iter=0; iter<p.iters; ++iter) {
        // IMPORTANT (why train_best != final_best happened before):
        // During training, epsilon-greedy (eps>0) can produce *lucky* schedules that
        // are not reproducible when we later evaluate deterministically (eps=0).
        // For a fair comparison table and for consistency with the Gantt schedule,
        // we optimize **deterministic** performance here.
        //
        // We still keep stochasticity via different tie-break streams (seq_base)
        // across reps/iterations/particles.
        double frac_eps = (p.iters<=1) ? 1.0 : (double)iter / (double)(p.iters-1);
        double current_eps = max(p.eps_min, p.eps0 * (1.0 - frac_eps));
        const double eval_eps = train_deterministic ? 0.0 : current_eps; // training exploration (optional)
        double iter_best = 1e300, iter_worst = -1e300, iter_sum = 0.0;
        for (int i=0;i<p.swarm_size;++i) {
            // NOTE: Eğitim sırasında Gantt logu tutmuyoruz (hız + karışıklık olmasın)
            Simulation sim(base_jobs, base_machines, build_rules(), feats, nullptr, use_gt_sgs);
            const auto& w = hh.particle_weights(i);
                        // Evaluate each particle multiple times with different tie-break streams.
            // This reduces the risk of getting stuck due to a single deterministic simultaneous-dispatch ordering.
            double fit = 1e300;
            double fit_sum = 0.0;
            for (int rep=0; rep<eval_k; ++rep) {
                uint64_t seq_base =
                    (seed * 1315423911ULL) ^
                    ((uint64_t)iter * 100000ULL) ^
                    ((uint64_t)i * 1000ULL) ^
                    ((uint64_t)rep * 17ULL);
                SimResult r = sim.run_episode(w, hh, seq_base, eval_eps);
                if (fitness_take_min) {
                    fit = min(fit, (double)r.Cmax);
                } else {
                    fit_sum += (double)r.Cmax;
                }
            }
            if (!fitness_take_min) fit = fit_sum / max(1, eval_k);
            hh.update_fitness(i, fit);
            // For progress printing we still track the particle fitnesses used by PSO
            iter_best  = min(iter_best,  fit);
            iter_worst = max(iter_worst, fit);
            iter_sum  += fit;
        }
        hh.move();
        cout << "iter=" << iter
             << "  iter_best=" << (long long)iter_best
             << "  iter_avg="  << (iter_sum / p.swarm_size)
             << "  iter_worst="<< (long long)iter_worst
             << "  gbest_Cmax="<< hh.gbest_fit
             << "\n";
    }
    // IMPORTANT:
    // hh.gbest_fit is the best fitness observed DURING TRAINING.
    // Training uses an exploration epsilon > 0 and also varies seq_base, which can introduce noise.
    // For reporting (and for Gantt), we must re-evaluate deterministically with eps=0 using the
    // SAME seq_base that will be used for logging, so "Best Cmax" matches the Gantt makespan.
        
    // ---------------- Optional post-PSO local search (hill-climb in theta space) ----------------
    auto eval_theta = [&](const vector<double>& theta, uint64_t seed_base)->double{
        Simulation sim(base_jobs, base_machines, build_rules(), feats, nullptr, use_gt_sgs);
        double fit = 1e300;
        double sum = 0.0;
        for (int rep=0; rep<eval_k; ++rep) {
            uint64_t seq_base =
                (seed_base * 2654435761ULL) ^
                ((uint64_t)rep * 97ULL) ^
                0x9E3779B97F4A7C15ULL;
            SimResult r = sim.run_episode(theta, hh, seq_base, /*eps*/ 0.0);
            if (fitness_take_min) fit = min(fit, (double)r.Cmax);
            else sum += (double)r.Cmax;
        }
        if (!fitness_take_min) fit = sum / max(1, eval_k);
        return fit;
    };
    // Choose a *deterministically* best theta before doing local search / final evaluation.
// Note: hh.gbest_fit is the best value *seen during PSO training* (may be stochastic if training uses eps>0).
// For reporting + Gantt we care about deterministic eps=0 evaluation, so we re-screen candidate thetas here.
vector<double> best_theta = hh.gbest;
double best_theta_fit = eval_theta(best_theta, seed ^ 0xC0FFEEULL);
// Re-screen all particles' personal bests under deterministic evaluation and keep the best.
for (size_t i=0; i<hh.swarm.size(); ++i) {
    if (hh.swarm[i].pbest.empty()) continue;
    double f = eval_theta(hh.swarm[i].pbest, (seed ^ 0xD00DFEEDULL) + (uint64_t)i*1315423911ULL);
    if (f + 1e-9 < best_theta_fit) {
        best_theta_fit = f;
        best_theta = hh.swarm[i].pbest;
    }
}
    if (ls_iters > 0) {
        std::mt19937_64 ls_rng(seed ^ 0xA5A5A5A5ULL);
        std::normal_distribution<double> nd(0.0, ls_step);
        const double XMAX = 6.0;
        const int D = (int)best_theta.size();
        for (int it=0; it<ls_iters; ++it) {
            vector<double> cand = best_theta;
            // Perturb 1-3 random dimensions
            int n_mut = 1 + (int)(ls_rng() % 3ULL);
            for (int mm=0; mm<n_mut; ++mm) {
                int d = (int)(ls_rng() % (uint64_t)D);
                cand[d] = clampd(cand[d] + nd(ls_rng), -XMAX, XMAX);
            }
            double cand_fit = eval_theta(cand, (seed + (uint64_t)it + 1ULL) ^ 0xBADC0DEULL);
            // Strict improvement accept (simple hill-climb)
            if (cand_fit + 1e-9 < best_theta_fit) {
                best_theta_fit = cand_fit;
                best_theta = std::move(cand);
            }
        }
    }
// Final deterministic re-evaluation (eps=0):
    // We try multiple tie-break streams and keep the best one. This is important because when
    // multiple machines become free at the same time, the processing order of dispatch decisions
    // can change the resulting schedule even under the same policy weights.
    double best_final = 1e300;
    uint64_t best_seq = 999000000ULL;
    {
        Simulation sim(base_jobs, base_machines, build_rules(), feats, nullptr, use_gt_sgs);
        for (int rep=0; rep<final_k; ++rep) {
            uint64_t seq_base =
                (seed * 2654435761ULL) ^
                999000000ULL ^
                ((uint64_t)rep * 97ULL);
            SimResult r = sim.run_episode(best_theta, hh, seq_base, /*eps*/ 0.0);
            if ((double)r.Cmax < best_final) {
                best_final = (double)r.Cmax;
                best_seq = seq_base;
            }
        }
    }
    // Now write Gantt using the best deterministic stream found above
    GanttLogger logger(string("gantt_") + instance + string(".csv"));
    SimResult final_res;
    {
        Simulation sim(base_jobs, base_machines, build_rules(), feats, &logger, use_gt_sgs);
        final_res = sim.run_episode(best_theta, hh, best_seq, /*eps*/ 0.0);
        // Optional schedule-level local search (critical block swaps) on the final deterministic schedule
        if (sls_iters > 0) {
            double newC = final_res.Cmax;
            bool ok = improve_gantt_with_schedule_ls(base_jobs, (int)base_jobs.size(), (int)base_machines.size(), logger.rows, sls_iters, &newC);
            if (ok) final_res.Cmax = newC;
        }
        logger.write_sorted();
    }
    cout << "Done. Best Cmax=" << (long long)final_res.Cmax
         << " (train_best=" << (long long)hh.gbest_fit
         << ", det_screen_best=" << (long long)best_theta_fit
         << ", best_seq_Cmax=" << (long long)best_final
         << ", best_seq=" << (unsigned long long)best_seq << ")\n";
    {
        auto r = build_rules();
        for (int k=0;k<(int)r.size();++k) cout << r[k]->name() << (k+1==(int)r.size()? '\n' : ' ');
    }
    cout << "Gantt written to gantt_" << instance << ".csv\n";
    return 0;
}