export type SimulationStatus = "idle" | "running" | "paused" | "completed" | "error";

export type MediaStreamConfig = {
  enabled: boolean;
  streamId: string;
  mediaType: "audio" | "video";
  frameIntervalMs: number;
  framePayloadBytes: number;
  packetsPerFrame: number;
};

export type NetworkConfig = {
  baseLatencyMs: number;
  jitterMs: number;
  packetLossRate: number;
  burstLossTriggerRate: number;
  burstLossLength: number;
  reorderRate: number;
  reorderHoldMs: number;
  bandwidthKbps: number;
  queueCapacityPackets: number;
  queuePressureDropThresholdMs: number;
};

export type JitterBufferConfig = {
  targetDelayMs: number;
  minDelayMs: number;
  maxDelayMs: number;
  maxPackets: number;
  adaptive: boolean;
  startBufferFrames: number;
};

export type ScenarioConfig = {
  name: string;
  description: string;
  durationMs: number;
  seed: number;
  sampleIntervalMs: number;
  audioStream: MediaStreamConfig;
  videoStream: MediaStreamConfig;
  network: NetworkConfig;
  jitterBuffer: JitterBufferConfig;
};

export type SummaryMetrics = {
  sentPackets: number;
  receivedPackets: number;
  playedPackets: number;
  sentFrames: number;
  playedFrames: number;
  droppedPackets: number;
  latePackets: number;
  reorderCount: number;
  underflowCount: number;
  overflowCount: number;
  networkDropCount: number;
  avgLatencyMs: number;
  p95LatencyMs: number;
  jitterEstimateMs: number;
  packetLossRatePct: number;
  throughputKbps: number;
  playoutSmoothnessPct: number;
  durationMs: number;
};

export type TimePoint = {
  timeMs: number;
  value: number;
};

export type EventRecord = {
  timeMs: number;
  kind: string;
  severity: "info" | "warning" | "error";
  message: string;
  streamId: string;
  packetId: number | null;
  frameId: number | null;
};

export type RuntimeState = {
  status: SimulationStatus;
  scenarioName: string;
  scenarioDescription: string;
  seed: number;
  simulationTimeMs: number;
  durationMs: number;
  bufferStrategy: string;
  summary: SummaryMetrics;
  series: {
    latencyMs: TimePoint[];
    jitterMs: TimePoint[];
    packetLossPct: TimePoint[];
    bufferOccupancyPackets: TimePoint[];
    throughputKbps: TimePoint[];
  };
  recentEvents: EventRecord[];
  config: ScenarioConfig;
};
