import type { RuntimeState, ScenarioConfig } from "../lib/types";

export function ScenarioPanel({
  draftConfig,
  runtimeState,
}: {
  draftConfig: ScenarioConfig | null;
  runtimeState: RuntimeState | null;
}) {
  const activeConfig = runtimeState?.config ?? draftConfig;

  if (!activeConfig) {
    return null;
  }

  return (
    <section className="rounded-[1.9rem] border border-white/10 bg-panelAlt/85 p-5 shadow-glow">
      <div className="mb-4 flex items-center justify-between gap-4">
        <div>
          <div className="text-[10px] uppercase tracking-[0.3em] text-chrome/45">Demo framing</div>
          <h3 className="font-display text-lg font-semibold text-white">
            Scenario Explanation
          </h3>
          <p className="text-sm text-chrome/70">
            Use this section as an interview walkthrough anchor while the run is live.
          </p>
        </div>
        <span className="rounded-full border border-accent/20 bg-accent/10 px-3 py-1 text-[11px] uppercase tracking-[0.24em] text-accent">
          {activeConfig.name}
        </span>
      </div>

      <p className="leading-7 text-chrome/80">{activeConfig.description}</p>

      <div className="mt-5 grid gap-3 md:grid-cols-4">
        <div className="rounded-2xl border border-white/8 bg-white/5 p-4">
          <div className="text-[11px] uppercase tracking-[0.22em] text-chrome/45">Network</div>
          <div className="mt-2 text-sm text-white">
            {activeConfig.network.baseLatencyMs} ms base, ±{activeConfig.network.jitterMs} ms jitter,
            {(activeConfig.network.packetLossRate * 100).toFixed(1)}% random loss
          </div>
        </div>
        <div className="rounded-2xl border border-white/8 bg-white/5 p-4">
          <div className="text-[11px] uppercase tracking-[0.22em] text-chrome/45">Buffer</div>
          <div className="mt-2 text-sm text-white">
            {activeConfig.jitterBuffer.targetDelayMs} ms target with{" "}
            {activeConfig.jitterBuffer.adaptive ? "adaptive retuning" : "fixed delay"} and{" "}
            {activeConfig.jitterBuffer.maxPackets} packet capacity
          </div>
        </div>
        <div className="rounded-2xl border border-white/8 bg-white/5 p-4">
          <div className="text-[11px] uppercase tracking-[0.22em] text-chrome/45">Streams</div>
          <div className="mt-2 text-sm text-white">
            {activeConfig.audioStream.enabled ? "Audio" : "No audio"} •{" "}
            {activeConfig.videoStream.enabled ? "Video" : "No video"} •{" "}
            {activeConfig.durationMs / 1000}s deterministic run
          </div>
        </div>
        <div className="rounded-2xl border border-white/8 bg-white/5 p-4">
          <div className="text-[11px] uppercase tracking-[0.22em] text-chrome/45">Interview angle</div>
          <div className="mt-2 text-sm text-white">
            Show how seeded impairment patterns map directly to latency, smoothness, and event-log
            changes.
          </div>
        </div>
      </div>
    </section>
  );
}
