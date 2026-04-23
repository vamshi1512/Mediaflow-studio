import {
  Area,
  AreaChart,
  CartesianGrid,
  Line,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

import type { TimePoint } from "../lib/types";

type ChartPanelProps = {
  title: string;
  subtitle: string;
  color: string;
  data: TimePoint[];
  valueSuffix: string;
};

export function ChartPanel({
  title,
  subtitle,
  color,
  data,
  valueSuffix,
}: ChartPanelProps) {
  const latestPoint = data[data.length - 1];
  const gradientId = `${title.toLowerCase().replace(/\s+/g, "-")}-gradient`;

  return (
    <section className="rounded-[1.9rem] border border-white/10 bg-panelAlt/85 p-5 shadow-glow">
      <div className="mb-4 flex items-start justify-between gap-3">
        <div>
          <div className="text-[10px] uppercase tracking-[0.28em] text-chrome/45">Telemetry</div>
          <h3 className="mt-2 font-display text-xl font-semibold text-white">{title}</h3>
          <p className="text-sm text-chrome/70">{subtitle}</p>
        </div>
        <div className="rounded-2xl border border-white/10 bg-white/5 px-3 py-2 text-right">
          <div className="text-[10px] uppercase tracking-[0.24em] text-chrome/45">Latest</div>
          <div className="mt-1 text-sm font-medium text-white">
            {latestPoint ? `${latestPoint.value.toFixed(2)} ${valueSuffix}` : "No data"}
          </div>
        </div>
      </div>
      <div className="h-64">
        {data.length === 0 ? (
          <div className="flex h-full items-center justify-center rounded-3xl border border-dashed border-white/10 bg-white/[0.03] text-sm text-chrome/60">
            Start a run to populate this chart.
          </div>
        ) : (
          <ResponsiveContainer width="100%" height="100%">
            <AreaChart data={data} margin={{ top: 8, right: 12, left: 0, bottom: 0 }}>
              <defs>
                <linearGradient id={gradientId} x1="0" y1="0" x2="0" y2="1">
                  <stop offset="0%" stopColor={color} stopOpacity={0.36} />
                  <stop offset="100%" stopColor={color} stopOpacity={0.02} />
                </linearGradient>
              </defs>
              <CartesianGrid
                stroke="rgba(180, 194, 211, 0.12)"
                vertical={false}
              />
              <XAxis
                dataKey="timeMs"
                tickFormatter={(value) => `${Math.round(value / 1000)}s`}
                stroke="rgba(180, 194, 211, 0.45)"
                tickLine={false}
                axisLine={false}
                fontSize={12}
              />
              <YAxis
                stroke="rgba(180, 194, 211, 0.45)"
                tickLine={false}
                axisLine={false}
                fontSize={12}
              />
              <Tooltip
                cursor={{ stroke: "rgba(255,255,255,0.18)" }}
                contentStyle={{
                  backgroundColor: "#101927",
                  border: "1px solid rgba(255,255,255,0.08)",
                  borderRadius: "16px",
                }}
                formatter={(value: number) => [`${value.toFixed(2)} ${valueSuffix}`, title]}
                labelFormatter={(label) => `t=${Math.round(Number(label) / 1000)}s`}
              />
              <Area
                type="monotone"
                dataKey="value"
                stroke="none"
                fill={`url(#${gradientId})`}
              />
              <Line
                type="monotone"
                dataKey="value"
                stroke={color}
                strokeWidth={2.8}
                dot={false}
                activeDot={{ r: 4, fill: color }}
              />
            </AreaChart>
          </ResponsiveContainer>
        )}
      </div>
    </section>
  );
}
