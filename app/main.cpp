/**
 * @file main.cpp
 * @brief Demo: simulates an exchange rate, reports its distribution, the risk of an FX
 *        position and the price of an option, Monte Carlo against the closed form.
 *
 * Parameters are illustrative, not market data.
 */
#include <cstdio>

#include "fxmc/models.hpp"
#include "fxmc/pricing.hpp"
#include "fxmc/simulator.hpp"
#include "fxmc/statistics.hpp"

/**
 * @brief Demo entry point.
 */
int main() {
    // Illustrative USD/RUB-style setup: RUB per 1 USD, domestic = RUB, foreign = USD.
    const fxmc::GbmParams params{100.0, 0.16, 0.04, 0.15};
    const fxmc::GbmModel model(params);
    const fxmc::SimulationConfig config{200'000, 252, 1.0, 42, true};

    std::printf("Model: Garman-Kohlhagen GBM, S0 = %.2f, r_d = %.2f, r_f = %.2f, sigma = %.2f\n",
                params.spot, params.domestic_rate, params.foreign_rate, params.volatility);
    std::printf("Run:   %zu paths x %zu daily steps, horizon %.1f y, antithetic\n\n", config.paths,
                config.steps, config.horizon);

    const auto terminal = fxmc::simulate_terminal(model, config);
    const auto s = fxmc::summarize(terminal);
    std::printf("Rate in 1 year\n");
    std::printf("  mean %.3f (forward %.3f), std %.3f\n", s.mean, model.expected_rate(1.0),
                s.stddev);
    std::printf("  5%% quantile %.3f, median %.3f, 95%% quantile %.3f\n\n",
                fxmc::quantile(terminal, 0.05), fxmc::quantile(terminal, 0.5),
                fxmc::quantile(terminal, 0.95));

    // An importer must pay 1M USD in one year. Measured against the forward, the P&L is the
    // result of staying unhedged instead of locking the forward today; its mean is ~0, so the
    // risk numbers reflect uncertainty only, not the risk-neutral drift.
    const double notional = -1'000'000.0;  // short USD
    const double forward = model.expected_rate(1.0);
    const auto pnl = fxmc::position_pnl(terminal, forward, notional);
    std::printf("Importer paying 1,000,000 USD in 1 year, unhedged vs the forward (RUB)\n");
    std::printf("  mean P&L %14.0f\n", fxmc::summarize(pnl).mean);
    std::printf("  VaR 99%%  %14.0f\n", fxmc::value_at_risk(pnl, 0.99));
    std::printf("  ES 97.5%% %14.0f\n\n", fxmc::expected_shortfall(pnl, 0.975));

    const double strike = 110.0;
    const auto mc =
        fxmc::mc_european_option(model, fxmc::OptionType::Call, strike, {200'000, 1, 1.0, 7, true});
    const double exact = fxmc::garman_kohlhagen_price(fxmc::OptionType::Call, params, strike, 1.0);
    std::printf("1-year call, strike %.0f\n", strike);
    std::printf("  Monte Carlo      %.4f  (95%% CI %.4f .. %.4f)\n", mc.value, mc.ci_low(),
                mc.ci_high());
    std::printf("  Garman-Kohlhagen %.4f\n", exact);
    return 0;
}
