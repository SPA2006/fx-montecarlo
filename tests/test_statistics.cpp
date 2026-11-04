#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>
#include <vector>

#include "fxmc/statistics.hpp"

TEST(Statistics, SummaryOfKnownSample) {
    const std::vector<double> x{2, 4, 4, 4, 5, 5, 7, 9};
    const auto s = fxmc::summarize(x);
    EXPECT_EQ(s.count, 8U);
    EXPECT_DOUBLE_EQ(s.mean, 5.0);
    EXPECT_NEAR(s.stddev, std::sqrt(32.0 / 7.0), 1e-12);
    EXPECT_DOUBLE_EQ(s.min, 2.0);
    EXPECT_DOUBLE_EQ(s.max, 9.0);
}

TEST(Statistics, QuantileInterpolatesLinearly) {
    const std::vector<double> x{5, 1, 4, 2, 3};
    EXPECT_DOUBLE_EQ(fxmc::quantile(x, 0.0), 1.0);
    EXPECT_DOUBLE_EQ(fxmc::quantile(x, 0.5), 3.0);
    EXPECT_DOUBLE_EQ(fxmc::quantile(x, 1.0), 5.0);
    EXPECT_DOUBLE_EQ(fxmc::quantile(x, 0.125), 1.5);
}

TEST(Statistics, VarAndExpectedShortfall) {
    std::vector<double> pnl;
    for (int i = -50; i <= 49; ++i) {
        pnl.push_back(i);
    }
    const double var95 = fxmc::value_at_risk(pnl, 0.95);
    const double es95 = fxmc::expected_shortfall(pnl, 0.95);
    EXPECT_NEAR(var95, 45.05, 1e-9);  // -quantile(0.05) = -(-50 + 0.05 * 99)
    EXPECT_NEAR(es95, 48.0, 1e-9);    // mean of the five worst: -50..-46
    EXPECT_GE(es95, var95);
}

TEST(Statistics, PositionPnlScalesWithNotional) {
    const std::vector<double> terminal{90.0, 100.0, 115.0};
    const auto pnl = fxmc::position_pnl(terminal, 100.0, 1'000.0);
    EXPECT_EQ(pnl, (std::vector<double>{-10'000.0, 0.0, 15'000.0}));
}

TEST(Statistics, RejectsInvalidInput) {
    const std::vector<double> empty;
    const std::vector<double> x{1.0, 2.0};
    EXPECT_THROW((void)fxmc::summarize(empty), std::invalid_argument);
    EXPECT_THROW((void)fxmc::quantile(x, 1.5), std::invalid_argument);
    EXPECT_THROW((void)fxmc::value_at_risk(x, 1.0), std::invalid_argument);
}
