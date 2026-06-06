#pragma once

#include "Job.h"
#include "Machine.h"

namespace djssp {

class IRule {
public:
    virtual ~IRule() = default;
    virtual std::string name() const = 0;
    virtual Operation* select(const Machine& machine, const std::vector<Job>& jobs, double t) const = 0;
};

}  // namespace djssp
