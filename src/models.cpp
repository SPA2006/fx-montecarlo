#include "fxmc/models.hpp"

#include <cmath>
#include <stdexcept>

namespace fxmc {

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::invalid_argument(message);
    }
}

}

GbmModel::GbmModel(const GbmParams& params) : params_(params) {
    require(std::isfinite(params.spot) && params.spot > 0.0, "GBM: spot must be positive");
    require(std::isfinite(params.domestic_rate), "GBM: domestic rate must be finite");
    require(std::isfinite(params.foreign_rate), "GBM: foreign rate must be finite");
    require(std::isfinite(params.volatility) && params.volatility >= 0.0,
            "GBM: volatility must be non-negative");
}

double GbmModel::expected_rate(double t) const noexcept {
    return params_.spot * std::exp((params_.domestic_rate - params_.foreign_rate) * t);
}

GbmModel::Step GbmModel::stepper(double dt) const noexcept {
    const double sigma = params_.volatility;
    return Step{(params_.domestic_rate - params_.foreign_rate - 0.5 * sigma * sigma) * dt,
                sigma * std::sqrt(dt)};
}

OuLogModel::OuLogModel(const OuLogParams& params) : params_(params) {
    require(std::isfinite(params.spot) && params.spot > 0.0, "OU: spot must be positive");
    require(std::isfinite(params.long_run_level) && params.long_run_level > 0.0,
            "OU: long-run level must be positive");
    require(std::isfinite(params.mean_reversion) && params.mean_reversion > 0.0,
            "OU: mean reversion must be positive");
    require(std::isfinite(params.volatility) && params.volatility >= 0.0,
            "OU: volatility must be non-negative");
}

double OuLogModel::log_mean(double t) const noexcept {
    const double mu = std::log(params_.long_run_level);
    return mu + (std::log(params_.spot) - mu) * std::exp(-params_.mean_reversion * t);
}

double OuLogModel::log_variance(double t) const noexcept {
    const double k = params_.mean_reversion;
    const double s = params_.volatility;
    return s * s * (1.0 - std::exp(-2.0 * k * t)) / (2.0 * k);
}

OuLogModel::Step OuLogModel::stepper(double dt) const noexcept {
    const double k = params_.mean_reversion;
    const double s = params_.volatility;
    return Step{std::log(params_.long_run_level), std::exp(-k * dt),
                s * std::sqrt((1.0 - std::exp(-2.0 * k * dt)) / (2.0 * k))};
}

}
