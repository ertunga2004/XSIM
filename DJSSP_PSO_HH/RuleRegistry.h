#pragma once

#include "IRule.h"

namespace djssp {

class RuleRegistry {
public:
    void register_rule(std::unique_ptr<IRule> rule);
    IRule* get(const std::string& name) const;
    std::vector<IRule*> build_active_rules(
        const std::vector<std::string>& names,
        std::string* error_message = nullptr
    ) const;
    std::vector<std::string> names() const;

private:
    std::vector<std::unique_ptr<IRule>> rules_;
};

const RuleRegistry& default_rule_registry();
std::vector<std::string> build_rule_names();
IRule* get_rule_by_name(const std::string& rule_name);
std::vector<IRule*> build_rules();
std::vector<IRule*> build_rules(const std::vector<std::string>& names, std::string* error_message = nullptr);

}  // namespace djssp
