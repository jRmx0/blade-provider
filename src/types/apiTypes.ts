import type { InMemoryJobStore } from "../job/jobStore";

export interface ProviderConfig {
    port: number;
    providerName: string;
}

export interface ComputeProcessHandle {
    activeProcess: { kill(): void } | null;
}

export interface ServerContext {
    config: ProviderConfig;
    jobs: InMemoryJobStore;
    processHandle: ComputeProcessHandle;
}