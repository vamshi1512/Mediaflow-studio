#pragma once

#include <cstdint>
#include <string>

#include "mediaflow/metrics.hpp"

namespace mediaflow {

std::string format_summary(
    const std::string& scenario_name,
    std::uint64_t seed,
    const SummaryMetrics& metrics);

}  // namespace mediaflow
