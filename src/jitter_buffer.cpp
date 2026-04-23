#include "mediaflow/jitter_buffer.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace mediaflow {

int FixedDelayStrategy::compute_target_delay(
    const BufferDelayContext&,
    const JitterBufferConfig& config) const {
  return config.target_delay_ms;
}

std::string FixedDelayStrategy::name() const { return "fixed"; }

int AdaptiveDelayStrategy::compute_target_delay(
    const BufferDelayContext& context,
    const JitterBufferConfig& config) const {
  int desired_delay = config.target_delay_ms + static_cast<int>(std::round(context.jitter_estimate_ms * 1.8));
  if (context.late_packet_observed || context.underflow_observed) {
    desired_delay += context.frame_interval_ms;
  }
  desired_delay = std::clamp(desired_delay, config.min_delay_ms, config.max_delay_ms);

  if (desired_delay > context.current_target_delay_ms) {
    return desired_delay;
  }
  return std::max(desired_delay, context.current_target_delay_ms - 1);
}

std::string AdaptiveDelayStrategy::name() const { return "adaptive"; }

JitterBuffer::JitterBuffer(const ScenarioConfig& config, EventBus<SimulationEvent>& bus)
    : config_(config), bus_(bus) {
  reset();
}

void JitterBuffer::reset() {
  streams_.clear();

  const auto make_state = [this](const StreamConfig& stream) {
    StreamState state;
    state.config = stream;
    state.current_target_delay_ms = config_.jitter_buffer.target_delay_ms;
    const auto duration_ms = static_cast<std::uint64_t>(std::max(0, config_.duration_ms));
    if (duration_ms > 0) {
      const auto interval_ms = static_cast<std::uint64_t>(std::max(1, stream.frame_interval_ms));
      state.last_expected_timestamp_ms = ((duration_ms - 1) / interval_ms) * interval_ms;
    }
    return state;
  };

  if (config_.audio_stream.enabled) {
    streams_.emplace(config_.audio_stream.stream_id, make_state(config_.audio_stream));
  }
  if (config_.video_stream.enabled) {
    streams_.emplace(config_.video_stream.stream_id, make_state(config_.video_stream));
  }

  if (config_.jitter_buffer.adaptive) {
    delay_strategy_ = std::make_unique<AdaptiveDelayStrategy>();
  } else {
    delay_strategy_ = std::make_unique<FixedDelayStrategy>();
  }
}

IngestResult JitterBuffer::ingest_packet(
    const Packet& packet,
    std::uint64_t arrival_time_ms,
    double jitter_estimate_ms) {
  auto& stream = state_for(packet.stream_id);

  if (!stream.first_timestamp_ms.has_value()) {
    stream.first_timestamp_ms = packet.timestamp_ms;
    stream.next_playout_timestamp_ms = packet.timestamp_ms;
    const std::uint64_t warmup_frames =
        static_cast<std::uint64_t>(std::max(0, config_.jitter_buffer.start_buffer_frames - 1));
    stream.playout_start_time_ms =
        arrival_time_ms +
        static_cast<std::uint64_t>(stream.current_target_delay_ms) +
        warmup_frames * static_cast<std::uint64_t>(stream.config.frame_interval_ms);
  }

  if (stream.playout_start_time_ms.has_value() &&
      arrival_time_ms > playout_deadline_ms(stream, packet.timestamp_ms)) {
    maybe_adjust_target_delay(stream, jitter_estimate_ms, true, false, arrival_time_ms);
    return IngestResult {IngestDisposition::LateDrop};
  }

  const std::size_t total_occupancy = occupancy_packets();
  if (total_occupancy >= static_cast<std::size_t>(config_.jitter_buffer.max_packets)) {
    return IngestResult {IngestDisposition::OverflowDrop};
  }

  auto [frame_it, inserted] =
      stream.frames.try_emplace(packet.timestamp_ms, BufferedFrame {});
  auto& frame = frame_it->second;
  if (inserted) {
    frame.frame_id = packet.frame_id;
    frame.timestamp_ms = packet.timestamp_ms;
    frame.generation_time_ms = packet.generation_time_ms;
    frame.media_type = packet.media_type;
    frame.expected_packets = packet.packets_in_frame;
    frame.packet_seen.assign(packet.packets_in_frame, false);
  }

  if (!frame.packet_seen.at(packet.packet_index)) {
    frame.packet_seen.at(packet.packet_index) = true;
    ++frame.received_packets;
    frame.payload_bytes += packet.payload_size_bytes;
    ++stream.occupancy_packets;
  }

  maybe_adjust_target_delay(stream, jitter_estimate_ms, false, false, arrival_time_ms);
  return IngestResult {IngestDisposition::Buffered};
}

std::vector<PlayoutResult> JitterBuffer::advance(std::uint64_t now_ms) {
  std::vector<PlayoutResult> results;
  for (auto& [stream_id, state] : streams_) {
    (void)stream_id;
    if (!state.playout_start_time_ms.has_value()) {
      continue;
    }

    while (state.next_playout_timestamp_ms <= state.last_expected_timestamp_ms &&
           now_ms >= playout_deadline_ms(state, state.next_playout_timestamp_ms)) {
      const auto deadline_ms = playout_deadline_ms(state, state.next_playout_timestamp_ms);
      auto frame_it = state.frames.find(state.next_playout_timestamp_ms);
      if (frame_it != state.frames.end() &&
          frame_it->second.received_packets == frame_it->second.expected_packets) {
        const auto packets_played = frame_it->second.received_packets;
        state.occupancy_packets -= packets_played;
        results.push_back(PlayoutResult {
            true,
            state.config.stream_id,
            frame_it->second.frame_id,
            packets_played,
            PlayedFrame {
                state.config.stream_id,
                frame_it->second.frame_id,
                frame_it->second.timestamp_ms,
                frame_it->second.generation_time_ms,
                frame_it->second.payload_bytes,
                frame_it->second.media_type,
                deadline_ms,
                state.current_target_delay_ms,
            },
        });
        state.frames.erase(frame_it);
      } else {
        std::uint64_t frame_id = state.next_playout_timestamp_ms;
        if (frame_it != state.frames.end()) {
          frame_id = frame_it->second.frame_id;
          state.occupancy_packets -= frame_it->second.received_packets;
          state.frames.erase(frame_it);
        }
        maybe_adjust_target_delay(state, 0.0, false, true, now_ms);
        results.push_back(PlayoutResult {false, state.config.stream_id, frame_id, 0, std::nullopt});
      }

      state.next_playout_timestamp_ms += static_cast<std::uint64_t>(state.config.frame_interval_ms);
    }
  }
  return results;
}

std::vector<PlayoutResult> JitterBuffer::flush_remaining(std::uint64_t now_ms) {
  std::vector<PlayoutResult> results = advance(now_ms);
  for (auto& [stream_id, state] : streams_) {
    (void)stream_id;
    while (state.next_playout_timestamp_ms <= state.last_expected_timestamp_ms) {
      auto frame_it = state.frames.find(state.next_playout_timestamp_ms);
      std::uint64_t frame_id = state.next_playout_timestamp_ms;
      if (frame_it != state.frames.end()) {
        frame_id = frame_it->second.frame_id;
        state.occupancy_packets -= frame_it->second.received_packets;
        state.frames.erase(frame_it);
      }
      results.push_back(PlayoutResult {false, state.config.stream_id, frame_id, 0, std::nullopt});
      state.next_playout_timestamp_ms += static_cast<std::uint64_t>(state.config.frame_interval_ms);
    }

    for (const auto& [timestamp, frame] : state.frames) {
      (void)timestamp;
      results.push_back(PlayoutResult {false, state.config.stream_id, frame.frame_id, 0, std::nullopt});
    }
    state.frames.clear();
    state.occupancy_packets = 0;
  }
  return results;
}

std::size_t JitterBuffer::occupancy_packets() const {
  std::size_t packets = 0;
  for (const auto& [stream_id, state] : streams_) {
    (void)stream_id;
    packets += state.occupancy_packets;
  }
  return packets;
}

int JitterBuffer::current_target_delay_ms(const std::string& stream_id) const {
  return state_for(stream_id).current_target_delay_ms;
}

std::string JitterBuffer::strategy_name() const { return delay_strategy_->name(); }

JitterBuffer::StreamState& JitterBuffer::state_for(const std::string& stream_id) {
  auto it = streams_.find(stream_id);
  if (it == streams_.end()) {
    throw std::runtime_error("Unknown stream id for jitter buffer: " + stream_id);
  }
  return it->second;
}

const JitterBuffer::StreamState& JitterBuffer::state_for(const std::string& stream_id) const {
  auto it = streams_.find(stream_id);
  if (it == streams_.end()) {
    throw std::runtime_error("Unknown stream id for jitter buffer: " + stream_id);
  }
  return it->second;
}

std::uint64_t JitterBuffer::playout_deadline_ms(
    const StreamState& state, std::uint64_t timestamp_ms) const {
  if (!state.playout_start_time_ms.has_value() || !state.first_timestamp_ms.has_value()) {
    return timestamp_ms;
  }
  return state.playout_start_time_ms.value() + (timestamp_ms - state.first_timestamp_ms.value());
}

void JitterBuffer::maybe_adjust_target_delay(
    StreamState& state,
    double jitter_estimate_ms,
    bool late_packet_observed,
    bool underflow_observed,
    std::uint64_t now_ms) {
  const BufferDelayContext context {
      state.current_target_delay_ms,
      jitter_estimate_ms,
      late_packet_observed,
      underflow_observed,
      state.config.frame_interval_ms,
  };
  const int next_target = delay_strategy_->compute_target_delay(context, config_.jitter_buffer);
  if (next_target == state.current_target_delay_ms) {
    return;
  }

  const int delta = next_target - state.current_target_delay_ms;
  state.current_target_delay_ms = next_target;
  if (state.playout_start_time_ms.has_value()) {
    const auto updated_start_time =
        static_cast<std::int64_t>(state.playout_start_time_ms.value()) + static_cast<std::int64_t>(delta);
    state.playout_start_time_ms = static_cast<std::uint64_t>(std::max<std::int64_t>(0, updated_start_time));
  }

  bus_.publish(SimulationEvent {
      now_ms,
      "buffer-target-update",
      Severity::Info,
      "Adjusted jitter buffer target to " + std::to_string(next_target) + " ms",
      state.config.stream_id,
      std::nullopt,
      std::nullopt,
  });
}

}  // namespace mediaflow
