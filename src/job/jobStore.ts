import type { ComputeJobError, ComputeJobState } from "../types/providerTypes";

function cloneJobState(jobState: ComputeJobState): ComputeJobState {
    return structuredClone(jobState);
}

export class InMemoryJobStore {
    private job: ComputeJobState | null = null;

    createQueued(input: { algorithmName: string; requestId?: string }): ComputeJobState {
        const jobState: ComputeJobState = {
            jobId: crypto.randomUUID(),
            status: "queued",
            algorithmName: input.algorithmName,
            requestId: input.requestId,
            createdAt: new Date().toISOString(),
        };

        this.job = jobState;
        return cloneJobState(jobState);
    }

    get(jobId: string): ComputeJobState | undefined {
        if (this.job?.jobId !== jobId) {
            return undefined;
        }
        return cloneJobState(this.job);
    }

    markRunning(jobId: string): ComputeJobState | undefined {
        if (this.job?.jobId !== jobId) {
            return undefined;
        }

        this.job.status = "running";
        this.job.startedAt = new Date().toISOString();
        return cloneJobState(this.job);
    }

    markCompleted(jobId: string, result: Record<string, unknown>): ComputeJobState | undefined {
        if (this.job?.jobId !== jobId) {
            return undefined;
        }

        this.job.status = "completed";
        this.job.result = structuredClone(result);
        this.job.completedAt = new Date().toISOString();
        this.job.error = undefined;
        return cloneJobState(this.job);
    }

    markFailed(jobId: string, error: ComputeJobError): ComputeJobState | undefined {
        if (this.job?.jobId !== jobId) {
            return undefined;
        }

        this.job.status = "failed";
        this.job.error = { ...error };
        this.job.completedAt = new Date().toISOString();
        return cloneJobState(this.job);
    }
}
