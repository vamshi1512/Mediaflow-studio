import type { Dispatch, SetStateAction } from "react";

import type { ScenarioConfig, SimulationStatus } from "../lib/types";

type ConnectionState = "connecting" | "online" | "offline";
type PendingAction = "start" | "pause" | "resume" | "reset" | "export" | null;

type SidebarProps = {
  scenarios: ScenarioConfig[];
  draftConfig: ScenarioConfig | null;
  status: SimulationStatus;
  seed: number;
  onSeedChange: (value: number) => void;
  onScenarioSelect: (scenarioName: string) => void;
  onUpdateConfig: Dispatch<SetStateAction<ScenarioConfig | null>>;
  onStart: () => void;
  onPause: () => void;
  onResume: () => void;
  onReset: () => void;
  onExport: () => void;
  onRestoreLive: () => void;
  onRestoreScenarioDefaults: () => void;
  disabled: boolean;
  pendingAction: PendingAction;
  isDirty: boolean;
  validationErrors: string[];
  connectionState: ConnectionState;
  lastSyncedLabel: string;
};

type SliderProps = {
  label: string;
  value: number;
  min: number;
  max: number;
  step?: number;
  suffix?: string;
  onChange: (value: number) => void;
};

function SliderField({
  label,
  value,
  min,
  max,
  step = 1,
  suffix = "",
  onChange,
}: SliderProps) {
  return (
    <label className="block space-y-2">
      <div className="flex items-center justify-between text-sm">
        <span className="text-chrome/70">{label}</span>
        <span className="font-medium text-white">
          {value}
          {suffix}
        </span>
      </div>
      <input
        type="range"
        className="h-2 w-full cursor-pointer appearance-none rounded-full bg-white/10 accent-cyan-300"
        value={value}
        min={min}
        max={max}
        step={step}
        onChange={(event) => onChange(Number(event.target.value))}
      />
    </label>
  );
}

function MiniButton({
  label,
  onClick,
  disabled,
}: {
  label: string;
  onClick: () => void;
  disabled: boolean;
}) {
  return (
    <button
      className="rounded-xl border border-white/10 bg-white/5 px-3 py-2 text-xs font-semibold uppercase tracking-[0.18em] text-chrome/70 transition hover:bg-white/10 disabled:cursor-not-allowed disabled:opacity-50"
      onClick={onClick}
      disabled={disabled}
    >
      {label}
    </button>
  );
}

const connectionTone: Record<ConnectionState, string> = {
  connecting: "border-amber-400/20 bg-amber-400/10 text-amber-200",
  online: "border-lime/20 bg-lime/10 text-lime",
  offline: "border-coral/20 bg-coral/10 text-coral",
};

export function ControlSidebar({
  scenarios,
  draftConfig,
  status,
  seed,
  onSeedChange,
  onScenarioSelect,
  onUpdateConfig,
  onStart,
  onPause,
  onResume,
  onReset,
  onExport,
  onRestoreLive,
  onRestoreScenarioDefaults,
  disabled,
  pendingAction,
  isDirty,
  validationErrors,
  connectionState,
  lastSyncedLabel,
}: SidebarProps) {
  if (!draftConfig) {
    return (
      <aside className="rounded-[2rem] border border-white/10 bg-panel/90 p-6 shadow-glow">
        <p className="text-sm text-chrome/70">Loading scenario controls…</p>
      </aside>
    );
  }

  const updateNetwork = (patch: Partial<ScenarioConfig["network"]>) =>
    onUpdateConfig((current) =>
      current ? { ...current, network: { ...current.network, ...patch } } : current,
    );
  const updateBuffer = (patch: Partial<ScenarioConfig["jitterBuffer"]>) =>
    onUpdateConfig((current) =>
      current ? { ...current, jitterBuffer: { ...current.jitterBuffer, ...patch } } : current,
    );
  const updateAudio = (patch: Partial<ScenarioConfig["audioStream"]>) =>
    onUpdateConfig((current) =>
      current ? { ...current, audioStream: { ...current.audioStream, ...patch } } : current,
    );
  const updateVideo = (patch: Partial<ScenarioConfig["videoStream"]>) =>
    onUpdateConfig((current) =>
      current ? { ...current, videoStream: { ...current.videoStream, ...patch } } : current,
    );

  const selectedScenario = scenarios.find((scenario) => scenario.name === draftConfig.name);
  const actionDisabled = disabled || pendingAction !== null;
  const canStart = !actionDisabled && validationErrors.length === 0;

  return (
    <aside className="rounded-[2rem] border border-white/10 bg-panel/90 p-6 shadow-glow backdrop-blur">
      <div className="space-y-6">
        <div className="rounded-[1.75rem] border border-white/10 bg-[linear-gradient(180deg,rgba(77,210,255,0.13),rgba(255,255,255,0.03))] p-5">
          <div className="flex items-start justify-between gap-3">
            <div>
              <div className="text-[11px] uppercase tracking-[0.3em] text-chrome/50">
                Scenario Runner
              </div>
              <h2 className="mt-2 font-display text-2xl font-semibold text-white">Controls</h2>
            </div>
            <span
              className={`rounded-full border px-3 py-1 text-[10px] font-semibold uppercase tracking-[0.22em] ${connectionTone[connectionState]}`}
            >
              {connectionState}
            </span>
          </div>
          <p className="mt-3 text-sm leading-6 text-chrome/70">
            Deterministic replay controls with live transport tuning. Edit the profile, run it, and
            then compare charts against the event trace.
          </p>
          <div className="mt-4 flex flex-wrap gap-2">
            <span className="rounded-full border border-white/10 bg-white/5 px-3 py-1 text-[10px] uppercase tracking-[0.2em] text-chrome/60">
              {lastSyncedLabel}
            </span>
            <span className="rounded-full border border-white/10 bg-white/5 px-3 py-1 text-[10px] uppercase tracking-[0.2em] text-chrome/60">
              {isDirty ? "Local edits pending" : "Draft mirrors live config"}
            </span>
          </div>
        </div>

        <div className="rounded-3xl border border-white/8 bg-white/5 p-4">
          <div className="flex items-start justify-between gap-3">
            <div>
              <div className="text-[10px] uppercase tracking-[0.24em] text-chrome/45">Selected preset</div>
              <div className="mt-2 font-display text-xl font-semibold text-white">{draftConfig.name}</div>
            </div>
            <div className="rounded-2xl border border-white/10 bg-white/5 px-3 py-2 text-xs uppercase tracking-[0.22em] text-chrome/55">
              {status}
            </div>
          </div>
          <p className="mt-3 text-sm leading-6 text-chrome/70">
            {selectedScenario?.description ?? draftConfig.description}
          </p>
          <div className="mt-4 grid grid-cols-2 gap-3 text-sm">
            <div className="rounded-2xl border border-white/8 bg-slate-950/30 p-3 text-chrome/70">
              <div className="text-[10px] uppercase tracking-[0.22em] text-chrome/45">Run length</div>
              <div className="mt-2 text-white">{draftConfig.durationMs / 1000}s</div>
            </div>
            <div className="rounded-2xl border border-white/8 bg-slate-950/30 p-3 text-chrome/70">
              <div className="text-[10px] uppercase tracking-[0.22em] text-chrome/45">Active streams</div>
              <div className="mt-2 text-white">
                {draftConfig.audioStream.enabled ? "Audio" : ""}
                {draftConfig.audioStream.enabled && draftConfig.videoStream.enabled ? " + " : ""}
                {draftConfig.videoStream.enabled ? "Video" : ""}
                {!draftConfig.audioStream.enabled && !draftConfig.videoStream.enabled ? "None" : ""}
              </div>
            </div>
          </div>
        </div>

        <div className="space-y-3">
          <label className="block text-sm text-chrome/70">
            Scenario
            <select
              className="mt-2 w-full rounded-2xl border border-white/10 bg-white/5 px-4 py-3 text-white outline-none transition focus:border-accent/50"
              value={draftConfig.name}
              onChange={(event) => onScenarioSelect(event.target.value)}
              disabled={actionDisabled}
            >
              {scenarios.map((scenario) => (
                <option key={scenario.name} value={scenario.name}>
                  {scenario.name}
                </option>
              ))}
            </select>
          </label>

          <label className="block text-sm text-chrome/70">
            Seed
            <input
              type="number"
              className="mt-2 w-full rounded-2xl border border-white/10 bg-white/5 px-4 py-3 text-white outline-none transition focus:border-accent/50"
              value={seed}
              onChange={(event) => onSeedChange(Number(event.target.value))}
              disabled={actionDisabled}
            />
          </label>

          <div className="grid grid-cols-2 gap-3">
            <button
              className="rounded-2xl bg-accent px-4 py-3 font-semibold text-slate-950 transition hover:brightness-110 disabled:cursor-not-allowed disabled:opacity-50"
              onClick={onStart}
              disabled={!canStart}
            >
              {pendingAction === "start" ? "Starting…" : "Start"}
            </button>
            <button
              className="rounded-2xl border border-white/10 bg-white/5 px-4 py-3 font-semibold text-white transition hover:bg-white/10 disabled:cursor-not-allowed disabled:opacity-50"
              onClick={status === "paused" ? onResume : onPause}
              disabled={
                actionDisabled || (status !== "running" && status !== "paused")
              }
            >
              {pendingAction === "pause" || pendingAction === "resume"
                ? "Updating…"
                : status === "paused"
                  ? "Resume"
                  : "Pause"}
            </button>
            <button
              className="rounded-2xl border border-white/10 bg-white/5 px-4 py-3 font-semibold text-white transition hover:bg-white/10 disabled:cursor-not-allowed disabled:opacity-50"
              onClick={onReset}
              disabled={actionDisabled}
            >
              {pendingAction === "reset" ? "Resetting…" : "Reset"}
            </button>
            <button
              className="rounded-2xl border border-lime/20 bg-lime/10 px-4 py-3 font-semibold text-lime transition hover:bg-lime/15 disabled:cursor-not-allowed disabled:opacity-50"
              onClick={onExport}
              disabled={actionDisabled}
            >
              {pendingAction === "export" ? "Exporting…" : "Export"}
            </button>
          </div>

          <div className="flex flex-wrap gap-2">
            <MiniButton
              label="Restore live"
              onClick={onRestoreLive}
              disabled={disabled}
            />
            <MiniButton
              label="Preset defaults"
              onClick={onRestoreScenarioDefaults}
              disabled={disabled}
            />
          </div>
        </div>

        {validationErrors.length > 0 ? (
          <div className="rounded-3xl border border-coral/20 bg-coral/10 p-4 text-sm text-coral">
            <div className="text-[10px] font-semibold uppercase tracking-[0.24em]">Configuration issues</div>
            <ul className="mt-3 space-y-2 leading-6">
              {validationErrors.map((issue) => (
                <li key={issue}>{issue}</li>
              ))}
            </ul>
          </div>
        ) : null}

        <div className="space-y-4 rounded-3xl border border-white/8 bg-white/5 p-4">
          <div className="flex items-center justify-between">
            <h3 className="font-display text-lg font-semibold text-white">Transport</h3>
            <span className="text-xs uppercase tracking-[0.22em] text-chrome/45">Seeded replay</span>
          </div>
          <SliderField
            label="Base latency"
            value={draftConfig.network.baseLatencyMs}
            min={10}
            max={200}
            suffix=" ms"
            onChange={(value) => updateNetwork({ baseLatencyMs: value })}
          />
          <SliderField
            label="Variable jitter"
            value={draftConfig.network.jitterMs}
            min={0}
            max={80}
            suffix=" ms"
            onChange={(value) => updateNetwork({ jitterMs: value })}
          />
          <SliderField
            label="Packet loss"
            value={Math.round(draftConfig.network.packetLossRate * 100)}
            min={0}
            max={30}
            suffix="%"
            onChange={(value) => updateNetwork({ packetLossRate: value / 100 })}
          />
          <SliderField
            label="Burst trigger"
            value={Math.round(draftConfig.network.burstLossTriggerRate * 100)}
            min={0}
            max={15}
            suffix="%"
            onChange={(value) => updateNetwork({ burstLossTriggerRate: value / 100 })}
          />
          <SliderField
            label="Burst length"
            value={draftConfig.network.burstLossLength}
            min={0}
            max={12}
            suffix=" pkts"
            onChange={(value) => updateNetwork({ burstLossLength: value })}
          />
          <SliderField
            label="Reorder chance"
            value={Math.round(draftConfig.network.reorderRate * 100)}
            min={0}
            max={20}
            suffix="%"
            onChange={(value) => updateNetwork({ reorderRate: value / 100 })}
          />
          <SliderField
            label="Reorder hold"
            value={draftConfig.network.reorderHoldMs}
            min={0}
            max={80}
            suffix=" ms"
            onChange={(value) => updateNetwork({ reorderHoldMs: value })}
          />
          <SliderField
            label="Bandwidth cap"
            value={draftConfig.network.bandwidthKbps}
            min={128}
            max={5000}
            step={32}
            suffix=" kbps"
            onChange={(value) => updateNetwork({ bandwidthKbps: value })}
          />
        </div>

        <div className="space-y-4 rounded-3xl border border-white/8 bg-white/5 p-4">
          <div className="flex items-center justify-between">
            <h3 className="font-display text-lg font-semibold text-white">Playout</h3>
            <label className="flex items-center gap-2 text-sm text-chrome/70">
              <input
                type="checkbox"
                checked={draftConfig.jitterBuffer.adaptive}
                onChange={(event) => updateBuffer({ adaptive: event.target.checked })}
              />
              Adaptive
            </label>
          </div>
          <SliderField
            label="Target delay"
            value={draftConfig.jitterBuffer.targetDelayMs}
            min={20}
            max={300}
            suffix=" ms"
            onChange={(value) => updateBuffer({ targetDelayMs: value })}
          />
          <SliderField
            label="Minimum delay"
            value={draftConfig.jitterBuffer.minDelayMs}
            min={20}
            max={200}
            suffix=" ms"
            onChange={(value) => updateBuffer({ minDelayMs: value })}
          />
          <SliderField
            label="Maximum delay"
            value={draftConfig.jitterBuffer.maxDelayMs}
            min={60}
            max={400}
            suffix=" ms"
            onChange={(value) => updateBuffer({ maxDelayMs: value })}
          />
          <SliderField
            label="Warm-up frames"
            value={draftConfig.jitterBuffer.startBufferFrames}
            min={1}
            max={8}
            suffix=" f"
            onChange={(value) => updateBuffer({ startBufferFrames: value })}
          />
          <SliderField
            label="Max buffer packets"
            value={draftConfig.jitterBuffer.maxPackets}
            min={64}
            max={1024}
            step={16}
            onChange={(value) => updateBuffer({ maxPackets: value })}
          />
        </div>

        <div className="space-y-4 rounded-3xl border border-white/8 bg-white/5 p-4">
          <div className="flex items-center justify-between">
            <h3 className="font-display text-lg font-semibold text-white">Streams</h3>
            <span className="text-xs uppercase tracking-[0.22em] text-chrome/45">Frame pacing</span>
          </div>
          <div className="grid grid-cols-2 gap-3">
            <label className="rounded-2xl border border-white/10 bg-white/5 p-3 text-sm text-chrome/70">
              <div className="mb-2">Audio stream</div>
              <input
                type="checkbox"
                checked={draftConfig.audioStream.enabled}
                onChange={(event) => updateAudio({ enabled: event.target.checked })}
              />
            </label>
            <label className="rounded-2xl border border-white/10 bg-white/5 p-3 text-sm text-chrome/70">
              <div className="mb-2">Video stream</div>
              <input
                type="checkbox"
                checked={draftConfig.videoStream.enabled}
                onChange={(event) => updateVideo({ enabled: event.target.checked })}
              />
            </label>
          </div>
          <SliderField
            label="Audio frame interval"
            value={draftConfig.audioStream.frameIntervalMs}
            min={10}
            max={60}
            suffix=" ms"
            onChange={(value) => updateAudio({ frameIntervalMs: value })}
          />
          <SliderField
            label="Video frame interval"
            value={draftConfig.videoStream.frameIntervalMs}
            min={16}
            max={50}
            suffix=" ms"
            onChange={(value) => updateVideo({ frameIntervalMs: value })}
          />
        </div>
      </div>
    </aside>
  );
}
