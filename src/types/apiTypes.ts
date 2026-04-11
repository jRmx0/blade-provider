import type { InMemoryJobStore } from "../job/jobStore";

export type LogLevel = "error" | "warn" | "info" | "debug";

export interface ProviderConfig {
    port: number;
    providerName: string;
    logLevel: LogLevel;
}

export interface ComputeProcessHandle {
    activeProcess: { kill(): void } | null;
}

export interface ServerContext {
    config: ProviderConfig;
    jobs: InMemoryJobStore;
    processHandle: ComputeProcessHandle;
}