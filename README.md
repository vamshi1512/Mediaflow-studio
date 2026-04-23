# MediaFlow Studio

MediaFlow Studio is a portfolio-grade real-time media transport simulator built for a recent graduate targeting a Cisco Webex Media C/C++ Software Engineer role. It combines a modern C++20 backend with a polished React dashboard to demonstrate transport timing, unreliable network behavior, jitter buffering, deterministic replay, and engineering quality practices.

## What it demonstrates

- Modern C++20 design with clean module boundaries
- Real-time and performance-sensitive simulation design
- RTP-like packet modeling without claiming RTP/WebRTC compliance
- Deterministic seeded scenarios for repeatable debugging and demos
- Strategy, Observer, and Factory patterns in practical use
- GoogleTest + CTest quality gates
- CMake-based build with separable library, executable, and test targets
- React + TypeScript + Tailwind + Recharts dashboard with interview-ready UX
- Responsible use of AI assistance, with human validation of architecture, code, and tests

## Feature set

### Backend

- Sender, RTP-like packet model, receiver, jitter buffer, metrics collector, and scenario runner
- Audio-only and audio+video modes
- Base latency, variable jitter, packet loss, burst loss, reordering, bandwidth cap, and queue pressure
- Fixed and adaptive jitter buffer strategies
- Headless CLI mode and local HTTP serve mode for the dashboard
- JSON scenario config loading and result export

### Frontend

- Dark, responsive dashboard with scenario controls on the left and monitoring panels on the right
- Live reconnect behavior if the backend starts after the page loads
- Summary cards for latency, jitter, packet loss, throughput, smoothness, and runtime clock
- Live charts for latency, jitter, packet loss, buffer occupancy, and throughput
- Event log for drops, reorder observations, underflows, buffer changes, and late packets
- Seeded replay controls, scenario explanation panel, export button, and resilient backend error state
- Tunable transport, playout, and basic stream pacing controls from the sidebar

## Repository layout

```text
include/mediaflow/   Public C++ headers
src/                 Backend implementation
tests/               GoogleTest coverage
configs/             Built-in scenario JSON files and custom config examples
docs/                Architecture, design decisions, interview notes, resume materials
scripts/             Demo, dashboard, and quality-check scripts
examples/            Exported sample result files
ui/                  React + TypeScript + Tailwind dashboard
```

## Build the backend

### Requirements

- CMake 3.25+
- C++20 compiler
- Node.js 20+ for the dashboard

### Configure and build

```bash
cmake -S . -B build -DMEDIAFLOW_BUILD_TESTS=ON
cmake --build build -j4
```

## Run tests

```bash
ctest --test-dir build --output-on-failure
```

Or run the combined check script:

```bash
./scripts/check.sh
```

`./scripts/check.sh` performs an isolated backend test build in `build-tests/` and then runs the frontend production build.

## Launch the frontend

```bash
cd ui
npm install
npm run dev
```

The Vite dev server proxies `/api/*` to `http://127.0.0.1:8080`.

## Launch the backend API for the UI

```bash
./build/mediaflow --serve
```

You can also use the helper script:

```bash
./scripts/run_dashboard_dev.sh
```

## Run a headless demo scenario

```bash
./build/mediaflow --scenario stable --seed 42 --export examples/stable-demo-result.json
```

Non-serve CLI runs are headless by default. The binary also accepts `--headless` for explicit script parity:

```bash
./build/mediaflow --headless --scenario stable --seed 42
```

## Load a JSON config directly

```bash
./build/mediaflow --config configs/custom-audio-only.json --seed 1337 --export examples/custom-audio-only-result.json
```

## Predefined scenarios

- `stable`
- `moderate-jitter`
- `high-jitter`
- `mild-loss`
- `burst-loss`
- `reorder-heavy`
- `bandwidth-constrained`
- `low-buffer-failure`
- `adaptive-buffer-comparison`
- `audio-only-stable`

## Backend and frontend communication

The dashboard uses HTTP polling rather than WebSockets. That choice keeps the runtime easy to inspect and demo. The Vite dev server proxy to `127.0.0.1:8080` was validated against the running backend.

- `GET /api/scenarios` returns scenario presets for the control panel.
- `POST /api/control/start` starts a deterministic run from a preset or a modified config.
- `GET /api/state` returns current summary metrics, time-series data, recent events, and active config.
- `GET /api/export` downloads the latest result JSON.
- Invalid `POST /api/control/start` payloads return `400` JSON errors instead of generic server failures.

## Architecture summary

- `SimulationEngine` advances the deterministic clock and coordinates the entire pipeline.
- `NetworkSimulator` applies transport impairments and queueing behavior.
- `JitterBuffer` controls playout timing and exposes fixed/adaptive buffering tradeoffs.
- `MetricsCollector` records summary counters, time-series telemetry, and event logs.
- `HttpServer` exposes a control plane for the React dashboard.

More detail lives in [docs/architecture.md](./docs/architecture.md).

## Design tradeoffs

- Polling was chosen over WebSockets to keep the transport and demo surface simple.
- The packet model is RTP-like, not RTP compliant.
- The jitter buffer is frame-oriented to keep playout logic understandable and testable.
- Adaptive buffering is intentionally lightweight and deterministic rather than overly heuristic.

More rationale lives in [docs/design_decisions.md](./docs/design_decisions.md).

## What was intentionally simplified

- No codec, media payload, or rendering pipeline
- No RTCP feedback, retransmission, FEC, or congestion-control protocol
- No production networking stack or cloud deployment
- No claim of WebRTC or RTP standards compliance

These simplifications were deliberate so the project stays centered on timing, buffering, transport impairments, and observability.

## How this maps to real-time media systems

This project mirrors the kinds of behaviors collaboration/media teams care about:

- sequence and timestamp integrity
- jitter and playout delay tradeoffs
- loss bursts versus random loss
- bandwidth pressure and queue collapse
- deterministic replay for debugging QoE regressions
- telemetry that explains why a run degraded

## Screenshots

The repo does not check in screenshots by default. To generate interview screenshots:

1. Run `./build/mediaflow --serve`
2. In another terminal, run `cd ui && npm run dev`
3. Open the Vite URL, start `adaptive-buffer-comparison`, and capture:
   - the summary cards
   - the latency/jitter charts
   - the event log during the run

## Responsible AI use

AI assistance was used to accelerate implementation and iteration, but not as an authority. The architecture, naming, scenario behavior, tests, and documentation were reviewed and corrected against the actual build and runtime behavior. The final repository was validated by:

- compiling the backend
- compiling the frontend
- running GoogleTest/CTest
- executing real headless scenarios
- validating the HTTP control plane used by the dashboard

## Interview support materials

- [docs/interview_notes.md](./docs/interview_notes.md)
- [docs/resume_materials.md](./docs/resume_materials.md)

## Suggested demo flow

1. Start with `stable` to show the baseline pipeline and deterministic replay.
2. Switch to `low-buffer-failure` to explain why low playout delay breaks under jitter.
3. Run `adaptive-buffer-comparison` and point out how delay strategy affects smoothness.
4. Finish with `bandwidth-constrained` to discuss queue pressure and transport collapse.
