# Architecture

MediaFlow Studio is structured as a deterministic transport simulator with a thin monitoring UI. The goal is to keep transport behavior in C++ and use the frontend only for orchestration and explanation.

## High-level flow

1. `FrameGenerator` emits synthetic audio/video frames at fixed intervals.
2. `Sender` packetizes each frame into an RTP-like packet sequence.
3. `NetworkSimulator` applies latency, jitter, random loss, burst loss, reorder holds, bandwidth shaping, and queue pressure.
4. `Receiver` ingests delivered packets, tracks sequence behavior, and forwards them into the jitter buffer.
5. `JitterBuffer` performs playout scheduling with fixed or adaptive target delay.
6. `MetricsCollector` samples latency, jitter, loss, occupancy, throughput, and event logs.
7. `HttpServer` exposes control and telemetry endpoints consumed by the React dashboard.

## Backend modules

- `include/mediaflow/types.hpp`
  Shared packet, frame, metrics, and event types.
- `include/mediaflow/config.hpp` + `src/config.cpp`
  JSON parsing and persistence for scenario configs.
- `include/mediaflow/frame_generator.hpp`
  Deterministic audio/video frame scheduling.
- `include/mediaflow/sender.hpp`
  RTP-like packetization and sequence number management.
- `include/mediaflow/network_simulator.hpp`
  Unreliable transport behavior with queue-aware delivery scheduling.
- `include/mediaflow/jitter_buffer.hpp`
  Frame-oriented playout buffer with pluggable delay strategy.
- `include/mediaflow/receiver.hpp`
  Packet reception, reorder detection, and buffer interaction.
- `include/mediaflow/metrics.hpp`
  Summary and time-series aggregation, plus event collection.
- `include/mediaflow/scenario_factory.hpp`
  Factory for predefined demo scenarios.
- `include/mediaflow/simulation_engine.hpp`
  Fixed-step orchestrator that advances the entire pipeline.
- `include/mediaflow/http_server.hpp`
  Poll-friendly JSON API used by the dashboard.

## Design patterns used

- Strategy
  `BufferDelayStrategy` switches between fixed and adaptive playout delay control.
- Observer
  `EventBus<SimulationEvent>` decouples event producers from metrics/event log consumers.
- Factory
  `ScenarioFactory` centralizes interview-ready scenario presets and prevents scattered config creation.

## Timing model

- Simulation time advances in 1 ms increments inside `SimulationEngine`.
- Frame timestamps are generated from exact frame intervals rather than wall-clock timers.
- Network delivery times are derived from:
  - sender time
  - serialization delay from `bandwidthKbps`
  - base latency
  - jitter offset
  - optional reorder hold
- The engine continues past scenario duration to drain in-flight packets and pending playout.

## API surface

The backend uses HTTP polling instead of WebSockets to keep dependencies small and behavior easy to inspect.

- `GET /api/health`
- `GET /api/scenarios`
- `GET /api/state`
- `POST /api/control/start`
- `POST /api/control/pause`
- `POST /api/control/resume`
- `POST /api/control/reset`
- `GET /api/export`

## Frontend architecture

- `src/App.tsx`
  Layout, polling, scenario state, and control orchestration.
- `src/components/ControlSidebar.tsx`
  Scenario selection, seeded replay, and config sliders.
- `src/components/ChartPanel.tsx`
  Reusable chart shell for Recharts line plots.
- `src/components/EventLog.tsx`
  Deferred rendering for transport events.
- `src/components/ScenarioPanel.tsx`
  Demo narrative panel for interview walkthroughs.

## Determinism

Determinism is enforced by:

- seeded `std::mt19937_64`
- fixed-step simulation time
- no dependence on wall-clock timing in headless mode
- scenario config as the single source of run inputs

This makes it practical to replay a specific failure profile during interviews or in tests.
