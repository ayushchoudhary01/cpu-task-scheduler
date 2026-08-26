// These mirror the JSON the C++ program prints. Keeping them in one file means
// the browser and the server agree on the shape of a simulation.

export interface Process {
  id: string;
  arrivalTime: number;
  burstTime: number;
  priority: number;
}

export interface TimeSlice {
  start: number;
  end: number;
  processId: string | null; // null while the CPU is idle
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
  timeline: TimeSlice[];
  processes: ProcessMetrics[];
  averages: Averages;
}

export interface SimulationRequest {
  processes: Process[];
  algorithm?: string;
  quantum?: number;
  agingRate?: number;
  compare?: string[];
}

export interface ApiError {
  errors: string[];
}
