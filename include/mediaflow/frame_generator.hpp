#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "mediaflow/config.hpp"
#include "mediaflow/types.hpp"

namespace mediaflow {

class FrameGenerator {
 public:
  explicit FrameGenerator(const ScenarioConfig& config);

  void reset();
  [[nodiscard]] std::vector<Frame> generate_due_frames(
      std::uint64_t now_ms,
      std::uint64_t duration_ms);

 private:
  ScenarioConfig config_;
  std::map<std::string, std::uint64_t> next_generation_ms_;
  std::map<std::string, std::uint64_t> next_frame_id_;
};

}  // namespace mediaflow
