#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "mediaflow/types.hpp"

namespace mediaflow {

struct StreamConfig {
  bool enabled {true};
  std::string stream_id;
  MediaType media_type {MediaType::Audio};
  int frame_interval_ms {20};
  std::size_t frame_payload_bytes {240};
  std::size_t packets_per_frame {1};
};

struct NetworkConfig {
  int base_latency_ms {40};
  int jitter_ms {6};
  double packet_loss_rate {0.0};
  double burst_loss_trigger_rate {0.0};
  int burst_loss_length {0};
  double reorder_rate {0.0};
  int reorder_hold_ms {0};
  int bandwidth_kbps {1'000};
  int queue_capacity_packets {256};
  int queue_pressure_drop_threshold_ms {350};
};

struct JitterBufferConfig {
  int target_delay_ms {80};
  int min_delay_ms {40};
  int max_delay_ms {240};
  int max_packets {512};
  bool adaptive {false};
  int start_buffer_frames {2};
};

struct ScenarioConfig {
  std::string name {"stable"};
  std::string description;
  int duration_ms {8'000};
  std::uint64_t seed {42};
  int sample_interval_ms {100};
  StreamConfig audio_stream {
      true,
      "audio-main",
      MediaType::Audio,
      20,
      240,
      1,
  };
  StreamConfig video_stream {
      false,
      "video-main",
      MediaType::Video,
      33,
      8'400,
      7,
  };
  NetworkConfig network {};
  JitterBufferConfig jitter_buffer {};
};

void to_json(nlohmann::json& json, const StreamConfig& config);
void from_json(const nlohmann::json& json, StreamConfig& config);

void to_json(nlohmann::json& json, const NetworkConfig& config);
void from_json(const nlohmann::json& json, NetworkConfig& config);

void to_json(nlohmann::json& json, const JitterBufferConfig& config);
void from_json(const nlohmann::json& json, JitterBufferConfig& config);

void to_json(nlohmann::json& json, const ScenarioConfig& config);
void from_json(const nlohmann::json& json, ScenarioConfig& config);

ScenarioConfig load_config_from_file(const std::filesystem::path& path);
void save_config_to_file(const ScenarioConfig& config, const std::filesystem::path& path);
std::vector<std::string> validate_config_errors(const ScenarioConfig& config);
void validate_config(const ScenarioConfig& config);

}  // namespace mediaflow
