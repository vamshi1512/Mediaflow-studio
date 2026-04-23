import type { ScenarioConfig } from "./types";

export function cloneConfig(config: ScenarioConfig): ScenarioConfig {
  if (typeof structuredClone === "function") {
    return structuredClone(config);
  }

  return JSON.parse(JSON.stringify(config)) as ScenarioConfig;
}

export function configSignature(config: ScenarioConfig | null): string {
  return config ? JSON.stringify(config) : "";
}

export function validateDraftConfig(config: ScenarioConfig | null, seed: number): string[] {
  if (!config) {
    return ["Configuration is still loading."];
  }

  const errors: string[] = [];
  if (!Number.isFinite(seed) || seed < 0) {
    errors.push("Seed must be a non-negative number.");
  }
  if (config.durationMs <= 0) {
    errors.push("Scenario duration must be positive.");
  }
  if (config.sampleIntervalMs <= 0) {
    errors.push("Sampling interval must be positive.");
  }
  if (!config.audioStream.enabled && !config.videoStream.enabled) {
    errors.push("Enable at least one media stream before starting a run.");
  }
  if (config.network.bandwidthKbps <= 0) {
    errors.push("Bandwidth cap must be greater than zero.");
  }
  if (
    config.jitterBuffer.targetDelayMs < config.jitterBuffer.minDelayMs ||
    config.jitterBuffer.targetDelayMs > config.jitterBuffer.maxDelayMs
  ) {
    errors.push("Jitter buffer target delay must stay within min/max bounds.");
  }

  return errors;
}
