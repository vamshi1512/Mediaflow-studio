#include <gtest/gtest.h>

#include "mediaflow/scenario_factory.hpp"
#include "mediaflow/simulation_engine.hpp"

namespace {

TEST(DeterministicReplayTest, SameSeedProducesIdenticalResults) {
  auto config = mediaflow::make_scenario("burst-loss");

  mediaflow::SimulationEngine first_run;
  first_run.load(config, 77);
  first_run.run_to_completion();

  mediaflow::SimulationEngine second_run;
  second_run.load(config, 77);
  second_run.run_to_completion();

  EXPECT_EQ(first_run.export_json().dump(), second_run.export_json().dump());
}

TEST(DeterministicReplayTest, DifferentSeedsProduceDifferentResults) {
  auto config = mediaflow::make_scenario("burst-loss");

  mediaflow::SimulationEngine first_run;
  first_run.load(config, 77);
  first_run.run_to_completion();

  mediaflow::SimulationEngine second_run;
  second_run.load(config, 78);
  second_run.run_to_completion();

  EXPECT_NE(first_run.export_json().dump(), second_run.export_json().dump());
}

}  // namespace
