type MetricCardProps = {
  label: string;
  value: string;
  accentClass: string;
  accentColor: string;
  footnote: string;
  eyebrow?: string;
};

export function MetricCard({
  label,
  value,
  accentClass,
  accentColor,
  footnote,
  eyebrow = "Live metric",
}: MetricCardProps) {
  return (
    <div className="rounded-[1.75rem] border border-white/10 bg-[linear-gradient(180deg,rgba(255,255,255,0.08),rgba(255,255,255,0.03))] p-5 shadow-glow backdrop-blur">
      <div className="flex items-start justify-between gap-3">
        <div className="text-[10px] uppercase tracking-[0.3em] text-chrome/45">{eyebrow}</div>
        <div className="h-2.5 w-2.5 rounded-full" style={{ backgroundColor: accentColor }} />
      </div>
      <div className="mt-4 text-[11px] uppercase tracking-[0.28em] text-chrome/60">{label}</div>
      <div className={`mt-3 font-display text-3xl font-semibold ${accentClass}`}>{value}</div>
      <div className="mt-3 border-t border-white/8 pt-3 text-sm text-chrome/70">{footnote}</div>
    </div>
  );
}
