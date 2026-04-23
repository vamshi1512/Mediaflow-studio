#include "mediaflow/frame_generator.hpp"

namespace mediaflow {

FrameGenerator::FrameGenerator(const ScenarioConfig& config) : config_(config) { reset(); }

void FrameGenerator::reset() {
  next_generation_ms_.clear();
  next_frame_id_.clear();

  if (config_.audio_stream.enabled) {
    next_generation_ms_[config_.audio_stream.stream_id] = 0;
    next_frame_id_[config_.audio_stream.stream_id] = 0;
  }

  if (config_.video_stream.enabled) {
    next_generation_ms_[config_.video_stream.stream_id] = 0;
    next_frame_id_[config_.video_stream.stream_id] = 0;
  }
}

std::vector<Frame> FrameGenerator::generate_due_frames(
    std::uint64_t now_ms, std::uint64_t duration_ms) {
  std::vector<Frame> frames;

  const auto emit = [&](const StreamConfig& stream, std::vector<Frame>& output) {
    auto& next_time_ms = next_generation_ms_[stream.stream_id];
    auto& frame_id = next_frame_id_[stream.stream_id];
    while (stream.enabled && next_time_ms <= now_ms && next_time_ms < duration_ms) {
      output.push_back(Frame {
          stream.stream_id,
          frame_id++,
          next_time_ms,
          next_time_ms,
          stream.frame_payload_bytes,
          stream.packets_per_frame,
          stream.media_type,
      });
      next_time_ms += static_cast<std::uint64_t>(stream.frame_interval_ms);
    }
  };

  if (config_.audio_stream.enabled) {
    emit(config_.audio_stream, frames);
  }
  if (config_.video_stream.enabled) {
    emit(config_.video_stream, frames);
  }

  return frames;
}

}  // namespace mediaflow
