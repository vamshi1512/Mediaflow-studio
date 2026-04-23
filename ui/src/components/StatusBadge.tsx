import type { SimulationStatus } from "../lib/types";

const tones: Record<SimulationStatus, string> = {
  idle: "border-white/10 bg-white/5 text-chrome",
  running: "border-accent/30 bg-accent/10 text-accent",
  paused: "border-amber-400/30 bg-amber-400/10 text-amber-300",
  completed: "border-lime/30 bg-lime/10 text-lime",
  error: "border-coral/30 bg-coral/10 text-coral",
};

export function StatusBadge({ status }: { status: SimulationStatus }) {
  return (
    <span
      className={`inline-flex rounded-full border px-3 py-1 text-[11px] font-semibold uppercase tracking-[0.28em] ${tones[status]}`}
    >
      {status}
    </span>
  );
}
