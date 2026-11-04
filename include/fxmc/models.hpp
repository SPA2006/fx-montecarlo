/**
 * @file models.hpp
 * @brief Stochastic models of an exchange rate with exact one-step transitions.
 *
 * An exchange rate @f$S_t@f$ is quoted as units of domestic currency per one unit of foreign
 * currency (e.g. RUB per USD). Each model exposes the current spot and a @c stepper(dt) that maps
 * the rate at time @f$t@f$ and a standard normal draw @f$Z@f$ to the rate at @f$t+dt@f$.
 * All schemes are exact in distribution, so the time step only controls path resolution,
 * never introduces discretisation bias.
 */
#pragma once

#include <cmath>
#include <concepts>

namespace fxmc {

/**
 * @brief A callable advancing the rate by one fixed time step.
 *
 * Signature: @c double(double rate, double z), where @c z is a standard normal draw.
 */
template <typename S>
concept Stepper = requires(const S& step, double rate, double z) {
    { step(rate, z) } -> std::convertible_to<double>;
};

/**
 * @brief Requirements for an exchange-rate model used by the simulator.
 *
 * A model provides @c spot() (the initial rate) and @c stepper(dt) returning a ::fxmc::Stepper
 * with all per-step constants precomputed.
 */
template <typename M>
concept FxModel = requires(const M& model, double dt) {
    { model.spot() } -> std::convertible_to<double>;
    { model.stepper(dt) } -> Stepper;
};

/// @brief Parameters of the Garman–Kohlhagen (geometric Brownian motion) model.
struct GbmParams {
    double spot{};           ///< Initial rate @f$S_0 > 0@f$.
    double domestic_rate{};  ///< Continuously compounded domestic risk-free rate @f$r_d@f$.
    double foreign_rate{};   ///< Continuously compounded foreign risk-free rate @f$r_f@f$.
    double volatility{};     ///< Annualised volatility @f$\sigma \ge 0@f$.
};

/**
 * @brief Garman–Kohlhagen model: risk-neutral GBM with drift @f$r_d - r_f@f$.
 *
 * @f[ dS_t = (r_d - r_f) S_t\,dt + \sigma S_t\,dW_t, @f]
 * simulated exactly via
 * @f$S_{t+\Delta} = S_t \exp\big((r_d - r_f - \tfrac12\sigma^2)\Delta +
 * \sigma\sqrt{\Delta}\,Z\big)@f$.
 */
class GbmModel {
public:
    /// @brief Precomputed one-step transition of ::fxmc::GbmModel.
    struct Step {
        double drift;      ///< @f$(r_d - r_f - \sigma^2/2)\Delta@f$.
        double diffusion;  ///< @f$\sigma\sqrt{\Delta}@f$.

        /**
         * @brief Advances the rate by one step.
         * @param rate Current rate.
         * @param z Standard normal draw.
         * @return Rate after one step.
         */
        double operator()(double rate, double z) const noexcept {
            return rate * std::exp(drift + diffusion * z);
        }
    };

    /**
     * @brief Creates the model.
     * @param params Model parameters.
     * @throws std::invalid_argument if spot <= 0, volatility < 0 or any value is not finite.
     */
    explicit GbmModel(const GbmParams& params);

    /// @brief Initial rate. @return @f$S_0@f$.
    [[nodiscard]] double spot() const noexcept { return params_.spot; }

    /// @brief Model parameters. @return The parameters passed to the constructor.
    [[nodiscard]] const GbmParams& params() const noexcept { return params_; }

    /**
     * @brief Risk-neutral expectation of the rate, equal to the FX forward.
     * @param t Time in years.
     * @return @f$S_0 e^{(r_d - r_f)t}@f$.
     */
    [[nodiscard]] double expected_rate(double t) const noexcept;

    /**
     * @brief Builds the exact one-step transition for a fixed step.
     * @param dt Step length in years, > 0.
     * @return Callable advancing the rate by @p dt.
     */
    [[nodiscard]] Step stepper(double dt) const noexcept;

private:
    GbmParams params_;
};

/// @brief Parameters of the mean-reverting log-rate (exponential Ornstein–Uhlenbeck) model.
struct OuLogParams {
    double spot{};            ///< Initial rate @f$S_0 > 0@f$.
    double long_run_level{};  ///< Rate the process reverts to, @f$L > 0@f$ (so @f$\mu = \ln L@f$).
    double mean_reversion{};  ///< Speed of mean reversion @f$\kappa > 0@f$, per year.
    double volatility{};      ///< Volatility of the log-rate @f$\sigma \ge 0@f$.
};

/**
 * @brief Exponential Ornstein–Uhlenbeck model: the log-rate reverts to a long-run level.
 *
 * With @f$X_t = \ln S_t@f$ and @f$\mu = \ln L@f$:
 * @f[ dX_t = \kappa(\mu - X_t)\,dt + \sigma\,dW_t, @f]
 * simulated exactly via
 * @f$X_{t+\Delta} = \mu + (X_t - \mu)e^{-\kappa\Delta}
 *   + \sigma\sqrt{(1 - e^{-2\kappa\Delta})/(2\kappa)}\,Z@f$.
 * This is a real-world (not risk-neutral) model, suited to scenario and risk analysis.
 */
class OuLogModel {
public:
    /// @brief Precomputed one-step transition of ::fxmc::OuLogModel.
    struct Step {
        double mu;     ///< Long-run log level @f$\mu@f$.
        double decay;  ///< @f$e^{-\kappa\Delta}@f$.
        double sd;     ///< Conditional standard deviation of the log-rate over one step.

        /**
         * @brief Advances the rate by one step.
         * @param rate Current rate (> 0).
         * @param z Standard normal draw.
         * @return Rate after one step.
         */
        double operator()(double rate, double z) const noexcept {
            return std::exp(mu + (std::log(rate) - mu) * decay + sd * z);
        }
    };

    /**
     * @brief Creates the model.
     * @param params Model parameters.
     * @throws std::invalid_argument if spot or level <= 0, mean reversion <= 0,
     *         volatility < 0, or any value is not finite.
     */
    explicit OuLogModel(const OuLogParams& params);

    /// @brief Initial rate. @return @f$S_0@f$.
    [[nodiscard]] double spot() const noexcept { return params_.spot; }

    /// @brief Model parameters. @return The parameters passed to the constructor.
    [[nodiscard]] const OuLogParams& params() const noexcept { return params_; }

    /**
     * @brief Mean of the log-rate at time @p t.
     * @param t Time in years.
     * @return @f$\mu + (\ln S_0 - \mu)e^{-\kappa t}@f$.
     */
    [[nodiscard]] double log_mean(double t) const noexcept;

    /**
     * @brief Variance of the log-rate at time @p t.
     * @param t Time in years.
     * @return @f$\sigma^2 (1 - e^{-2\kappa t}) / (2\kappa)@f$.
     */
    [[nodiscard]] double log_variance(double t) const noexcept;

    /**
     * @brief Builds the exact one-step transition for a fixed step.
     * @param dt Step length in years, > 0.
     * @return Callable advancing the rate by @p dt.
     */
    [[nodiscard]] Step stepper(double dt) const noexcept;

private:
    OuLogParams params_;
};

}  // namespace fxmc
