#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "mediaflow/config.hpp"

namespace mediaflow {

ScenarioConfig make_scenario(const std::string& name);
std::vector<ScenarioConfig> predefined_scenarios();
nlohmann::json predefined_scenarios_json();

}  // namespace mediaflow
