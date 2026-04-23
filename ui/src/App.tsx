import {
  startTransition,
  useEffect,
  useMemo,
  useRef,
  useState,
  type Dispatch,
  type SetStateAction,
} from "react";

import { ChartPanel } from "./components/ChartPanel";
import { ControlSidebar } from "./components/ControlSidebar";
import { EventLog } from "./components/EventLog";
import { MetricCard } from "./components/MetricCard";
import { ScenarioPanel } from "./components/ScenarioPanel";
import { StatusBadge } from "./components/StatusBadge";
import { exportResults, fetchScenarios, fetchState, postControl } from "./lib/api";
import { cloneConfig, configSignature, validateDraftConfig } from "./lib/config";
import {
  formatClock,
  formatCount,
  formatKbps,
  formatMs,
  formatPct,
  formatRelativeSync,
} from "./lib/format";
import type { RuntimeState, ScenarioConfig } from "./lib/types";

type ConnectionState = "connecting" | "online" | "offline";
type PendingAction = "start" | "pause" | "resume" | "reset" | "export" | null;

function getErrorMessage(error: unknown): string {
  return error instanceof Error ? error.message : "Unexpected application error.";
}

export default function App() {
  const [scenarios, setScenarios] = useState<ScenarioConfig[]>([]);
  const [draftConfig, setDraftConfig] = useState<ScenarioConfig | null>(null);
  const [runtimeState, setRuntimeState] = useState<RuntimeState | null>(null);
  const [seed, setSeed] = useState<number>(42);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [connectionState, setConnectionState] = useState<ConnectionState>("connecting");
  const [lastSyncedAt, setLastSyncedAt] = useState<number | null>(null);
  const [pendingAction, setPendingAction] = useState<PendingAction>(null);
  const [localEdits, setLocalEdits] = useState(false);

  const draftConfigRef = useRef<ScenarioConfig | null>(null);
  const scenariosRef = useRef<ScenarioConfig[]>([]);
  const localEditsRef = useRef(false);
  const syncInFlightRef = useRef(false);

  useEffect(() => {
    draftConfigRef.current = draftConfig;
  }, [draftConfig]);

  useEffect(() => {
    scenariosRef.current = scenarios;
  }, [scenarios]);

  useEffect(() => {
    localEditsRef.current = localEdits;
  }, [localEdits]);

  useEffect(() => {
    let cancelled = false;

    const refreshSnapshot = async (includeCatalog: boolean) => {
      if (syncInFlightRef.current) {
        return;
      }

      syncInFlightRef.current = true;
      try {
        const [nextState, nextScenarios] = await Promise.all([
          fetchState(),
          includeCatalog || scenariosRef.current.length === 0
            ? fetchScenarios()
            : Promise.resolve(null),
        ]);

        if (cancelled) {
          return;
        }

        startTransition(() => {
          setRuntimeState(nextState);
          if (nextScenarios) {
            setScenarios(nextScenarios);
          }
          if (draftConfigRef.current === null || !localEditsRef.current) {
            setDraftConfig(cloneConfig(nextState.config));
            setSeed(nextState.seed);
            setLocalEdits(false);
          }
          setConnectionState("online");
          setLastSyncedAt(Date.now());
          setError(null);
          setLoading(false);
        });
      } catch (requestError) {
        if (!cancelled) {
          startTransition(() => {
            setConnectionState("offline");
            setError(getErrorMessage(requestError));
            setLoading(false);
          });
        }
      } finally {
        syncInFlightRef.current = false;
      }
    };

    void refreshSnapshot(true);
    const interval = window.setInterval(() => {
      void refreshSnapshot(false);
    }, 900);

    return () => {
      cancelled = true;
      window.clearInterval(interval);
    };
  }, []);

  const applyDraftUpdate: Dispatch<SetStateAction<ScenarioConfig | null>> = (update) => {
    setLocalEdits(true);
    setDraftConfig((current) =>
      typeof update === "function"
        ? (update as (previous: ScenarioConfig | null) => ScenarioConfig | null)(current)
        : update,
    );
  };

  const runControlAction = async (
    action: Exclude<PendingAction, "export">,
    request: () => Promise<RuntimeState>,
    options?: { adoptLiveDraft?: boolean },
  ) => {
    setPendingAction(action);
    try {
      const response = await request();
      startTransition(() => {
        setRuntimeState(response);
        setConnectionState("online");
        setLastSyncedAt(Date.now());
        setError(null);
        if (options?.adoptLiveDraft) {
          setDraftConfig(cloneConfig(response.config));
          setSeed(response.seed);
          setLocalEdits(false);
        }
      });
    } catch (requestError) {
      setError(getErrorMessage(requestError));
    } finally {
      setPendingAction(null);
    }
  };

  const handleScenarioSelect = (scenarioName: string) => {
    const scenario = scenarios.find((entry) => entry.name === scenarioName);
    if (!scenario) {
      return;
    }

    startTransition(() => {
      setDraftConfig(cloneConfig(scenario));
      setSeed(scenario.seed);
      setLocalEdits(true);
    });
  };

  const handleRestoreLive = () => {
    if (!runtimeState) {
      return;
    }

    startTransition(() => {
      setDraftConfig(cloneConfig(runtimeState.config));
      setSeed(runtimeState.seed);
      setLocalEdits(false);
      setError(null);
    });
  };

  const handleRestoreScenarioDefaults = () => {
    if (!draftConfig) {
      return;
    }

    const preset = scenarios.find((scenario) => scenario.name === draftConfig.name);
    if (!preset) {
      return;
    }

    startTransition(() => {
      setDraftConfig(cloneConfig(preset));
      setSeed(preset.seed);
      setLocalEdits(true);
    });
  };

  const handleStart = async () => {
    if (!draftConfig) {
      return;
    }

    await runControlAction(
      "start",
      () =>
        postControl<RuntimeState>("start", {
          seed,
          config: {
            ...draftConfig,
            seed,
          },
        }),
      { adoptLiveDraft: true },
    );
  };

  const handlePause = async () => {
    await runControlAction("pause", () => postControl<RuntimeState>("pause"));
  };

  const handleResume = async () => {
    await runControlAction("resume", () => postControl<RuntimeState>("resume"));
  };

  const handleReset = async () => {
    await runControlAction("reset", () => postControl<RuntimeState>("reset"));
  };

  const handleExport = async () => {
    setPendingAction("export");
    try {
      const blob = await exportResults();
      const url = window.URL.createObjectURL(blob);
      const anchor = document.createElement("a");
      anchor.href = url;
      anchor.download = "mediaflow-export.json";
      anchor.click();
      window.URL.revokeObjectURL(url);
      setError(null);
    } catch (requestError) {
      setError(getErrorMessage(requestError));
    } finally {
      setPendingAction(null);
    }
  };

  const selectedSummary = runtimeState?.summary;
  const validationErrors = useMemo(
    () => validateDraftConfig(draftConfig, seed),
    [draftConfig, seed],
  );
  const isDirty =
    runtimeState !== null &&
    draftConfig !== null &&
    (configSignature(runtimeState.config) !== configSignature(draftConfig) || runtimeState.seed !== seed);

  const cards = useMemo(() => {
    if (!selectedSummary || !runtimeState) {
      return [];
    }

    return [
      {
        label: "Average Latency",
        value: formatMs(selectedSummary.avgLatencyMs),
        accentClass: "text-accent",
        accentColor: "#4dd2ff",
        footnote: `P95 ${formatMs(selectedSummary.p95LatencyMs)}`,
        eyebrow: "Network quality",
      },
      {
        label: "Jitter Estimate",
        value: formatMs(selectedSummary.jitterEstimateMs),
        accentClass: "text-violet",
        accentColor: "#8fb6ff",
        footnote: `${formatCount(selectedSummary.reorderCount)} reorder observations`,
        eyebrow: "Transit variance",
      },
      {
        label: "Packet Loss",
        value: formatPct(selectedSummary.packetLossRatePct),
        accentClass: "text-coral",
        accentColor: "#ff7a66",
        footnote: `${formatCount(selectedSummary.droppedPackets)} dropped packets`,
        eyebrow: "Reliability",
      },
      {
        label: "Throughput",
        value: formatKbps(selectedSummary.throughputKbps),
        accentClass: "text-lime",
        accentColor: "#d7ff64",
        footnote: `${formatCount(selectedSummary.playedPackets)} packets played`,
        eyebrow: "Delivery rate",
      },
      {
        label: "Smoothness",
        value: formatPct(selectedSummary.playoutSmoothnessPct),
        accentClass: "text-white",
        accentColor: "#ffffff",
        footnote: `${formatCount(selectedSummary.underflowCount)} underflows`,
        eyebrow: "Playout health",
      },
      {
        label: "Run Clock",
        value: formatClock(runtimeState.simulationTimeMs, runtimeState.durationMs),
        accentClass: "text-white",
        accentColor: "#7ceec9",
        footnote: `${runtimeState.bufferStrategy} buffer strategy`,
        eyebrow: "Execution",
      },
    ];
  }, [runtimeState, selectedSummary]);

  return (
    <div className="min-h-screen bg-shell text-white">
      <div className="fixed inset-0 bg-[radial-gradient(circle_at_top_left,_rgba(77,210,255,0.18),_transparent_26%),radial-gradient(circle_at_center_right,_rgba(143,182,255,0.12),_transparent_25%),radial-gradient(circle_at_bottom_right,_rgba(255,122,102,0.12),_transparent_22%)]" />
      <div className="fixed inset-0 bg-dashboard-grid bg-[size:72px_72px] opacity-[0.06]" />
      <main className="relative mx-auto max-w-[1650px] px-4 py-5 md:px-6 xl:px-8">
        <header className="mb-6 rounded-[2.25rem] border border-white/10 bg-panel/75 px-6 py-6 shadow-glow backdrop-blur">
          <div className="flex flex-col gap-5 xl:flex-row xl:items-end xl:justify-between">
            <div>
              <div className="text-[11px] uppercase tracking-[0.34em] text-chrome/50">
                MediaFlow Studio
              </div>
              <h1 className="mt-3 font-display text-4xl font-semibold text-white md:text-5xl xl:text-[3.5rem]">
                Real-Time Media Transport Simulator
              </h1>
              <p className="mt-4 max-w-4xl text-sm leading-7 text-chrome/75 md:text-base">
                Deterministic sender → impaired network → receiver → jitter buffer simulation with
                live telemetry. Tuned for media-systems interviews where you need to explain timing,
                buffering, loss, reorder, and recovery with confidence.
              </p>
            </div>
            <div className="flex flex-wrap items-center gap-3">
              <StatusBadge status={runtimeState?.status ?? "idle"} />
              <div className="rounded-full border border-white/10 bg-white/5 px-4 py-2 text-sm text-chrome/70">
                Scenario{" "}
                <span className="font-medium text-white">
                  {runtimeState?.scenarioName ?? draftConfig?.name ?? "loading"}
                </span>
              </div>
              <div className="rounded-full border border-white/10 bg-white/5 px-4 py-2 text-sm text-chrome/70">
                Seed <span className="font-medium text-white">{seed}</span>
              </div>
              <div className="rounded-full border border-white/10 bg-white/5 px-4 py-2 text-sm text-chrome/70">
                {formatRelativeSync(lastSyncedAt)}
              </div>
            </div>
          </div>

          <div className="mt-5 grid gap-3 md:grid-cols-3">
            <div className="rounded-2xl border border-white/10 bg-white/5 px-4 py-3 text-sm text-chrome/70">
              <div className="text-[10px] uppercase tracking-[0.24em] text-chrome/45">Connection</div>
              <div className="mt-2 text-white">
                {connectionState === "online"
                  ? "Backend connected and polling live state."
                  : connectionState === "connecting"
                    ? "Establishing backend connection."
                    : "Using the last known state while attempting to reconnect."}
              </div>
            </div>
            <div className="rounded-2xl border border-white/10 bg-white/5 px-4 py-3 text-sm text-chrome/70">
              <div className="text-[10px] uppercase tracking-[0.24em] text-chrome/45">Draft state</div>
              <div className="mt-2 text-white">
                {isDirty
                  ? "Controls have unsaved edits relative to the live simulator."
                  : "Control draft currently matches the live simulator config."}
              </div>
            </div>
            <div className="rounded-2xl border border-white/10 bg-white/5 px-4 py-3 text-sm text-chrome/70">
              <div className="text-[10px] uppercase tracking-[0.24em] text-chrome/45">Current action</div>
              <div className="mt-2 text-white">
                {pendingAction ? `${pendingAction} in progress…` : "Ready for another scenario run."}
              </div>
            </div>
          </div>

          {error ? (
            <div className="mt-5 rounded-2xl border border-coral/20 bg-coral/10 px-4 py-3 text-sm text-coral">
              {connectionState === "offline"
                ? `Backend communication issue: ${error}. Start the backend with ./build/mediaflow --serve and keep the page open; the controls will recover automatically once polling reconnects.`
                : error}
            </div>
          ) : null}
        </header>

        <div className="grid gap-6 xl:grid-cols-[390px_minmax(0,1fr)]">
          <ControlSidebar
            scenarios={scenarios}
            draftConfig={draftConfig}
            status={runtimeState?.status ?? "idle"}
            seed={seed}
            onSeedChange={(value) => {
              setSeed(value);
              setLocalEdits(true);
            }}
            onScenarioSelect={handleScenarioSelect}
            onUpdateConfig={applyDraftUpdate}
            onStart={handleStart}
            onPause={handlePause}
            onResume={handleResume}
            onReset={handleReset}
            onExport={handleExport}
            onRestoreLive={handleRestoreLive}
            onRestoreScenarioDefaults={handleRestoreScenarioDefaults}
            disabled={loading}
            pendingAction={pendingAction}
            isDirty={isDirty || localEdits}
            validationErrors={validationErrors}
            connectionState={connectionState}
            lastSyncedLabel={formatRelativeSync(lastSyncedAt)}
          />

          <section className="space-y-6">
            <ScenarioPanel draftConfig={draftConfig} runtimeState={runtimeState} />

            {loading ? (
              <div className="rounded-[2rem] border border-white/10 bg-panel/80 p-8 text-center text-chrome/70 shadow-glow">
                Connecting to the simulator backend and loading default scenarios…
              </div>
            ) : runtimeState ? (
              <>
                <div className="grid gap-4 md:grid-cols-2 2xl:grid-cols-3">
                  {cards.map((card) => (
                    <MetricCard key={card.label} {...card} />
                  ))}
                </div>

                <div className="grid gap-5 2xl:grid-cols-2">
                  <ChartPanel
                    title="Latency"
                    subtitle="End-to-end playout latency trend"
                    color="#4dd2ff"
                    data={runtimeState.series.latencyMs}
                    valueSuffix="ms"
                  />
                  <ChartPanel
                    title="Jitter"
                    subtitle="Receiver-side transit variation estimate"
                    color="#8fb6ff"
                    data={runtimeState.series.jitterMs}
                    valueSuffix="ms"
                  />
                  <ChartPanel
                    title="Packet Loss"
                    subtitle="Cumulative transport loss percentage"
                    color="#ff7a66"
                    data={runtimeState.series.packetLossPct}
                    valueSuffix="%"
                  />
                  <ChartPanel
                    title="Buffer Occupancy"
                    subtitle="Packets currently queued inside the jitter buffer"
                    color="#d7ff64"
                    data={runtimeState.series.bufferOccupancyPackets}
                    valueSuffix="packets"
                  />
                  <ChartPanel
                    title="Throughput"
                    subtitle="Received transport throughput across sampling windows"
                    color="#7ceec9"
                    data={runtimeState.series.throughputKbps}
                    valueSuffix="kbps"
                  />
                  <section className="rounded-[1.9rem] border border-white/10 bg-panelAlt/85 p-5 shadow-glow">
                    <div className="text-[10px] uppercase tracking-[0.28em] text-chrome/45">
                      Interview notes
                    </div>
                    <h3 className="mt-2 font-display text-2xl font-semibold text-white">
                      What to call out live
                    </h3>
                    <div className="mt-4 space-y-3">
                      {[
                        "The simulator is deterministic for a fixed seed, so failures are replayable and testable.",
                        "The adaptive jitter buffer shows a concrete latency versus smoothness tradeoff instead of a purely theoretical one.",
                        "The backend owns the timing model, packet model, and impairment logic; the UI is only a monitor and control surface.",
                        "The event log and charts are intentionally correlated, so you can point to a drop burst and then show the exact QoE impact.",
                      ].map((item) => (
                        <div
                          key={item}
                          className="rounded-2xl border border-white/8 bg-white/5 px-4 py-3 text-sm leading-6 text-chrome/75"
                        >
                          {item}
                        </div>
                      ))}
                    </div>
                  </section>
                </div>

                <EventLog events={runtimeState.recentEvents} />
              </>
            ) : (
              <div className="rounded-[2rem] border border-white/10 bg-panel/80 p-8 text-center text-chrome/70 shadow-glow">
                No runtime state available yet. Start the backend with `./build/mediaflow --serve`
                and the dashboard will reconnect automatically.
              </div>
            )}
          </section>
        </div>
      </main>
    </div>
  );
}
