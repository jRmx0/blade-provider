import { executeCoreCompute, CoreComputeError } from "../api/coreBridge";
import type { ComputeWorkerRequest, ComputeWorkerResponse } from "../types/providerTypes";

self.onmessage = (event: MessageEvent<ComputeWorkerRequest>) => {
    const { jobId, payload } = event.data;

    try {
        const result = executeCoreCompute(payload);

        const response: ComputeWorkerResponse = { jobId, type: "completed", result };
        self.postMessage(response);
    } catch (error) {
        const response: ComputeWorkerResponse = {
            jobId,
            type: "failed",
            error: {
                code: error instanceof CoreComputeError ? error.code : "internal_error",
                message: error instanceof Error ? error.message : String(error),
            },
        };
        self.postMessage(response);
    }
};
