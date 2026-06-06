#include "RuleRegistry.h"

#include "StateFeatures.h"

namespace djssp {

namespace {

class RuleSPT : public IRule {
public:
    std::string name() const override { return "SPT"; }
    Operation* select(const Machine& machine, const std::vector<Job>&, double) const override {
        Operation* best = nullptr;
        double best_proc = 1e300;
        for (auto* op : machine.ready_q) {
            if (op->proc_time < best_proc) {
                best_proc = op->proc_time;
                best = op;
            }
        }
        return best;
    }
};

class RuleLPT : public IRule {
public:
    std::string name() const override { return "LPT"; }
    Operation* select(const Machine& machine, const std::vector<Job>&, double) const override {
        Operation* best = nullptr;
        double best_proc = -1e300;
        for (auto* op : machine.ready_q) {
            if (op->proc_time > best_proc) {
                best_proc = op->proc_time;
                best = op;
            }
        }
        return best;
    }
};

class RuleMWKR : public IRule {
public:
    std::string name() const override { return "MWKR"; }
    Operation* select(const Machine& machine, const std::vector<Job>& jobs, double) const override {
        Operation* best = nullptr;
        double best_work = -1e300;
        for (auto* op : machine.ready_q) {
            const double work = remaining_work(jobs[op->job_id]);
            if (work > best_work) {
                best_work = work;
                best = op;
            }
        }
        return best;
    }
};

class RuleMOR : public IRule {
public:
    std::string name() const override { return "MOR"; }
    Operation* select(const Machine& machine, const std::vector<Job>& jobs, double) const override {
        Operation* best = nullptr;
        int best_remaining_ops = -1;
        for (auto* op : machine.ready_q) {
            const int remaining_ops = static_cast<int>(jobs[op->job_id].ops.size()) - jobs[op->job_id].next_op;
            if (remaining_ops > best_remaining_ops) {
                best_remaining_ops = remaining_ops;
                best = op;
            }
        }
        return best;
    }
};

class RuleFIFO : public IRule {
public:
    std::string name() const override { return "FIFO"; }
    Operation* select(const Machine& machine, const std::vector<Job>& jobs, double) const override {
        Operation* best = nullptr;
        double best_release = 1e300;
        for (auto* op : machine.ready_q) {
            const double release_time = jobs[op->job_id].release_time;
            if (release_time < best_release) {
                best_release = release_time;
                best = op;
            }
        }
        return best;
    }
};

class RuleSIO : public IRule {
public:
    std::string name() const override { return "SIO"; }
    Operation* select(const Machine& machine, const std::vector<Job>& jobs, double) const override {
        Operation* best = nullptr;
        double best_next_proc = 1e300;
        for (auto* op : machine.ready_q) {
            const auto& job = jobs[op->job_id];
            const int next_index = op->op_index + 1;
            const double next_proc = (next_index < static_cast<int>(job.ops.size()))
                ? job.ops[next_index].proc_time
                : 0.0;
            if (next_proc < best_next_proc) {
                best_next_proc = next_proc;
                best = op;
            }
        }
        return best;
    }
};

class RulePTWINQ : public IRule {
public:
    std::string name() const override { return "PT+WINQ"; }
    Operation* select(const Machine& machine, const std::vector<Job>& jobs, double) const override {
        Operation* best = nullptr;
        double best_score = 1e300;
        for (auto* op : machine.ready_q) {
            const auto& job = jobs[op->job_id];
            const int next_index = op->op_index + 1;
            const double next_proc = (next_index < static_cast<int>(job.ops.size()))
                ? job.ops[next_index].proc_time
                : 0.0;
            const double score = op->proc_time + next_proc;
            if (score < best_score) {
                best_score = score;
                best = op;
            }
        }
        return best;
    }
};

RuleRegistry make_default_rule_registry() {
    RuleRegistry registry;
    registry.register_rule(std::make_unique<RuleSPT>());
    registry.register_rule(std::make_unique<RuleLPT>());
    registry.register_rule(std::make_unique<RuleMWKR>());
    registry.register_rule(std::make_unique<RuleMOR>());
    registry.register_rule(std::make_unique<RuleFIFO>());
    registry.register_rule(std::make_unique<RuleSIO>());
    registry.register_rule(std::make_unique<RulePTWINQ>());
    return registry;
}

}  // namespace

void RuleRegistry::register_rule(std::unique_ptr<IRule> rule) {
    if (rule != nullptr) {
        rules_.push_back(std::move(rule));
    }
}

IRule* RuleRegistry::get(const std::string& name) const {
    for (const auto& rule : rules_) {
        if (rule != nullptr && rule->name() == name) {
            return rule.get();
        }
    }
    return nullptr;
}

std::vector<IRule*> RuleRegistry::build_active_rules(
    const std::vector<std::string>& names,
    std::string* error_message
) const {
    std::vector<IRule*> active_rules;
    active_rules.reserve(names.size());
    for (const auto& name : names) {
        IRule* rule = get(name);
        if (rule == nullptr) {
            if (error_message != nullptr) {
                *error_message = "Unknown rule name in active rule set: " + name;
            }
            return {};
        }
        active_rules.push_back(rule);
    }
    if (active_rules.empty() && error_message != nullptr) {
        *error_message = "Active rule set must contain at least one rule.";
    }
    return active_rules;
}

std::vector<std::string> RuleRegistry::names() const {
    std::vector<std::string> result;
    result.reserve(rules_.size());
    for (const auto& rule : rules_) {
        if (rule != nullptr) {
            result.push_back(rule->name());
        }
    }
    return result;
}

const RuleRegistry& default_rule_registry() {
    static const RuleRegistry registry = make_default_rule_registry();
    return registry;
}

std::vector<std::string> build_rule_names() {
    return default_rule_registry().names();
}

IRule* get_rule_by_name(const std::string& rule_name) {
    return default_rule_registry().get(rule_name);
}

std::vector<IRule*> build_rules() {
    return default_rule_registry().build_active_rules(build_rule_names());
}

std::vector<IRule*> build_rules(const std::vector<std::string>& names, std::string* error_message) {
    return default_rule_registry().build_active_rules(names, error_message);
}

}  // namespace djssp
