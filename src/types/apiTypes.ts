import type { InMemoryJobStore } from "../job/jobStore";

export interface ProviderConfig {
    port: number;
    providerName: string;
}

export interface ComputeWorkerHandle {
    activeWorker: Worker | null;
}

export interface ServerContext {
    config: ProviderConfig;
    jobs: InMemoryJobStore;
    workerHandle: ComputeWorkerHandle;
}