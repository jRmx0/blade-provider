import type { InMemoryJobStore } from "../job/jobStore";

export interface ProviderConfig {
    port: number;
    providerName: string;
}

export interface ServerContext {
    config: ProviderConfig;
    jobs: InMemoryJobStore;
}