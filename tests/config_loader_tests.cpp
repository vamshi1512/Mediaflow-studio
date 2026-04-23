#include <filesystem>

#include <gtest/gtest.h>

#include "mediaflow/config.hpp"
#include "mediaflow/scenario_factory.hpp"

namespace {

TEST(ConfigLoaderTest, LoadsAudioOnlyCustomConfig) {
  const auto config_path =
      std::filesystem::path(MEDIAFLOW_TEST_CONFIG_DIR) / "custom-audio-only.json";

  const auto config = mediaflow::load_config_from_file(config_path);
  EXPECT_EQ(config.name, "custom-audio-only");
  EXPECT_TRUE(config.audio_stream.enabled);
  EXPECT_FALSE(config.video_stream.enabled);
  EXPECT_TRUE(config.jitter_buffer.adaptive);
  EXPECT_EQ(config.network.bandwidth_kbps, 256);
}

TEST(ConfigLoaderTest, SaveAndReloadRoundTripsScenarioConfig) {
  const auto temp_path =
      std::filesystem::temp_directory_path() / "mediaflow-roundtrip-config.json";

  auto config = mediaflow::make_scenario("moderate-jitter");
  config.seed = 9001;
  config.network.packet_loss_rate = 0.03;

  mediaflow::save_config_to_file(config, temp_path);
  const auto reloaded = mediaflow::load_config_from_file(temp_path);

  EXPECT_EQ(nlohmann::json(config).dump(), nlohmann::json(reloaded).dump());
  std::filesystem::remove(temp_path);
}

TEST(ConfigLoaderTest, RejectsInvalidConfig) {
  auto config = mediaflow::make_scenario("stable");
  config.audio_stream.enabled = false;
  config.video_stream.enabled = false;
  config.sample_interval_ms = 0;

  const auto errors = mediaflow::validate_config_errors(config);
  EXPECT_FALSE(errors.empty());
  EXPECT_THROW(mediaflow::validate_config(config), std::runtime_error);
}

}  // namespace
