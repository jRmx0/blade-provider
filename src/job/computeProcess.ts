import { executeCoreCompute, CoreComputeError } from "../api/coreBridge";
import type { ComputeProcessRequest, ComputeProcessResponse } from "../types/providerTypes";

const stdinText = await Bun.stdin.text();

let response: ComputeProcessResponse;

try {
    const req = JSON.parse(stdinText.trim()) as ComputeProcessRequest;

    try {
        const result = executeCoreCompute(req.payload);
        response = { jobId: req.jobId, type: "completed", result };
    } catch (error) {
        response = {
            jobId: req.jobId,
            type: "failed",
            error: {
                code: error instanceof CoreComputeError ? error.code : "internal_error",
                message: error instanceof Error ? error.message : String(error),
            },
        };
    }
} catch {
    response = {
        jobId: "unknown",
        type: "failed",
        error: { code: "invalid_request", message: "Failed to parse compute request from stdin." },
    };
}

await Bun.write(Bun.stdout, JSON.stringify(response) + "\n");

