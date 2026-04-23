#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "mediaflow/config.hpp"
#include "mediaflow/http_server.hpp"
#include "mediaflow/scenario_factory.hpp"
#include "mediaflow/simulation_engine.hpp"

namespace {

struct CliOptions {
  std::string scenario {"stable"};
  std::filesystem::path config_path;
  std::filesystem::path export_path;
  std::uint64_t seed {42};
  bool seed_overridden {false};
  bool headless_requested {false};
  bool serve {false};
  bool list_scenarios {false};
  int port {8080};
};

void print_usage() {
  std::cout
      << "Usage:\n"
      << "  ./mediaflow --serve [--port 8080]\n"
      << "  ./mediaflow [--headless] [--scenario name | --config path] [--seed N] [--export result.json]\n"
      << "  ./mediaflow --list-scenarios\n\n"
      << "Note: non-serve runs are headless by default. --headless is accepted for explicit script parity.\n";
}

CliOptions parse_args(int argc, char** argv) {
  CliOptions options;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    const auto take_value = [&](const std::string& flag) -> std::string {
      if (index + 1 >= argc) {
        throw std::runtime_error("Missing value for " + flag);
      }
      ++index;
      return argv[index];
    };

    if (argument == "--scenario") {
      options.scenario = take_value(argument);
    } else if (argument == "--config") {
      options.config_path = take_value(argument);
    } else if (argument == "--seed") {
      options.seed = static_cast<std::uint64_t>(std::stoull(take_value(argument)));
      options.seed_overridden = true;
    } else if (argument == "--headless") {
      options.headless_requested = true;
    } else if (argument == "--serve") {
      options.serve = true;
    } else if (argument == "--port") {
      options.port = std::stoi(take_value(argument));
    } else if (argument == "--export") {
      options.export_path = take_value(argument);
    } else if (argument == "--list-scenarios") {
      options.list_scenarios = true;
    } else if (argument == "--help") {
      print_usage();
      std::exit(0);
    } else {
      throw std::runtime_error("Unknown argument: " + argument);
    }
  }
  return options;
}

void validate_cli_options(const CliOptions& options) {
  if (options.serve) {
    if (options.headless_requested) {
      throw std::runtime_error("--headless cannot be combined with --serve");
    }
    if (!options.config_path.empty() || options.scenario != "stable" || options.seed_overridden ||
        !options.export_path.empty()) {
      throw std::runtime_error(
          "--serve cannot be combined with scenario/config/seed/export flags; start runs through the API instead");
    }
  }

  if (options.list_scenarios) {
    if (options.serve || options.headless_requested || !options.config_path.empty() || options.seed_overridden ||
        !options.export_path.empty() || options.port != 8080) {
      throw std::runtime_error("--list-scenarios must be used on its own");
    }
  }

  if (!options.config_path.empty() && options.scenario != "stable") {
    throw std::runtime_error("--scenario and --config are mutually exclusive");
  }

  if (!options.serve && options.port != 8080) {
    throw std::runtime_error("--port is only valid with --serve");
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const CliOptions options = parse_args(argc, argv);
    validate_cli_options(options);

    if (options.list_scenarios) {
      for (const auto& scenario : mediaflow::predefined_scenarios()) {
        std::cout << scenario.name << " - " << scenario.description << '\n';
      }
      return 0;
    }

    if (options.serve) {
      mediaflow::HttpServer server;
      std::cout << "Serving MediaFlow Studio API on http://127.0.0.1:" << options.port << '\n';
      return server.serve("127.0.0.1", options.port);
    }

    mediaflow::ScenarioConfig config = options.config_path.empty()
                                           ? mediaflow::make_scenario(options.scenario)
                                           : mediaflow::load_config_from_file(options.config_path);
    const std::uint64_t seed = options.seed_overridden ? options.seed : config.seed;

    mediaflow::SimulationEngine engine;
    engine.load(config, seed);
    engine.run_to_completion();
    std::cout << engine.printable_summary();

    if (!options.export_path.empty()) {
      if (options.export_path.has_parent_path()) {
        std::filesystem::create_directories(options.export_path.parent_path());
      }
      std::ofstream output(options.export_path);
      if (!output.is_open()) {
        throw std::runtime_error("Unable to write export file: " + options.export_path.string());
      }
      output << engine.export_json().dump(2) << '\n';
      std::cout << "Exported results to " << options.export_path << '\n';
    }

    return 0;
  } catch (const std::exception& exception) {
    std::cerr << "Error: " << exception.what() << '\n';
    print_usage();
    return 1;
  }
}
