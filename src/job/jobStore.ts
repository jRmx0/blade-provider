import type { ComputeJobError, ComputeJobState, ComputeResult } from "../types/providerTypes";

function cloneJobState(jobState: ComputeJobState): ComputeJobState {
    return structuredClone(jobState);
}

export class InMemoryJobStore {
    private readonly jobs = new Map<string, ComputeJobState>();

    createQueued(input: { algorithmName: string; requestId?: string }): ComputeJobState {
        const jobState: ComputeJobState = {
            jobId: crypto.randomUUID(),
            status: "queued",
            algorithmName: input.algorithmName,
            requestId: input.requestId,
            createdAt: new Date().toISOString(),
        };

        this.jobs.set(jobState.jobId, jobState);
        return cloneJobState(jobState);
    }

    get(jobId: string): ComputeJobState | undefined {
        const jobState = this.jobs.get(jobId);
        return jobState ? cloneJobState(jobState) : undefined;
    }

    markRunning(jobId: string): ComputeJobState | undefined {
        const jobState = this.jobs.get(jobId);
        if (!jobState) {
            return undefined;
        }

        jobState.status = "running";
        jobState.startedAt = new Date().toISOString();
        this.jobs.set(jobId, jobState);
        return cloneJobState(jobState);
    }

    markCompleted(jobId: string, result: ComputeResult): ComputeJobState | undefined {
        const jobState = this.jobs.get(jobId);
        if (!jobState) {
            return undefined;
        }

        jobState.status = "completed";
        jobState.result = structuredClone(result);
        jobState.completedAt = new Date().toISOString();
        jobState.error = undefined;
        this.jobs.set(jobId, jobState);
        return cloneJobState(jobState);
    }

    markFailed(jobId: string, error: ComputeJobError): ComputeJobState | undefined {
        const jobState = this.jobs.get(jobId);
        if (!jobState) {
            return undefined;
        }

        jobState.status = "failed";
        jobState.error = { ...error };
        jobState.completedAt = new Date().toISOString();
        this.jobs.set(jobId, jobState);
        return cloneJobState(jobState);
    }
}
