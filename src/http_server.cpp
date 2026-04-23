#include "mediaflow/http_server.hpp"

#include <chrono>
#include <exception>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "mediaflow/config.hpp"
#include "mediaflow/scenario_factory.hpp"

namespace mediaflow {

namespace {

constexpr auto kWorkerSleep = std::chrono::milliseconds(40);
constexpr std::uint64_t kWorkerSimStepMs = 40;

void set_json_response(
    httplib::Response& response,
    const nlohmann::json& body,
    int status = 200) {
  response.status = status;
  response.set_content(body.dump(2), "application/json");
}

void set_error_response(
    httplib::Response& response,
    int status,
    const std::string& message) {
  set_json_response(response, {{"error", message}}, status);
}

}  // namespace

HttpServer::HttpServer() = default;

HttpServer::~HttpServer() { stop(); }

int HttpServer::serve(const std::string& host, int port) {
  {
    std::scoped_lock lock(server_mutex_);
    server_ = std::make_unique<httplib::Server>();
  }

  httplib::Server& server = *server_;
  server.set_default_headers({
      {"Access-Control-Allow-Origin", "*"},
      {"Access-Control-Allow-Headers", "Content-Type"},
      {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
  });
  server.set_exception_handler([](
                                   const httplib::Request&,
                                   httplib::Response& response,
                                   std::exception_ptr exception_ptr) {
    try {
      if (exception_ptr) {
        std::rethrow_exception(exception_ptr);
      }
    } catch (const nlohmann::json::exception& exception) {
      set_error_response(response, 400, exception.what());
      return;
    } catch (const std::exception& exception) {
      set_error_response(response, 500, exception.what());
      return;
    } catch (...) {
      set_error_response(response, 500, "Unknown server error");
    }
  });

  server.Options(R"(.*)", [](const httplib::Request&, httplib::Response& response) {
    response.status = 204;
  });

  server.Get("/api/health", [this](const httplib::Request&, httplib::Response& response) {
    std::scoped_lock lock(mutex_);
    set_json_response(
        response,
        {
            {"ok", true},
            {"status", to_string(engine_.status())},
            {"scenarioName", engine_.config().name},
            {"simulationTimeMs", engine_.current_time_ms()},
        });
  });

  server.Get("/api/scenarios", [](const httplib::Request&, httplib::Response& response) {
    set_json_response(response, predefined_scenarios_json());
  });

  server.Get("/api/state", [this](const httplib::Request&, httplib::Response& response) {
    std::scoped_lock lock(mutex_);
    set_json_response(response, engine_.snapshot_json());
  });

  server.Post("/api/control/start", [this](const httplib::Request& request, httplib::Response& response) {
    try {
      const auto body =
          request.body.empty() ? nlohmann::json::object() : nlohmann::json::parse(request.body);

      ScenarioConfig config;
      if (body.contains("config")) {
        config = body.at("config").get<ScenarioConfig>();
      } else {
        config = make_scenario(body.value("scenario", std::string("stable")));
      }

      validate_config(config);
      const std::uint64_t seed = body.value("seed", config.seed);

      std::scoped_lock lock(mutex_);
      engine_.load(config, seed);
      engine_.start();
      set_json_response(response, engine_.snapshot_json());
    } catch (const std::exception& exception) {
      set_error_response(response, 400, exception.what());
    }
  });

  server.Post("/api/control/pause", [this](const httplib::Request&, httplib::Response& response) {
    std::scoped_lock lock(mutex_);
    engine_.pause();
    set_json_response(response, engine_.snapshot_json());
  });

  server.Post("/api/control/resume", [this](const httplib::Request&, httplib::Response& response) {
    std::scoped_lock lock(mutex_);
    engine_.resume();
    set_json_response(response, engine_.snapshot_json());
  });

  server.Post("/api/control/reset", [this](const httplib::Request&, httplib::Response& response) {
    std::scoped_lock lock(mutex_);
    engine_.reset();
    set_json_response(response, engine_.snapshot_json());
  });

  server.Get("/api/export", [this](const httplib::Request&, httplib::Response& response) {
    std::scoped_lock lock(mutex_);
    response.set_header("Content-Disposition", "attachment; filename=\"mediaflow-export.json\"");
    set_json_response(response, engine_.export_json());
  });

  stop_requested_ = false;
  worker_ = std::thread([this]() { worker_loop(); });

  const bool listened = server.listen(host, port);
  stop();
  return listened ? 0 : 1;
}

void HttpServer::stop() {
  stop_requested_ = true;
  {
    std::scoped_lock lock(server_mutex_);
    if (server_ != nullptr) {
      server_->stop();
    }
  }
  if (worker_.joinable()) {
    worker_.join();
  }
  std::scoped_lock lock(server_mutex_);
  server_.reset();
}

void HttpServer::worker_loop() {
  while (!stop_requested_.load()) {
    {
      std::scoped_lock lock(mutex_);
      engine_.step(kWorkerSimStepMs);
    }
    std::this_thread::sleep_for(kWorkerSleep);
  }
}

}  // namespace mediaflow
