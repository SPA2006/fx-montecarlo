#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

#include "fxmc/models.hpp"
#include "fxmc/simulator.hpp"

using fxmc::GbmModel;
using fxmc::GbmParams;
using fxmc::SimulationConfig;

namespace {

const GbmParams kGbm{100.0, 0.16, 0.04, 0.15};

}  // namespace

TEST(Simulator, SameSeedIsReproducible) {
    const GbmModel model(kGbm);
    const SimulationConfig cfg{1'000, 10, 1.0, 123, false};
    EXPECT_EQ(fxmc::simulate_terminal(model, cfg), fxmc::simulate_terminal(model, cfg));
    SimulationConfig other = cfg;
    other.seed = 124;
    EXPECT_NE(fxmc::simulate_terminal(model, cfg), fxmc::simulate_terminal(model, other));
}

TEST(Simulator, PathsMatchTerminalAndStartAtSpot) {
    const GbmModel model(kGbm);
    const SimulationConfig cfg{100, 52, 1.0, 5, true};
    const auto paths = fxmc::simulate_paths(model, cfg);
    const auto terminal = fxmc::simulate_terminal(model, cfg);
    ASSERT_EQ(paths.paths(), 100U);
    ASSERT_EQ(paths.points(), 53U);
    for (std::size_t i = 0; i < paths.paths(); ++i) {
        EXPECT_DOUBLE_EQ(paths(i, 0), 100.0);
        EXPECT_DOUBLE_EQ(paths(i, 52), terminal[i]);
    }
}

TEST(Simulator, AntitheticPairsMirrorEachOther) {
    const GbmModel model({100.0, 0.0, 0.0, 0.2});  // zero drift: log-returns of a pair are z, -z
    const auto terminal = fxmc::simulate_terminal(model, {1'000, 1, 1.0, 9, true});
    for (std::size_t i = 0; i < terminal.size(); i += 2) {
        const double up = std::log(terminal[i] / 100.0);
        const double down = std::log(terminal[i + 1] / 100.0);
        EXPECT_NEAR(up + down, -0.2 * 0.2, 1e-12);  // both carry the -sigma^2/2 drift
    }
}

TEST(Simulator, RejectsInvalidConfig) {
    const GbmModel model(kGbm);
    EXPECT_THROW((void)fxmc::simulate_terminal(model, {0, 1, 1.0, 1, false}),
                 std::invalid_argument);
    EXPECT_THROW((void)fxmc::simulate_terminal(model, {10, 0, 1.0, 1, false}),
                 std::invalid_argument);
    EXPECT_THROW((void)fxmc::simulate_terminal(model, {10, 1, -1.0, 1, false}),
                 std::invalid_argument);
    EXPECT_THROW((void)fxmc::simulate_terminal(model, {11, 1, 1.0, 1, true}),
                 std::invalid_argument);
}
