#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "mediaflow/types.hpp"

namespace mediaflow {

class Sender {
 public:
  Sender() = default;

  void reset();
  [[nodiscard]] std::vector<Packet> packetize(const Frame& frame, std::uint64_t send_time_ms);

 private:
  std::map<std::string, std::uint32_t> next_sequence_number_;
  std::uint64_t next_packet_id_ {0};
};

}  // namespace mediaflow
