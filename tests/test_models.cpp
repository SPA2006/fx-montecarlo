#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

#include "fxmc/models.hpp"
#include "fxmc/simulator.hpp"
#include "fxmc/statistics.hpp"

using fxmc::GbmModel;
using fxmc::GbmParams;
using fxmc::OuLogModel;
using fxmc::OuLogParams;

namespace {

const GbmParams kGbm{100.0, 0.16, 0.04, 0.15};

std::vector<double> logs(const std::vector<double>& rates) {
    std::vector<double> out;
    out.reserve(rates.size());
    for (const double r : rates) {
        out.push_back(std::log(r));
    }
    return out;
}

}  // namespace

TEST(GbmModel, TerminalMeanMatchesForward) {
    const GbmModel model(kGbm);
    const auto terminal = fxmc::simulate_terminal(model, {200'000, 1, 1.0, 7, false});
    const auto s = fxmc::summarize(terminal);
    const double se = s.stddev / std::sqrt(static_cast<double>(s.count));
    EXPECT_NEAR(s.mean, model.expected_rate(1.0), 4.0 * se);
}

TEST(GbmModel, LogReturnVarianceIsSigmaSquaredT) {
    const GbmModel model(kGbm);
    const auto terminal = fxmc::simulate_terminal(model, {200'000, 12, 2.0, 11, false});
    const auto s = fxmc::summarize(logs(terminal));
    EXPECT_NEAR(s.stddev, 0.15 * std::sqrt(2.0), 0.003);
}

TEST(GbmModel, ZeroVolatilityIsDeterministicForward) {
    const GbmModel model({100.0, 0.16, 0.04, 0.0});
    const auto terminal = fxmc::simulate_terminal(model, {10, 50, 1.5, 1, false});
    for (const double rate : terminal) {
        EXPECT_NEAR(rate, model.expected_rate(1.5), 1e-9);
    }
}

TEST(GbmModel, RejectsInvalidParameters) {
    EXPECT_THROW(GbmModel({0.0, 0.1, 0.0, 0.2}), std::invalid_argument);
    EXPECT_THROW(GbmModel({100.0, 0.1, 0.0, -0.2}), std::invalid_argument);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    EXPECT_THROW(GbmModel({100.0, nan, 0.0, 0.2}), std::invalid_argument);
}

TEST(OuLogModel, LogMomentsMatchClosedForm) {
    const OuLogModel model({100.0, 90.0, 2.0, 0.2});
    const auto terminal = fxmc::simulate_terminal(model, {200'000, 24, 1.0, 3, false});
    const auto s = fxmc::summarize(logs(terminal));
    const double se = s.stddev / std::sqrt(static_cast<double>(s.count));
    EXPECT_NEAR(s.mean, model.log_mean(1.0), 4.0 * se);
    EXPECT_NEAR(s.stddev, std::sqrt(model.log_variance(1.0)), 0.003);
}

TEST(OuLogModel, RevertsToLongRunLevel) {
    const OuLogModel model({120.0, 90.0, 5.0, 0.0});
    const auto terminal = fxmc::simulate_terminal(model, {4, 100, 5.0, 1, false});
    for (const double rate : terminal) {
        EXPECT_NEAR(rate, 90.0, 1e-6);
    }
}

TEST(OuLogModel, RejectsInvalidParameters) {
    EXPECT_THROW(OuLogModel({100.0, 90.0, 0.0, 0.2}), std::invalid_argument);
    EXPECT_THROW(OuLogModel({100.0, -1.0, 1.0, 0.2}), std::invalid_argument);
}
