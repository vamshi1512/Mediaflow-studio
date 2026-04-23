import type { RuntimeState, ScenarioConfig } from "./types";

async function parseJson<T>(response: Response): Promise<T> {
  const contentType = response.headers.get("content-type") ?? "";
  const isJson = contentType.includes("application/json");

  if (!response.ok) {
    if (isJson) {
      const payload = (await response.json()) as { error?: string };
      throw new Error(payload.error ?? `Request failed: ${response.status}`);
    }

    const text = await response.text();
    throw new Error(text || `Request failed: ${response.status}`);
  }

  if (!isJson) {
    throw new Error("Expected a JSON response from the backend.");
  }

  return (await response.json()) as T;
}

export async function fetchScenarios(): Promise<ScenarioConfig[]> {
  const response = await fetch("/api/scenarios");
  return parseJson<ScenarioConfig[]>(response);
}

export async function fetchState(): Promise<RuntimeState> {
  const response = await fetch("/api/state");
  return parseJson<RuntimeState>(response);
}

export async function postControl<T = RuntimeState>(
  action: "start" | "pause" | "resume" | "reset",
  payload?: unknown,
): Promise<T> {
  const response = await fetch(`/api/control/${action}`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: payload ? JSON.stringify(payload) : undefined,
  });
  return parseJson<T>(response);
}

export async function exportResults(): Promise<Blob> {
  const response = await fetch("/api/export");
  if (!response.ok) {
    throw new Error(`Export failed: ${response.status}`);
  }
  return response.blob();
}
