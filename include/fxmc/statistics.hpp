/**
 * @file statistics.hpp
 * @brief Descriptive statistics and risk measures over simulated samples.
 */
#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace fxmc {

/// @brief Basic sample statistics.
struct Summary {
    std::size_t count{};  ///< Sample size.
    double mean{};        ///< Arithmetic mean.
    double stddev{};      ///< Sample standard deviation (n - 1 denominator; 0 if n = 1).
    double min{};         ///< Smallest value.
    double max{};         ///< Largest value.
};

/**
 * @brief Computes mean, standard deviation and range in one pass (Welford's algorithm).
 * @param sample Values.
 * @return Summary of @p sample.
 * @throws std::invalid_argument if @p sample is empty.
 */
[[nodiscard]] Summary summarize(std::span<const double> sample);

/**
 * @brief Empirical quantile with linear interpolation between order statistics.
 * @param sample Values (not modified).
 * @param p Probability in [0, 1].
 * @return The @p p-quantile of @p sample.
 * @throws std::invalid_argument if @p sample is empty or @p p is outside [0, 1].
 */
[[nodiscard]] double quantile(std::span<const double> sample, double p);

/**
 * @brief Profit and loss of a foreign-currency position, in domestic currency.
 *
 * Choose @p reference_rate to match the question. Against the forward rate, the P&L is the result
 * of leaving the position unhedged instead of locking the forward; under a risk-neutral model its
 * mean is zero, so risk measures reflect only uncertainty. Against today's spot, the P&L also
 * contains the model's drift, which for a risk-neutral model is the interest-rate differential,
 * not a forecast.
 *
 * @param terminal Simulated terminal rates.
 * @param reference_rate Rate the P&L is measured against (e.g. the forward, or today's spot).
 * @param notional Amount of foreign currency (positive = long, negative = short).
 * @return @f$\text{notional}\cdot(S_T - \text{reference\_rate})@f$ for every scenario.
 */
[[nodiscard]] std::vector<double> position_pnl(std::span<const double> terminal,
                                               double reference_rate, double notional);

/**
 * @brief Value at Risk: the loss not exceeded with probability @p confidence.
 * @param pnl Simulated profit-and-loss scenarios (losses negative).
 * @param confidence Confidence level in (0, 1), e.g. 0.99.
 * @return VaR as a positive number when the tail is a loss: @f$-q_{1-c}(\text{PnL})@f$.
 * @throws std::invalid_argument if @p pnl is empty or @p confidence is outside (0, 1).
 */
[[nodiscard]] double value_at_risk(std::span<const double> pnl, double confidence);

/**
 * @brief Expected Shortfall: the average loss in the worst @f$(1-c)@f$ share of scenarios.
 * @param pnl Simulated profit-and-loss scenarios (losses negative).
 * @param confidence Confidence level in (0, 1), e.g. 0.975.
 * @return ES as a positive number when the tail is a loss; always >= ::fxmc::value_at_risk.
 * @throws std::invalid_argument if @p pnl is empty or @p confidence is outside (0, 1).
 */
[[nodiscard]] double expected_shortfall(std::span<const double> pnl, double confidence);

}  // namespace fxmc
