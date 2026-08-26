// Colours used to tell processes apart in the Gantt chart.
//
// These are a validated categorical palette stepped for a dark surface: every
// pair stays distinguishable to colour-blind readers, and every colour clears
// 3:1 contrast against the page background. They are assigned in a fixed order
// and never cycled.
//
// Colour is only ever a secondary cue here - every block also carries the
// process id as a label, so the chart still reads without colour at all.
export const SERIES_COLORS = [
  "#3987e5", // blue
  "#d95926", // orange
  "#199e70", // aqua
  "#c98500", // yellow
  "#d55181", // magenta
  "#008300", // green
  "#9085e9", // violet
  "#e66767", // red
] as const;

// Past the eighth process we stop inventing hues and fall back to a neutral.
// The label on the block still identifies it.
const OVERFLOW_COLOR = "#64748b";

export const IDLE_COLOR = "#334155";

// Map each process id to a colour, fixed by its position in the workload.
//
// Keying on the process rather than on the order it happens to run matters:
// re-running with a different algorithm reorders the timeline, and a process
// that changed colour between runs would be impossible to follow.
export function buildColorMap(ids: string[]): Map<string, string> {
  const colors = new Map<string, string>();
  ids.forEach((id, index) => {
    colors.set(id, index < SERIES_COLORS.length ? SERIES_COLORS[index] : OVERFLOW_COLOR);
  });
  return colors;
}
