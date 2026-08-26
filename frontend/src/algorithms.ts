// The algorithms the C++ program knows about, and a plain-English line about
// what each one trades away. Kept here so both views say the same thing.

export const ALGORITHMS = ["FCFS", "SJF", "SRTF", "RR", "Priority"] as const;

export const DESCRIPTIONS: Record<string, string> = {
  FCFS: "Runs in arrival order, each to completion. Simple, but one long job delays everyone behind it.",
  SJF: "Runs the shortest waiting job first. Best average waiting time, but long jobs can starve.",
  SRTF: "Like SJF, but a shorter arrival takes over immediately. Best waiting time of all, at the cost of switching.",
  RR: "Everyone takes turns of a fixed length. Nobody starves and it feels responsive, but nothing finishes early.",
  Priority: "Most important first. Aging raises the priority of anything left waiting, so nothing starves forever.",
};
