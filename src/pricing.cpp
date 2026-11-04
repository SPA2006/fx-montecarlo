#include "fxmc/pricing.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <vector>

#include "fxmc/statistics.hpp"

namespace fxmc {

namespace {

double normal_cdf(double x) { return 0.5 * std::erfc(-x / std::numbers::sqrt2); }

double payoff(OptionType type, double rate, double strike) {
    return type == OptionType::Call ? std::max(rate - strike, 0.0) : std::max(strike - rate, 0.0);
}

}  // namespace

double forward_rate(double spot, double domestic_rate, double foreign_rate,
                    double maturity) noexcept {
    return spot * std::exp((domestic_rate - foreign_rate) * maturity);
}

double garman_kohlhagen_price(OptionType type, const GbmParams& params, double strike,
                              double maturity) {
    if (!(strike > 0.0)) {
        throw std::invalid_argument("Garman-Kohlhagen: strike must be positive");
    }
    if (!(maturity >= 0.0)) {
        throw std::invalid_argument("Garman-Kohlhagen: maturity must be non-negative");
    }
    const double df_dom = std::exp(-params.domestic_rate * maturity);
    const double df_for = std::exp(-params.foreign_rate * maturity);
    const double vol_sqrt_t = params.volatility * std::sqrt(maturity);

    if (vol_sqrt_t == 0.0) {
        const double fwd =
            forward_rate(params.spot, params.domestic_rate, params.foreign_rate, maturity);
        return df_dom * payoff(type, fwd, strike);
    }
    const double d1 =
        (std::log(params.spot / strike) + (params.domestic_rate - params.foreign_rate) * maturity) /
            vol_sqrt_t +
        0.5 * vol_sqrt_t;
    const double d2 = d1 - vol_sqrt_t;
    if (type == OptionType::Call) {
        return params.spot * df_for * normal_cdf(d1) - strike * df_dom * normal_cdf(d2);
    }
    return strike * df_dom * normal_cdf(-d2) - params.spot * df_for * normal_cdf(-d1);
}

McEstimate mc_european_option(std::span<const double> terminal, OptionType type, double strike,
                              double discount_factor, bool antithetic_pairs) {
    if (terminal.empty()) {
        throw std::invalid_argument("mc_european_option: no simulated rates");
    }
    if (antithetic_pairs && terminal.size() % 2 != 0) {
        throw std::invalid_argument("mc_european_option: antithetic pairs need an even size");
    }
    std::vector<double> samples;
    if (antithetic_pairs) {
        samples.reserve(terminal.size() / 2);
        for (std::size_t i = 0; i < terminal.size(); i += 2) {
            samples.push_back(
                0.5 * (payoff(type, terminal[i], strike) + payoff(type, terminal[i + 1], strike)));
        }
    } else {
        samples.reserve(terminal.size());
        for (const double rate : terminal) {
            samples.push_back(payoff(type, rate, strike));
        }
    }
    const Summary s = summarize(samples);
    return {discount_factor * s.mean,
            discount_factor * s.stddev / std::sqrt(static_cast<double>(s.count))};
}

McEstimate mc_european_option(const GbmModel& model, OptionType type, double strike,
                              const SimulationConfig& config) {
    const std::vector<double> terminal = simulate_terminal(model, config);
    const double discount = std::exp(-model.params().domestic_rate * config.horizon);
    return mc_european_option(terminal, type, strike, discount, config.antithetic);
}

}  // namespace fxmc
