#include "fxmc/simulator.hpp"

#include <cmath>
#include <stdexcept>

namespace fxmc {

void SimulationConfig::validate() const {
    if (paths == 0) {
        throw std::invalid_argument("simulation: paths must be positive");
    }
    if (steps == 0) {
        throw std::invalid_argument("simulation: steps must be positive");
    }
    if (!std::isfinite(horizon) || horizon <= 0.0) {
        throw std::invalid_argument("simulation: horizon must be positive");
    }
    if (antithetic && paths % 2 != 0) {
        throw std::invalid_argument("simulation: antithetic variates need an even number of paths");
    }
}

}  // namespace fxmc
