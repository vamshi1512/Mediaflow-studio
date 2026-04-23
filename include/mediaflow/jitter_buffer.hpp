#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "mediaflow/config.hpp"
#include "mediaflow/event_bus.hpp"
#include "mediaflow/types.hpp"

namespace mediaflow {

enum class IngestDisposition {
  Buffered,
  LateDrop,
  OverflowDrop,
};

struct IngestResult {
  IngestDisposition disposition {IngestDisposition::Buffered};
};

struct PlayoutResult {
  bool played {false};
  std::string stream_id;
  std::uint64_t frame_id {};
  std::size_t packets_played {};
  std::optional<PlayedFrame> frame;
};

struct BufferDelayContext {
  int current_target_delay_ms {};
  double jitter_estimate_ms {};
  bool late_packet_observed {false};
  bool underflow_observed {false};
  int frame_interval_ms {};
};

class BufferDelayStrategy {
 public:
  virtual ~BufferDelayStrategy() = default;
  [[nodiscard]] virtual int compute_target_delay(
      const BufferDelayContext& context,
      const JitterBufferConfig& config) const = 0;
  [[nodiscard]] virtual std::string name() const = 0;
};

class FixedDelayStrategy final : public BufferDelayStrategy {
 public:
  [[nodiscard]] int compute_target_delay(
      const BufferDelayContext& context,
      const JitterBufferConfig& config) const override;
  [[nodiscard]] std::string name() const override;
};

class AdaptiveDelayStrategy final : public BufferDelayStrategy {
 public:
  [[nodiscard]] int compute_target_delay(
      const BufferDelayContext& context,
      const JitterBufferConfig& config) const override;
  [[nodiscard]] std::string name() const override;
};

class JitterBuffer {
 public:
  JitterBuffer(
      const ScenarioConfig& config,
      EventBus<SimulationEvent>& bus);

  void reset();
  [[nodiscard]] IngestResult ingest_packet(
      const Packet& packet,
      std::uint64_t arrival_time_ms,
      double jitter_estimate_ms);
  [[nodiscard]] std::vector<PlayoutResult> advance(std::uint64_t now_ms);
  [[nodiscard]] std::vector<PlayoutResult> flush_remaining(std::uint64_t now_ms);
  [[nodiscard]] std::size_t occupancy_packets() const;
  [[nodiscard]] int current_target_delay_ms(const std::string& stream_id) const;
  [[nodiscard]] std::string strategy_name() const;

 private:
  struct BufferedFrame {
    std::uint64_t frame_id {};
    std::uint64_t timestamp_ms {};
    std::uint64_t generation_time_ms {};
    MediaType media_type {MediaType::Audio};
    std::size_t expected_packets {};
    std::size_t received_packets {};
    std::size_t payload_bytes {};
    std::vector<bool> packet_seen;
  };

  struct StreamState {
    StreamConfig config;
    std::map<std::uint64_t, BufferedFrame> frames;
    std::optional<std::uint64_t> first_timestamp_ms;
    std::optional<std::uint64_t> playout_start_time_ms;
    std::uint64_t next_playout_timestamp_ms {};
    std::uint64_t last_expected_timestamp_ms {};
    int current_target_delay_ms {};
    std::size_t occupancy_packets {};
  };

  [[nodiscard]] StreamState& state_for(const std::string& stream_id);
  [[nodiscard]] const StreamState& state_for(const std::string& stream_id) const;
  [[nodiscard]] std::uint64_t playout_deadline_ms(const StreamState& state, std::uint64_t timestamp_ms) const;
  void maybe_adjust_target_delay(
      StreamState& state,
      double jitter_estimate_ms,
      bool late_packet_observed,
      bool underflow_observed,
      std::uint64_t now_ms);

  ScenarioConfig config_;
  EventBus<SimulationEvent>& bus_;
  std::map<std::string, StreamState> streams_;
  std::unique_ptr<BufferDelayStrategy> delay_strategy_;
};

}  // namespace mediaflow
