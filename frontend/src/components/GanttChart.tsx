import { motion } from "framer-motion";
import { useState } from "react";

import { IDLE_COLOR } from "../theme";
import type { TimeSlice } from "../types";

interface Props {
  timeline: TimeSlice[];
  colors: Map<string, string>;
}

interface HoverInfo {
  slice: TimeSlice;
  x: number;
}

// Blocks narrower than this get no label - there is nowhere to put it.
const MIN_LABEL_WIDTH_PERCENT = 4;

export default function GanttChart({ timeline, colors }: Props) {
  const [hover, setHover] = useState<HoverInfo | null>(null);

  if (timeline.length === 0) return null;

  const totalTime = timeline[timeline.length - 1].end;

  // Tick marks at every block boundary, plus the finish time.
  const boundaries = [...timeline.map((slice) => slice.start), totalTime];

  return (
    <div className="relative">
      <div className="flex h-16 w-full gap-[2px]">
        {timeline.map((slice, index) => {
          const widthPercent = ((slice.end - slice.start) / totalTime) * 100;
          const isIdle = slice.processId === null;
          const color = isIdle ? IDLE_COLOR : (colors.get(slice.processId!) ?? IDLE_COLOR);

          return (
            <motion.button
              key={`${slice.start}-${slice.processId}`}
              type="button"
              initial={{ opacity: 0, scaleX: 0.3 }}
              animate={{ opacity: 1, scaleX: 1 }}
              transition={{ duration: 0.25, delay: index * 0.03, ease: "easeOut" }}
              style={{ width: `${widthPercent}%`, backgroundColor: color, transformOrigin: "left" }}
              className={`relative flex min-w-[3px] items-center justify-center rounded-[4px] focus:outline-none focus-visible:ring-2 focus-visible:ring-white ${
                isIdle ? "opacity-60" : ""
              }`}
              onMouseEnter={(event) =>
                setHover({ slice, x: event.currentTarget.offsetLeft + event.currentTarget.offsetWidth / 2 })
              }
              onFocus={(event) =>
                setHover({ slice, x: event.currentTarget.offsetLeft + event.currentTarget.offsetWidth / 2 })
              }
              onMouseLeave={() => setHover(null)}
              onBlur={() => setHover(null)}
            >
              {widthPercent >= MIN_LABEL_WIDTH_PERCENT && (
                <span
                  className={`font-mono text-xs font-medium ${
                    isIdle ? "text-slate-300" : "text-white"
                  }`}
                >
                  {slice.processId ?? "idle"}
                </span>
              )}
            </motion.button>
          );
        })}
      </div>

      {/* Time axis. Boundaries that would collide are dropped rather than
          overlapping, which is why this walks the list tracking the last
          position it drew at. */}
      <div className="relative mt-1 h-5">
        {boundaries.map((tick, index) => {
          const left = (tick / totalTime) * 100;
          const previous = index > 0 ? (boundaries[index - 1] / totalTime) * 100 : -100;
          if (index > 0 && left - previous < 4) return null;
          return (
            <span
              key={tick}
              className="absolute font-mono text-[11px] text-ink-dim"
              style={{ left: `${left}%`, transform: "translateX(-50%)" }}
            >
              {tick}
            </span>
          );
        })}
      </div>

      {hover && (
        <div
          className="pointer-events-none absolute -top-16 z-10 rounded-md border border-edge bg-surface px-3 py-2 text-xs shadow-lg"
          style={{ left: hover.x, transform: "translateX(-50%)" }}
        >
          <div className="font-medium text-ink">{hover.slice.processId ?? "CPU idle"}</div>
          <div className="mt-0.5 font-mono text-ink-dim">
            {hover.slice.start} to {hover.slice.end} ({hover.slice.end - hover.slice.start}{" "}
            {hover.slice.end - hover.slice.start === 1 ? "tick" : "ticks"})
          </div>
        </div>
      )}
    </div>
  );
}
