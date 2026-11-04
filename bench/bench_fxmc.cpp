#include <benchmark/benchmark.h>

#include <cstddef>

#include "fxmc/models.hpp"
#include "fxmc/pricing.hpp"
#include "fxmc/simulator.hpp"

namespace {

const fxmc::GbmParams kGbm{100.0, 0.16, 0.04, 0.15};
const fxmc::OuLogParams kOu{100.0, 90.0, 2.0, 0.15};

}  // namespace

/// Terminal rates for N single-step GBM paths: cost per path.
static void BM_GbmTerminal(benchmark::State& state) {
    const fxmc::GbmModel model(kGbm);
    const fxmc::SimulationConfig cfg{static_cast<std::size_t>(state.range(0)), 1, 1.0, 42, false};
    for (auto _ : state) {
        benchmark::DoNotOptimize(fxmc::simulate_terminal(model, cfg));
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_GbmTerminal)
    ->RangeMultiplier(10)
    ->Range(10'000, 1'000'000)
    ->Unit(benchmark::kMillisecond);

/// 10,000 full GBM paths with weekly (52) or daily (252) steps: cost per simulated step.
static void BM_GbmPaths(benchmark::State& state) {
    const fxmc::GbmModel model(kGbm);
    const auto steps = static_cast<std::size_t>(state.range(0));
    const fxmc::SimulationConfig cfg{10'000, steps, 1.0, 42, false};
    for (auto _ : state) {
        benchmark::DoNotOptimize(fxmc::simulate_paths(model, cfg));
    }
    state.SetItemsProcessed(state.iterations() * 10'000 * state.range(0));
}
BENCHMARK(BM_GbmPaths)->Arg(52)->Arg(252)->Unit(benchmark::kMillisecond);

/// Same as BM_GbmPaths for the mean-reverting model (one extra log per step).
static void BM_OuPaths(benchmark::State& state) {
    const fxmc::OuLogModel model(kOu);
    const auto steps = static_cast<std::size_t>(state.range(0));
    const fxmc::SimulationConfig cfg{10'000, steps, 1.0, 42, false};
    for (auto _ : state) {
        benchmark::DoNotOptimize(fxmc::simulate_paths(model, cfg));
    }
    state.SetItemsProcessed(state.iterations() * 10'000 * state.range(0));
}
BENCHMARK(BM_OuPaths)->Arg(52)->Arg(252)->Unit(benchmark::kMillisecond);

/// Pricing a call with 1M paths, plain (0) vs antithetic (1); reports the standard error.
static void BM_PriceCall(benchmark::State& state) {
    const fxmc::GbmModel model(kGbm);
    const bool antithetic = state.range(0) != 0;
    const fxmc::SimulationConfig cfg{1'000'000, 1, 1.0, 42, antithetic};
    fxmc::McEstimate est{};
    for (auto _ : state) {
        est = fxmc::mc_european_option(model, fxmc::OptionType::Call, 110.0, cfg);
        benchmark::DoNotOptimize(est);
    }
    state.counters["price"] = est.value;
    state.counters["std_error"] = est.std_error;
}
BENCHMARK(BM_PriceCall)->ArgName("antithetic")->Arg(0)->Arg(1)->Unit(benchmark::kMillisecond);
