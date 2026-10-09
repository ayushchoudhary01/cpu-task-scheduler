// These mirror the JSON the C++ program prints. Keeping them in one file means
// the browser and the server agree on the shape of a simulation.

export interface Process {
  id: string;
  arrivalTime: number;
  burstTime: number;
  priority: number;
}

// "running" - a process held the CPU
// "idle"    - nothing was ready to run
// "switch"  - the CPU was changing from one process to another, doing no work
export type SliceKind = "running" | "idle" | "switch";

export interface TimeSlice {
  start: number;
  end: number;
  kind: SliceKind;
  processId: string | null; // set only when kind is "running"
}

export interface ProcessMetrics {
  id: string;
  arrivalTime: number;
  burstTime: number;
  completionTime: number;
  turnaroundTime: number;
  waitingTime: number;
  responseTime: number;
}

export interface Averages {
  waitingTime: number;
  turnaroundTime: number;
  responseTime: number;
  cpuUtilization: number;
  throughput: number;
}

export interface SimulationResult {
  algorithm: string;
  totalTime: number;
  busyTime: number;
  switchTime: number;
  timeline: TimeSlice[];
  processes: ProcessMetrics[];
  averages: Averages;
}

export interface SimulationRequest {
  processes: Process[];
  algorithm?: string;
  quantum?: number;
  agingRate?: number;
  switchCost?: number;
  compare?: string[];
}

export interface ApiError {
  errors: string[];
}
