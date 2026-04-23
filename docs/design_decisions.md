# Design Decisions

## Why polling instead of WebSockets

Polling is sufficient for a single-machine demo dashboard and keeps the backend dependency footprint small. A WebSocket transport would be reasonable next work, but HTTP polling is easier to debug with `curl`, easier to test, and enough for the update rate used here.

## Why the jitter buffer is frame-oriented

The simulator models packet transport but schedules playout at frame granularity. This keeps the implementation explainable while still exposing the key tradeoff interviewers care about: latency versus smoothness under network variation.

## Why the transport model is “RTP-like”

The packet shape mirrors media transport concepts such as sequence number, timestamp, frame id, and stream id, but the project does not claim RTP or WebRTC compliance. The intent is educational and architectural, not standards conformance.

## Why the adaptive buffer strategy is intentionally simple

The adaptive delay controller only reacts to measured jitter plus late/underflow signals. This is deliberate:

- it is easy to reason about in an interview
- it is deterministic under a fixed seed
- it demonstrates strategy-based design without hiding behavior in a complex heuristic

## Why metrics are sampled instead of emitted every tick

The simulation advances every millisecond, but sampling every 100 ms keeps exported data compact and the dashboard responsive. The underlying simulation stays fine-grained while the UI only receives the signal needed for monitoring.

## Simplifications made on purpose

- No codec model or payload content.
- No true RTP/RTCP signaling.
- No FEC, NACK, retransmission, pacing, or congestion control protocol.
- No browser-side real-time media pipeline.
- No persistence layer or cloud deployment.

These omissions keep the project focused on deterministic transport timing, buffering, and explainability, which are the strongest signals for the target role.
