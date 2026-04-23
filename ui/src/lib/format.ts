export function formatMs(value: number): string {
  return `${value.toFixed(value >= 100 ? 0 : 1)} ms`;
}

export function formatPct(value: number): string {
  return `${value.toFixed(value >= 10 ? 1 : 2)}%`;
}

export function formatKbps(value: number): string {
  return `${value.toFixed(value >= 1000 ? 0 : 1)} kbps`;
}

export function formatCount(value: number): string {
  return Intl.NumberFormat("en-US").format(Math.round(value));
}

export function formatClock(currentMs: number, totalMs: number): string {
  const currentSeconds = Math.floor(currentMs / 1000);
  const totalSeconds = Math.floor(totalMs / 1000);
  return `${currentSeconds.toString().padStart(2, "0")}s / ${totalSeconds.toString().padStart(2, "0")}s`;
}

export function formatRelativeSync(timestampMs: number | null): string {
  if (!timestampMs) {
    return "No live sync yet";
  }

  const deltaMs = Math.max(0, Date.now() - timestampMs);
  if (deltaMs < 1_000) {
    return "Synced just now";
  }
  if (deltaMs < 60_000) {
    return `Synced ${Math.round(deltaMs / 1_000)}s ago`;
  }

  return `Synced ${Math.round(deltaMs / 60_000)}m ago`;
}
