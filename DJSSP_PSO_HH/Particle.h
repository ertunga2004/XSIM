#pragma once

#include "Common.h"

namespace djssp {

struct PSOParams {
    int swarm_size = 15;
    int iters = 30;
    double w_inertia = 0.7;
    double c1 = 1.4;
    double c2 = 1.4;
    double eps0 = 0.25;
    double eps_min = 0.05;
};

struct Particle {
    std::vector<double> x;
    std::vector<double> v;
    std::vector<double> pbest;
    double pbest_fit = 1e300;
};

}  // namespace djssp
