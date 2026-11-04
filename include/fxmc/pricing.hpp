/**
 * @file pricing.hpp
 * @brief FX forwards and European FX options: Monte Carlo and the Garman–Kohlhagen closed form.
 */
#pragma once

#include <span>

#include "fxmc/models.hpp"
#include "fxmc/simulator.hpp"

namespace fxmc {

/// @brief Kind of European option.
enum class OptionType {
    Call,  ///< Right to buy foreign currency at the strike.
    Put    ///< Right to sell foreign currency at the strike.
};

/// @brief A Monte Carlo estimate with its standard error.
struct McEstimate {
    double value{};      ///< Point estimate.
    double std_error{};  ///< Standard error of the estimate.

    /// @brief Lower end of the 95% confidence interval. @return value - 1.96 * std_error.
    [[nodiscard]] double ci_low() const noexcept { return value - 1.96 * std_error; }

    /// @brief Upper end of the 95% confidence interval. @return value + 1.96 * std_error.
    [[nodiscard]] double ci_high() const noexcept { return value + 1.96 * std_error; }
};

/**
 * @brief FX forward rate from covered interest rate parity.
 * @param spot Spot rate.
 * @param domestic_rate Domestic rate @f$r_d@f$ (continuous compounding).
 * @param foreign_rate Foreign rate @f$r_f@f$ (continuous compounding).
 * @param maturity Time to delivery in years.
 * @return @f$S_0 e^{(r_d - r_f)T}@f$.
 */
[[nodiscard]] double forward_rate(double spot, double domestic_rate, double foreign_rate,
                                  double maturity) noexcept;

/**
 * @brief Garman–Kohlhagen price of a European FX option, in domestic currency per unit of foreign.
 *
 * @f$C = S_0 e^{-r_f T} N(d_1) - K e^{-r_d T} N(d_2)@f$,
 * @f$P = K e^{-r_d T} N(-d_2) - S_0 e^{-r_f T} N(-d_1)@f$.
 * With zero volatility or maturity it returns the discounted intrinsic value of the forward.
 *
 * @param type Call or put.
 * @param params Market data: spot, rates and volatility.
 * @param strike Strike rate @f$K > 0@f$.
 * @param maturity Time to expiry in years, >= 0.
 * @return Option premium.
 * @throws std::invalid_argument if strike <= 0 or maturity < 0.
 */
[[nodiscard]] double garman_kohlhagen_price(OptionType type, const GbmParams& params, double strike,
                                            double maturity);

/**
 * @brief Monte Carlo price of a European option from already simulated terminal rates.
 *
 * With @p antithetic_pairs the standard error is computed over pair averages, which is the
 * correct estimator for antithetic variates.
 *
 * @param terminal Terminal rates under the risk-neutral measure.
 * @param type Call or put.
 * @param strike Strike rate.
 * @param discount_factor @f$e^{-r_d T}@f$.
 * @param antithetic_pairs Whether elements 2k and 2k+1 are antithetic pairs.
 * @return Discounted mean payoff and its standard error.
 * @throws std::invalid_argument if @p terminal is empty, or has odd size with antithetic pairs.
 */
[[nodiscard]] McEstimate mc_european_option(std::span<const double> terminal, OptionType type,
                                            double strike, double discount_factor,
                                            bool antithetic_pairs);

/**
 * @brief Monte Carlo price of a European option under the Garman–Kohlhagen model.
 *
 * Simulates terminal rates with @p config (its horizon is the option maturity) and prices them.
 *
 * @param model Risk-neutral GBM model.
 * @param type Call or put.
 * @param strike Strike rate.
 * @param config Simulation settings; @c horizon is the maturity.
 * @return Price estimate and standard error.
 * @throws std::invalid_argument if @p config is invalid.
 */
[[nodiscard]] McEstimate mc_european_option(const GbmModel& model, OptionType type, double strike,
                                            const SimulationConfig& config);

}  // namespace fxmc
