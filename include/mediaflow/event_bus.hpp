#pragma once

#include <functional>
#include <utility>
#include <vector>

namespace mediaflow {

template <typename EventT>
class EventBus {
 public:
  using Listener = std::function<void(const EventT&)>;

  void subscribe(Listener listener) { listeners_.push_back(std::move(listener)); }

  void publish(const EventT& event) const {
    for (const auto& listener : listeners_) {
      listener(event);
    }
  }

 private:
  std::vector<Listener> listeners_;
};

}  // namespace mediaflow
