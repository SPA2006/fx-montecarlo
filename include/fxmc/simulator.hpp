/**
 * @file simulator.hpp
 * @brief Monte Carlo path generation for any ::fxmc::FxModel.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <span>
#include <vector>

#include "fxmc/models.hpp"

namespace fxmc {

/**
 * @brief Settings of a Monte Carlo run.
 *
 * Runs are reproducible: the same model, config and standard library produce identical paths.
 */
struct SimulationConfig {
    std::size_t paths = 100'000;  ///< Number of simulated paths (> 0; even if antithetic).
    std::size_t steps = 1;        ///< Time steps per path (> 0).
    double horizon = 1.0;         ///< Simulation horizon in years (> 0).
    std::uint64_t seed = 42;      ///< Seed of the random number generator.
    /// Antithetic variates: paths come in pairs driven by @f$Z@f$ and @f$-Z@f$.
    bool antithetic = false;

    /**
     * @brief Checks the settings.
     * @throws std::invalid_argument if any setting is out of range.
     */
    void validate() const;
};

/**
 * @brief Dense row-major matrix of simulated paths.
 *
 * Row @c i is path @c i; column @c j is the rate at time @f$j \cdot \text{horizon}/\text{steps}@f$,
 * so column 0 holds the spot.
 */
class PathMatrix {
public:
    /**
     * @brief Allocates a zero-filled matrix.
     * @param paths Number of rows.
     * @param points Number of columns (steps + 1).
     */
    PathMatrix(std::size_t paths, std::size_t points)
        : paths_(paths), points_(points), data_(paths * points) {}

    /// @brief Number of paths. @return Row count.
    [[nodiscard]] std::size_t paths() const noexcept { return paths_; }

    /// @brief Number of time points per path. @return Column count (steps + 1).
    [[nodiscard]] std::size_t points() const noexcept { return points_; }

    /**
     * @brief Element access.
     * @param path Row index.
     * @param point Column index.
     * @return Reference to the rate of @p path at time point @p point.
     */
    double& operator()(std::size_t path, std::size_t point) noexcept {
        return data_[path * points_ + point];
    }

    /**
     * @brief Read-only element access.
     * @param path Row index.
     * @param point Column index.
     * @return The rate of @p path at time point @p point.
     */
    [[nodiscard]] double operator()(std::size_t path, std::size_t point) const noexcept {
        return data_[path * points_ + point];
    }

    /**
     * @brief One whole path.
     * @param path Row index.
     * @return View of the path's rates, from spot to horizon.
     */
    [[nodiscard]] std::span<const double> path(std::size_t path) const noexcept {
        return {data_.data() + path * points_, points_};
    }

private:
    std::size_t paths_;
    std::size_t points_;
    std::vector<double> data_;
};

namespace detail {

/// Shared simulation loop; @p record(path, point, rate) receives every generated value.
template <FxModel M, typename Record>
void simulate(const M& model, const SimulationConfig& config, Record record) {
    config.validate();
    const double dt = config.horizon / static_cast<double>(config.steps);
    const auto step = model.stepper(dt);
    std::mt19937_64 rng(config.seed);
    std::normal_distribution<double> normal(0.0, 1.0);
    const double spot = model.spot();

    if (!config.antithetic) {
        for (std::size_t i = 0; i < config.paths; ++i) {
            double rate = spot;
            record(i, 0, rate);
            for (std::size_t j = 1; j <= config.steps; ++j) {
                rate = step(rate, normal(rng));
                record(i, j, rate);
            }
        }
        return;
    }
    for (std::size_t i = 0; i < config.paths; i += 2) {
        double up = spot;
        double down = spot;
        record(i, 0, up);
        record(i + 1, 0, down);
        for (std::size_t j = 1; j <= config.steps; ++j) {
            const double z = normal(rng);
            up = step(up, z);
            down = step(down, -z);
            record(i, j, up);
            record(i + 1, j, down);
        }
    }
}

}  // namespace detail

/**
 * @brief Simulates paths and keeps only the rate at the horizon.
 *
 * Memory is O(paths), independent of the number of steps.
 * With antithetic variates, elements @c 2k and @c 2k+1 form a pair.
 *
 * @tparam M Model type satisfying ::fxmc::FxModel.
 * @param model Exchange-rate model.
 * @param config Run settings.
 * @return Terminal rates, one per path.
 * @throws std::invalid_argument if @p config is invalid.
 */
template <FxModel M>
[[nodiscard]] std::vector<double> simulate_terminal(const M& model,
                                                    const SimulationConfig& config) {
    config.validate();
    std::vector<double> terminal(config.paths);
    detail::simulate(model, config, [&](std::size_t i, std::size_t j, double rate) {
        if (j == config.steps) {
            terminal[i] = rate;
        }
    });
    return terminal;
}

/**
 * @brief Simulates and stores complete paths.
 *
 * Uses the same random stream as ::fxmc::simulate_terminal, so the last column equals its output.
 *
 * @tparam M Model type satisfying ::fxmc::FxModel.
 * @param model Exchange-rate model.
 * @param config Run settings.
 * @return Matrix of size paths x (steps + 1).
 * @throws std::invalid_argument if @p config is invalid.
 */
template <FxModel M>
[[nodiscard]] PathMatrix simulate_paths(const M& model, const SimulationConfig& config) {
    config.validate();
    PathMatrix out(config.paths, config.steps + 1);
    detail::simulate(model, config,
                     [&](std::size_t i, std::size_t j, double rate) { out(i, j) = rate; });
    return out;
}

}  // namespace fxmc
