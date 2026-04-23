#include <gtest/gtest.h>

#include "mediaflow/scenario_factory.hpp"
#include "mediaflow/simulation_engine.hpp"

namespace {

TEST(IntegrationScenarioTest, LowBufferFailureScenarioExposesQualityRegression) {
  auto config = mediaflow::make_scenario("low-buffer-failure");

  mediaflow::SimulationEngine engine;
  engine.load(config, 42);
  engine.run_to_completion();

  const auto summary = engine.summary();
  EXPECT_GT(summary.underflow_count, 0U);
  EXPECT_GT(summary.dropped_packets, 0U);
  EXPECT_LT(summary.playout_smoothness_pct, 95.0);
}

}  // namespace
