#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mediaflow {

enum class MediaType {
  Audio,
  Video,
};

enum class Severity {
  Info,
  Warning,
  Error,
};

enum class SimulationStatus {
  Idle,
  Running,
  Paused,
  Completed,
  Error,
};

inline std::string to_string(MediaType media_type) {
  return media_type == MediaType::Audio ? "audio" : "video";
}

inline std::string to_string(Severity severity) {
  switch (severity) {
    case Severity::Info:
      return "info";
    case Severity::Warning:
      return "warning";
    case Severity::Error:
      return "error";
  }
  return "info";
}

inline std::string to_string(SimulationStatus status) {
  switch (status) {
    case SimulationStatus::Idle:
      return "idle";
    case SimulationStatus::Running:
      return "running";
    case SimulationStatus::Paused:
      return "paused";
    case SimulationStatus::Completed:
      return "completed";
    case SimulationStatus::Error:
      return "error";
  }
  return "idle";
}

struct Packet {
  std::string stream_id;
  std::uint64_t packet_id {};
  std::uint32_t sequence_number {};
  std::uint64_t timestamp_ms {};
  std::uint64_t frame_id {};
  std::size_t payload_size_bytes {};
  std::size_t packet_index {};
  std::size_t packets_in_frame {};
  MediaType media_type {MediaType::Audio};
  std::uint64_t generation_time_ms {};
  std::uint64_t send_time_ms {};
};

struct Frame {
  std::string stream_id;
  std::uint64_t frame_id {};
  std::uint64_t timestamp_ms {};
  std::uint64_t generation_time_ms {};
  std::size_t payload_size_bytes {};
  std::size_t packets_in_frame {};
  MediaType media_type {MediaType::Audio};
};

struct PlayedFrame {
  std::string stream_id;
  std::uint64_t frame_id {};
  std::uint64_t timestamp_ms {};
  std::uint64_t generation_time_ms {};
  std::size_t payload_size_bytes {};
  MediaType media_type {MediaType::Audio};
  std::uint64_t playout_time_ms {};
  int target_delay_ms {};
};

struct TimeSeriesPoint {
  std::uint64_t time_ms {};
  double value {};
};

struct SimulationEvent {
  std::uint64_t time_ms {};
  std::string kind;
  Severity severity {Severity::Info};
  std::string message;
  std::string stream_id;
  std::optional<std::uint64_t> packet_id;
  std::optional<std::uint64_t> frame_id;
};

}  // namespace mediaflow
