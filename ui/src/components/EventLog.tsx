import { useDeferredValue } from "react";

import type { EventRecord } from "../lib/types";

const severityTone: Record<EventRecord["severity"], string> = {
  info: "border-accent/20 bg-accent/10 text-accent",
  warning: "border-amber-400/20 bg-amber-400/10 text-amber-200",
  error: "border-coral/20 bg-coral/10 text-coral",
};

export function EventLog({ events }: { events: EventRecord[] }) {
  const deferredEvents = useDeferredValue(events);
  const severitySummary = deferredEvents.reduce(
    (summary, event) => {
      summary[event.severity] += 1;
      return summary;
    },
    { info: 0, warning: 0, error: 0 } as Record<EventRecord["severity"], number>,
  );

  return (
    <section className="rounded-[1.9rem] border border-white/10 bg-panelAlt/85 p-5 shadow-glow">
      <div className="mb-4 flex items-center justify-between">
        <div>
          <h3 className="font-display text-lg font-semibold text-white">Event Log</h3>
          <p className="text-sm text-chrome/70">
            Drops, reorder observations, underflows, and delay retuning events.
          </p>
        </div>
        <div className="text-xs uppercase tracking-[0.24em] text-chrome/50">
          {deferredEvents.length} entries
        </div>
      </div>
      <div className="mb-4 flex flex-wrap gap-2">
        {(["info", "warning", "error"] as const).map((severity) => (
          <span
            key={severity}
            className={`rounded-full border px-3 py-1 text-[10px] font-semibold uppercase tracking-[0.22em] ${severityTone[severity]}`}
          >
            {severity} {severitySummary[severity]}
          </span>
        ))}
      </div>
      <div className="max-h-[31rem] space-y-3 overflow-auto pr-1">
        {deferredEvents.length === 0 ? (
          <div className="rounded-2xl border border-dashed border-white/10 px-4 py-8 text-center text-sm text-chrome/60">
            No transport events yet. Start a scenario to inspect impairments and recovery.
          </div>
        ) : (
          deferredEvents
            .slice()
            .reverse()
            .map((event, index) => (
              <article
                key={`${event.timeMs}-${event.kind}-${index}`}
                className="rounded-2xl border border-white/8 bg-[linear-gradient(180deg,rgba(255,255,255,0.06),rgba(255,255,255,0.03))] p-4"
              >
                <div className="flex items-start justify-between gap-3">
                  <div>
                    <div className="font-medium text-white">{event.message}</div>
                    <div className="mt-1 text-xs uppercase tracking-[0.22em] text-chrome/50">
                      {event.kind.replaceAll("-", " ")} • {event.streamId || "system"}
                    </div>
                  </div>
                  <span
                    className={`rounded-full border px-2.5 py-1 text-[10px] font-semibold uppercase tracking-[0.22em] ${severityTone[event.severity]}`}
                  >
                    {event.severity}
                  </span>
                </div>
                <div className="mt-3 flex items-center justify-between text-xs text-chrome/55">
                  <span>t = {(event.timeMs / 1000).toFixed(2)}s</span>
                  <span>
                    packet {event.packetId ?? "n/a"} • frame {event.frameId ?? "n/a"}
                  </span>
                </div>
              </article>
            ))
        )}
      </div>
    </section>
  );
}
