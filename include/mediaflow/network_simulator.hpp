#pragma once

#include <cstdint>
#include <optional>
#include <queue>
#include <vector>

#include "mediaflow/config.hpp"
#include "mediaflow/random.hpp"
#include "mediaflow/types.hpp"

namespace mediaflow {

struct NetworkSubmitResult {
  bool accepted {false};
  bool reordered_hold_applied {false};
  std::optional<std::string> drop_reason;
};

class NetworkSimulator {
 public:
  NetworkSimulator(const NetworkConfig& config, DeterministicRng& rng);

  void reset();
  [[nodiscard]] NetworkSubmitResult enqueue_packet(const Packet& packet, std::uint64_t now_ms);
  [[nodiscard]] std::vector<Packet> poll_arrivals(std::uint64_t now_ms);
  [[nodiscard]] std::size_t queue_depth() const;
  [[nodiscard]] std::uint64_t drain_horizon_ms() const;

 private:
  struct PendingPacket {
    std::uint64_t arrival_time_ms {};
    std::uint64_t insertion_order {};
    Packet packet;
  };

  struct ComparePendingPacket {
    bool operator()(const PendingPacket& lhs, const PendingPacket& rhs) const {
      if (lhs.arrival_time_ms == rhs.arrival_time_ms) {
        return lhs.insertion_order > rhs.insertion_order;
      }
      return lhs.arrival_time_ms > rhs.arrival_time_ms;
    }
  };

  [[nodiscard]] std::uint64_t transmission_time_ms(std::size_t payload_size_bytes) const;

  NetworkConfig config_;
  DeterministicRng& rng_;
  std::priority_queue<PendingPacket, std::vector<PendingPacket>, ComparePendingPacket> pending_;
  std::uint64_t next_link_free_time_ms_ {0};
  int burst_drop_remaining_ {0};
  std::uint64_t insertion_counter_ {0};
  std::uint64_t max_arrival_time_ms_ {0};
};

}  // namespace mediaflow
