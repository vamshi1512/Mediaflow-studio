#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include "mediaflow/simulation_engine.hpp"

namespace httplib {
class Server;
}

namespace mediaflow {

class HttpServer {
 public:
  HttpServer();
  ~HttpServer();

  int serve(const std::string& host, int port);
  void stop();

 private:
  void worker_loop();

  SimulationEngine engine_;
  std::mutex mutex_;
  std::mutex server_mutex_;
  std::thread worker_;
  std::atomic<bool> stop_requested_ {false};
  std::unique_ptr<httplib::Server> server_;
};

}  // namespace mediaflow
