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
    ofstream out;
    explicit GanttLogger(const string& path) : out(path) {
        out << "machine_id,job_id,op_index,start,end,proc_time\n";
    }
    void logOp(int m, int j, int opi, double s, double e, double p) {
        out << m << "," << j << "," << opi << "," << s << "," << e << "," << p << "\n";
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
};

struct Particle {
    vector<double> x, v, pbest;
    double pbest_fit = 1e300;
};

static void normalize_pos(vector<double>& x) {
    double sum=0.0;
    for (double& a: x) { a = max(1e-9, a); sum += a; }
    for (double& a: x) a /= sum;
}

enum class RuleGroup { PROC_TIME, WORK_REMAINING, FIFO_OTHER };

static RuleGroup group_of_rule_index(int k) {
    // Rule order we will use:
    // 0 SPT (PROC_TIME)
    // 1 LPT (PROC_TIME)
    // 2 MWKR (WORK_REMAINING)
    // 3 MOR (WORK_REMAINING)
    // 4 FIFO (FIFO_OTHER)
    if (k==0 || k==1) return RuleGroup::PROC_TIME;
    if (k==2 || k==3) return RuleGroup::WORK_REMAINING;
    return RuleGroup::FIFO_OTHER;
}

static double psi_for_rule(const StateVector& s, RuleGroup g) {
    // s=[U,Qm,WIP,RWavg]
    double U=s.x[0], Qm=s.x[1], WIP=s.x[2], RWavg=s.x[3];

    double psi=1.0;
    if (g==RuleGroup::WORK_REMAINING) {
        // busy & congested => emphasize load/remaining-work rules
        psi += 0.7*U + 0.6*Qm + 0.4*WIP + 0.2*RWavg;
    } else if (g==RuleGroup::PROC_TIME) {
        // light system => emphasize quick completion
        psi += 0.6*(1-U) + 0.6*(1-Qm) + 0.2*(1-WIP);
    } else {
        psi += 0.2;
    }
    return clamp(psi, 0.5, 2.5);
}

struct PSOHH {
    PSOParams p;
    int K;
    RNG& rng;
    vector<Particle> swarm;
    vector<double> gbest;
    double gbest_fit = 1e300;

    PSOHH(PSOParams pp, int K_, RNG& r) : p(pp), K(K_), rng(r) {
        swarm.resize(p.swarm_size);
        for (auto& pt: swarm) {
            pt.x.resize(K); pt.v.resize(K); pt.pbest.resize(K);
            for (int k=0;k<K;++k) {
                pt.x[k] = 0.1 + rng.u01();
                pt.v[k] = (rng.u01()-0.5)*0.2;
            }
            normalize_pos(pt.x);
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
        for (auto& pt: swarm) {
            double r1=rng.u01(), r2=rng.u01();
            for (int k=0;k<K;++k) {
                pt.v[k] = p.w_inertia*pt.v[k]
                        + p.c1*r1*(pt.pbest[k]-pt.x[k])
                        + p.c2*r2*(gbest[k]-pt.x[k]);
                pt.x[k] += pt.v[k];
            }
            normalize_pos(pt.x);
        }
    }

    
// Select rule index using given weights (particle policy) + state scaling
// Adds exploration via epsilon-greedy and tiny tie-breaking noise.
int select_rule(const vector<double>& w, const StateVector& s, uint64_t seed) const {
    uint64_t x = seed ^ 0xD1B54A32D192ED03ULL;

    // 1) Exploration: with small probability, pick a random rule
    constexpr double EPS = 0.35;
    if (u01_from_u64(x) < EPS) {
        return randint_from_u64(x, K);
    }

    // 2) Exploitation: argmax(score) with tiny noise to break ties
    int best = 0;
    double bestScore = -1e300;

    for (int k=0; k<K; ++k) {
        double psi = psi_for_rule(s, group_of_rule_index(k));
        double score = w[k] * psi;

        // tiny symmetric noise (~1e-12 scale) to break equalities deterministically
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

    Simulation(vector<Job> j, vector<Machine> m,
               vector<unique_ptr<Rule>> r,
               StateFeatures f, GanttLogger* lg)
        : jobs(std::move(j)), machines(std::move(m)),
          rules(std::move(r)), feats(std::move(f)), logger(lg) {}

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
                          double& Cmax)
    {
        // compute state for this decision point
        StateVector s = feats.extract(machines, jobs, m, t);

        // choose rule and operation
        int rk = hh.select_rule(policy_w, s, seq);
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
                      double& Cmax)
    {
        bool progressed=true;
        while (progressed) {
            progressed=false;
            for (auto& m: machines) {
                if (m.isIdle(t) && !m.ready_q.empty()) {
                    dispatch_machine(eq, seq, t, m, policy_w, hh, Cmax);
                    progressed=true;
                }
            }
        }
    }

    SimResult run_episode(const vector<double>& policy_w,
                          const PSOHH& hh,
                          uint64_t seed_seq_base = 0)
    {
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
                dispatch_all(eq, seq, t, policy_w, hh, Cmax);
            } else { // OP_COMPLETE
                on_complete(e.machine_id, e.op, t, Cmax);
                dispatch_all(eq, seq, t, policy_w, hh, Cmax);
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

    Simulation sim(base_jobs, base_machines, std::move(rules), feats, logger);
    SimResult res = sim.run_episode(w, dummy_hh, seq_base);
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
        for (int i=0;i<pp.swarm_size;++i) {
            Simulation sim(base_jobs, base_machines, build_rules(), feats, logger);
            const auto& w = hh.particle_weights(i);
            uint64_t sb = seq_base + (uint64_t)iter*100000ULL + (uint64_t)i*1000ULL;
            SimResult res = sim.run_episode(w, hh, sb);
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
    vector<double> lambdas = {0.2, 0.5, 1.0};

    // 3) Replications
    const int R = 30;

    // 4) Feature config (tune these for your thesis)
    FeatureConfig fc;
    fc.q_max = 50;
    fc.wip_max = 200;
    fc.rw_scale = 200.0;
    StateFeatures feats(fc);

    // 5) Output CSVs
    ofstream raw("raw_results.csv");
    raw << "instance,n_jobs,n_machines,lambda,seed,method,Cmax\n";

    ofstream sum("summary_results.csv");
    sum << "instance,n_jobs,n_machines,lambda,method,mean,std,min,max\n";

    for (const auto& inst : instances) {
        for (double lambda : lambdas) {
            vector<double> cmax_psohh, cmax_spt;

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

                // PSO-HH
                {
                    double Cmax = run_psohh_scenario(jobs, machines, feats, logger, rng_pso,
                                                    (uint64_t)rep*1000000ULL,
                                                    /*pso_iters*/ 25,
                                                    /*swarm*/ 15);
                    cmax_psohh.push_back(Cmax);
                    raw << inst.name << "," << inst.n_jobs << "," << inst.n_machines << ","
                        << lambda << "," << rep << "," << "PSOHH" << "," << Cmax << "\n";
                }

                // Baseline SPT
                {
                    double Cmax = run_single_rule_episode(jobs, machines, feats, logger,
                                                          make_unique<RuleSPT>(),
                                                          (uint64_t)rep*2000000ULL);
                    cmax_spt.push_back(Cmax);
                    raw << inst.name << "," << inst.n_jobs << "," << inst.n_machines << ","
                        << lambda << "," << rep << "," << "SPT" << "," << Cmax << "\n";
                }
            }

            // Summaries
            {
                auto st = compute_stats(cmax_psohh);
                sum << inst.name << "," << inst.n_jobs << "," << inst.n_machines << ","
                    << lambda << "," << "PSOHH" << ","
                    << st.mean << "," << st.std << "," << st.mn << "," << st.mx << "\n";
            }
            {
                auto st = compute_stats(cmax_spt);
                sum << inst.name << "," << inst.n_jobs << "," << inst.n_machines << ","
                    << lambda << "," << "SPT" << ","
                    << st.mean << "," << st.std << "," << st.mn << "," << st.mx << "\n";
            }
        }
    }

    cerr << "Experiments finished. Wrote raw_results.csv and summary_results.csv\n";
}

// ---------------- Main ----------------
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
    string instance = "ft06";
    if (argc >= 2) instance = string(argv[1]);

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
    RNG rng_pso(777);
    PSOParams p;
    p.swarm_size = 40;
    p.iters = 150;
    p.w_inertia = 0.7;
    p.c1 = 1.4;
    p.c2 = 1.4;

    auto tmp_rules = build_rules();
    const int K = (int)tmp_rules.size();
    PSOHH hh(p, K, rng_pso);

    // Log all episodes here for demo
    GanttLogger logger(string("gantt_") + instance + string(".csv"));

        for (int iter=0; iter<p.iters; ++iter) {
        double iter_best = 1e300, iter_worst = -1e300, iter_sum = 0.0;

        for (int i=0;i<p.swarm_size;++i) {
            // NOTE: Eğitim sırasında Gantt logu tutmuyoruz (hız + karışıklık olmasın)
            Simulation sim(base_jobs, base_machines, build_rules(), feats, nullptr);

            const auto& w = hh.particle_weights(i);
            uint64_t seq_base = (uint64_t)iter*100000ULL + (uint64_t)i*1000ULL;

            SimResult res = sim.run_episode(w, hh, seq_base);
            hh.update_fitness(i, res.Cmax);

            iter_best  = min(iter_best,  (double)res.Cmax);
            iter_worst = max(iter_worst, (double)res.Cmax);
            iter_sum  += (double)res.Cmax;
        }

        hh.move();

        cout << "iter=" << iter
             << "  iter_best=" << (long long)iter_best
             << "  iter_avg="  << (iter_sum / p.swarm_size)
             << "  iter_worst="<< (long long)iter_worst
             << "  gbest_Cmax="<< hh.gbest_fit
             << "\n";
    }

    cout << "Done. Best Cmax=" << hh.gbest_fit << "\n";
    cout << "Rules: ";
    {
        auto r = build_rules();
        for (int k=0;k<(int)r.size();++k) cout << r[k]->name() << (k+1==(int)r.size()? '\n' : ' ');
    }
    cout << "Gantt written to gantt_" << instance << ".csv\n";
    return 0;
}
