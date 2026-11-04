#include "fxmc/statistics.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace fxmc {

namespace {

void require_confidence(double confidence) {
    if (!(confidence > 0.0 && confidence < 1.0)) {
        throw std::invalid_argument("confidence must be in (0, 1)");
    }
}

}

Summary summarize(std::span<const double> sample) {
    if (sample.empty()) {
        throw std::invalid_argument("summarize: empty sample");
    }
    Summary s;
    s.min = sample.front();
    s.max = sample.front();
    double mean = 0.0;
    double m2 = 0.0;
    std::size_t n = 0;
    for (const double x : sample) {
        ++n;
        const double delta = x - mean;
        mean += delta / static_cast<double>(n);
        m2 += delta * (x - mean);
        s.min = std::min(s.min, x);
        s.max = std::max(s.max, x);
    }
    s.count = n;
    s.mean = mean;
    s.stddev = n > 1 ? std::sqrt(m2 / static_cast<double>(n - 1)) : 0.0;
    return s;
}

double quantile(std::span<const double> sample, double p) {
    if (sample.empty()) {
        throw std::invalid_argument("quantile: empty sample");
    }
    if (!(p >= 0.0 && p <= 1.0)) {
        throw std::invalid_argument("quantile: p must be in [0, 1]");
    }
    std::vector<double> sorted(sample.begin(), sample.end());
    std::sort(sorted.begin(), sorted.end());
    const double pos = p * static_cast<double>(sorted.size() - 1);
    const auto lo = static_cast<std::size_t>(std::floor(pos));
    const std::size_t hi = std::min(lo + 1, sorted.size() - 1);
    const double frac = pos - static_cast<double>(lo);
    return sorted[lo] + frac * (sorted[hi] - sorted[lo]);
}

std::vector<double> position_pnl(std::span<const double> terminal, double reference_rate,
                                 double notional) {
    std::vector<double> pnl;
    pnl.reserve(terminal.size());
    for (const double rate : terminal) {
        pnl.push_back(notional * (rate - reference_rate));
    }
    return pnl;
}

double value_at_risk(std::span<const double> pnl, double confidence) {
    require_confidence(confidence);
    return -quantile(pnl, 1.0 - confidence);
}

double expected_shortfall(std::span<const double> pnl, double confidence) {
    require_confidence(confidence);
    const double threshold = quantile(pnl, 1.0 - confidence);
    double sum = 0.0;
    std::size_t count = 0;
    for (const double x : pnl) {
        if (x <= threshold) {
            sum += x;
            ++count;
        }
    }
    return -sum / static_cast<double>(count);  // count >= 1: the minimum is always <= threshold
}

}
