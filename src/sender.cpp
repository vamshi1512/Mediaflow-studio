#include "mediaflow/sender.hpp"

#include <algorithm>

namespace mediaflow {

void Sender::reset() {
  next_sequence_number_.clear();
  next_packet_id_ = 0;
}

std::vector<Packet> Sender::packetize(const Frame& frame, std::uint64_t send_time_ms) {
  const std::size_t packets = std::max<std::size_t>(1, frame.packets_in_frame);
  const std::size_t base_payload = frame.payload_size_bytes / packets;
  const std::size_t remainder = frame.payload_size_bytes % packets;

  std::vector<Packet> output;
  output.reserve(packets);

  auto& sequence_number = next_sequence_number_[frame.stream_id];
  for (std::size_t index = 0; index < packets; ++index) {
    output.push_back(Packet {
        frame.stream_id,
        next_packet_id_++,
        sequence_number++,
        frame.timestamp_ms,
        frame.frame_id,
        base_payload + (index < remainder ? 1U : 0U),
        index,
        packets,
        frame.media_type,
        frame.generation_time_ms,
        send_time_ms,
    });
  }

  return output;
}

}  // namespace mediaflow
