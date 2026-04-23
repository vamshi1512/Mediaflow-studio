#include "mediaflow/logger.hpp"

#include <iomanip>
#include <sstream>

namespace mediaflow {

std::string format_summary(
    const std::string& scenario_name,
    std::uint64_t seed,
    const SummaryMetrics& metrics) {
  std::ostringstream output;
  output << "MediaFlow Studio Summary\n";
  output << "Scenario: " << scenario_name << '\n';
  output << "Seed: " << seed << '\n';
  output << "Duration: " << metrics.duration_ms << " ms\n";
  output << "Sent packets: " << metrics.sent_packets << '\n';
  output << "Received packets: " << metrics.received_packets << '\n';
  output << "Played packets: " << metrics.played_packets << '\n';
  output << "Dropped packets: " << metrics.dropped_packets << '\n';
  output << "Reordered packets: " << metrics.reorder_count << '\n';
  output << "Underflows: " << metrics.underflow_count << '\n';
  output << std::fixed << std::setprecision(2);
  output << "Average latency: " << metrics.avg_latency_ms << " ms\n";
  output << "P95 latency: " << metrics.p95_latency_ms << " ms\n";
  output << "Jitter estimate: " << metrics.jitter_estimate_ms << " ms\n";
  output << "Packet loss: " << metrics.packet_loss_rate_pct << "%\n";
  output << "Throughput: " << metrics.throughput_kbps << " kbps\n";
  output << "Playout smoothness: " << metrics.playout_smoothness_pct << "%\n";
  return output.str();
}

}  // namespace mediaflow
