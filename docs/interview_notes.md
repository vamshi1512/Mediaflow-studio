# Interview Notes

## 60-second explanation

MediaFlow Studio is a deterministic real-time media transport simulator built in C++20 with a React monitoring dashboard. It models a sender, an RTP-like packet layer, an unreliable network, a receiver, and a jitter buffer, then exposes live latency, jitter, loss, reorder, throughput, and playout events. I built it to show systems thinking around timing-sensitive media delivery, not just UI polish.

## 2-minute explanation

The backend uses a fixed-step simulation engine so runs are deterministic for a given seed. Frames are generated at configured audio/video intervals, packetized with sequence numbers and timestamps, pushed through a network simulator that can inject jitter, loss, burst loss, reorder, and bandwidth pressure, and then consumed by a receiver with a jitter buffer. The jitter buffer supports fixed and adaptive delay strategies, which lets me demonstrate the classic tradeoff between low latency and playout smoothness. On top of that, I added a React dashboard that can start seeded runs, adjust parameters, and visualize the metrics in real time. I also wrote GoogleTest coverage around packetization, buffer timing, late packets, missing packet behavior, config loading, deterministic replay, and a full scenario integration run.

## Architecture walkthrough

1. `SimulationEngine` owns the deterministic clock and coordinates the pipeline.
2. `FrameGenerator` emits synthetic audio/video frames with exact timing.
3. `Sender` packetizes frames into RTP-like packets.
4. `NetworkSimulator` schedules delivery under latency, jitter, loss, reorder, and bandwidth limits.
5. `Receiver` tracks sequence behavior and passes packets to the jitter buffer.
6. `JitterBuffer` decides when playout should happen and whether a frame is playable, late, or missed.
7. `MetricsCollector` samples summary counters, time-series data, and event logs.
8. `HttpServer` exposes state and control APIs for the UI.

## Tradeoffs to mention

- I chose polling over WebSockets for simplicity and debuggability.
- I modeled frame playout instead of a full codec pipeline because timing and buffering were the primary learning goals.
- The adaptive delay strategy is intentionally transparent rather than “smart but opaque.”
- The dashboard is visually polished, but I kept core transport logic strictly in C++.

## What I would improve next

- Add side-by-side adaptive versus fixed comparison runs in one view.
- Add retransmission/FEC simulation to study recovery strategies.
- Add richer queueing disciplines and pacing behavior.
- Record scenario baselines and diff regressions automatically in CI.
- Add a small WebSocket mode for lower-latency dashboard updates.

## How this relates to Cisco Webex Media

The project focuses on the parts of media delivery that matter to collaboration products:

- real-time timing sensitivity
- packet sequencing and timestamping
- jitter buffer behavior
- loss and reordering under imperfect networks
- deterministic testability for transport behavior
- instrumentation that helps explain QoE regressions

It is not a production media stack, but it demonstrates the core engineering instincts that transfer directly to media transport and playout work.
