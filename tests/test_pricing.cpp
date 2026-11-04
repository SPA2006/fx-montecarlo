#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

#include "fxmc/pricing.hpp"

using fxmc::GbmModel;
using fxmc::GbmParams;
using fxmc::OptionType;

namespace {

// Hull, "Options, Futures, and Other Derivatives": 4-month option on GBP/USD.
const GbmParams kHull{1.60, 0.08, 0.11, 0.141};
constexpr double kHullT = 4.0 / 12.0;

// Illustrative parameters used in the demo (not market data).
const GbmParams kDemo{100.0, 0.16, 0.04, 0.15};

}  // namespace

TEST(Pricing, GarmanKohlhagenMatchesReferenceValues) {
    // References computed independently with mpmath at 30 significant digits.
    EXPECT_NEAR(fxmc::garman_kohlhagen_price(OptionType::Call, kHull, 1.60, kHullT),
                0.0429577301925958, 1e-12);
    EXPECT_NEAR(fxmc::garman_kohlhagen_price(OptionType::Put, kHull, 1.60, kHullT),
                0.0584590663240032, 1e-12);
    EXPECT_NEAR(fxmc::garman_kohlhagen_price(OptionType::Call, kDemo, 110.0, 1.0), 6.92217008716207,
                1e-10);
    EXPECT_NEAR(fxmc::garman_kohlhagen_price(OptionType::Put, kDemo, 110.0, 1.0), 4.57904295821300,
                1e-10);
}

TEST(Pricing, PutCallParityHoldsInClosedForm) {
    const double k = 105.0;
    const double t = 0.75;
    const double call = fxmc::garman_kohlhagen_price(OptionType::Call, kDemo, k, t);
    const double put = fxmc::garman_kohlhagen_price(OptionType::Put, kDemo, k, t);
    const double rhs =
        kDemo.spot * std::exp(-kDemo.foreign_rate * t) - k * std::exp(-kDemo.domestic_rate * t);
    EXPECT_NEAR(call - put, rhs, 1e-10);
}

TEST(Pricing, ZeroVolatilityGivesDiscountedForwardIntrinsic) {
    const GbmParams p{100.0, 0.16, 0.04, 0.0};
    const double fwd = fxmc::forward_rate(100.0, 0.16, 0.04, 1.0);
    EXPECT_NEAR(fxmc::garman_kohlhagen_price(OptionType::Call, p, 100.0, 1.0),
                std::exp(-0.16) * (fwd - 100.0), 1e-12);
    EXPECT_DOUBLE_EQ(fxmc::garman_kohlhagen_price(OptionType::Put, p, 100.0, 1.0), 0.0);
}

TEST(Pricing, MonteCarloAgreesWithClosedForm) {
    const GbmModel model(kDemo);
    for (const auto type : {OptionType::Call, OptionType::Put}) {
        const auto mc = fxmc::mc_european_option(model, type, 110.0, {400'000, 1, 1.0, 2024, true});
        const double exact = fxmc::garman_kohlhagen_price(type, kDemo, 110.0, 1.0);
        EXPECT_NEAR(mc.value, exact, 4.0 * mc.std_error);
        EXPECT_LT(mc.std_error, 0.02);
    }
}

TEST(Pricing, AntitheticVariatesReduceStandardError) {
    const GbmModel model(kDemo);
    const auto plain =
        fxmc::mc_european_option(model, OptionType::Call, 100.0, {200'000, 1, 1.0, 1, false});
    const auto anti =
        fxmc::mc_european_option(model, OptionType::Call, 100.0, {200'000, 1, 1.0, 1, true});
    EXPECT_LT(anti.std_error, plain.std_error);
}

TEST(Pricing, RejectsInvalidInput) {
    EXPECT_THROW((void)fxmc::garman_kohlhagen_price(OptionType::Call, kDemo, 0.0, 1.0),
                 std::invalid_argument);
    EXPECT_THROW((void)fxmc::garman_kohlhagen_price(OptionType::Call, kDemo, 100.0, -1.0),
                 std::invalid_argument);
}
