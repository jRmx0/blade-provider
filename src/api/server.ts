import type { ServerContext } from "../types/apiTypes";
import { getCoreMetadata } from "./coreBridge";
import { isRecord, parseRequestJsonBody } from "./jsonUtil";
import type {
    ComputeAcceptedResponse,
    ComputeJobState,
    ComputeWorkerRequest,
    ComputeWorkerResponse,
    ErrorResponse,
    HealthResponse,
} from "../types/providerTypes";

function getQueuedAlgorithmName(rawBody: unknown): string {
    if (!isRecord(rawBody)) {
        return "Unknown algorithm";
    }

    const algorithmId = rawBody.algorithmId;
    if (typeof algorithmId !== "number") {
        return "Unknown algorithm";
    }

    const metadata = getCoreMetadata();
    const algorithm = metadata.algorithms.find((a) => a.id === algorithmId);
    return algorithm?.name ?? "Unknown algorithm";
}

function getQueuedRequestId(rawBody: unknown): string | undefined {
    if (!isRecord(rawBody)) {
        return undefined;
    }

    return typeof rawBody.requestId === "string" && rawBody.requestId.trim() !== ""
        ? rawBody.requestId
        : undefined;
}

function jsonResponse(body: unknown, status = 200) {
    const headers = buildCorsHeaders();
    headers.set("Content-Type", "application/json");

    return new Response(JSON.stringify(body, null, 2), {
        status,
        headers,
    });
}

function emptyResponse(status = 204) {
    return new Response(null, {
        status,
        headers: buildCorsHeaders(),
    });
}

function buildCorsHeaders(): Headers {
    const headers = new Headers();
    headers.set("Access-Control-Allow-Origin", "*");
    headers.set("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    headers.set("Access-Control-Allow-Headers", "Content-Type, Authorization");
    headers.set("Access-Control-Max-Age", "86400");

    return headers;
}

function notFound() {
    const body: ErrorResponse = {
        error: {
            code: "not_found",
            message: "Route not found.",
        },
    };

    return jsonResponse(body, 404);
}

function methodNotAllowed(request: Request, allowedMethods: string[]) {
    const body: ErrorResponse = {
        error: {
            code: "method_not_allowed",
            message: `Method ${request.method} is not allowed for this route.`,
            details: { allowedMethods },
        },
    };

    return jsonResponse(body, 405);
}

async function handleHealth(request: Request, context: ServerContext) {
    if (request.method !== "GET") {
        return methodNotAllowed(request, ["GET", "OPTIONS"]);
    }

    const body: HealthResponse = {
        status: "ok",
        service: context.config.providerName,
        timestamp: new Date().toISOString(),
    };

    return jsonResponse(body, 200);
}

async function handleMetadata(request: Request, context: ServerContext) {
    if (request.method !== "GET") {
        return methodNotAllowed(request, ["GET", "OPTIONS"]);
    }

    return jsonResponse(getCoreMetadata(), 200);
}

function launchComputeWorker(
    jobId: string,
    rawBody: unknown,
    context: ServerContext,
) {
    if (context.workerHandle.activeWorker !== null) {
        context.workerHandle.activeWorker.terminate();
        context.workerHandle.activeWorker = null;
    }

    const worker = new Worker(new URL("../job/computeWorker.ts", import.meta.url));
    context.workerHandle.activeWorker = worker;
    context.jobs.markRunning(jobId);

    const request: ComputeWorkerRequest = { jobId, payload: rawBody };
    worker.postMessage(request);

    worker.onmessage = (event: MessageEvent<ComputeWorkerResponse>) => {
        context.workerHandle.activeWorker = null;
        const response = event.data;
        if (response.type === "completed") {
            context.jobs.markCompleted(response.jobId, response.result);
        } else {
            context.jobs.markFailed(response.jobId, response.error);
        }
    };

    worker.onerror = (err) => {
        context.workerHandle.activeWorker = null;
        context.jobs.markFailed(jobId, {
            code: "internal_error",
            message: err.message ?? "Worker encountered an unexpected error.",
        });
    };
}

async function handleComputeSubmission(request: Request, context: ServerContext) {
    if (request.method !== "POST") {
        return methodNotAllowed(request, ["POST", "OPTIONS"]);
    }

    const parsedBody = await parseRequestJsonBody(request);
    if (!parsedBody.ok) {
        return jsonResponse(parsedBody.errorBody, 400);
    }

    console.log("[compute] Incoming request:", JSON.stringify(parsedBody.value, null, 2));

    const queuedJob = context.jobs.createQueued({
        algorithmName: getQueuedAlgorithmName(parsedBody.value),
        requestId: getQueuedRequestId(parsedBody.value),
    });

    launchComputeWorker(queuedJob.jobId, parsedBody.value, context);

    const origin = new URL(request.url).origin;
    const responseBody: ComputeAcceptedResponse = {
        jobId: queuedJob.jobId,
        status: "queued",
        createdAt: queuedJob.createdAt,
        pollUrl: `${origin}/compute/${queuedJob.jobId}`,
    };

    return jsonResponse(responseBody, 202);
}

async function handleComputeStatus(request: Request, context: ServerContext, jobId: string) {
    if (request.method !== "GET") {
        return methodNotAllowed(request, ["GET", "OPTIONS"]);
    }

    const job = context.jobs.get(jobId);
    if (!job) {
        const body: ErrorResponse = {
            error: {
                code: "job_not_found",
                message: `No compute job found for id ${jobId}.`,
            },
        };

        return jsonResponse(body, 404);
    }

    return jsonResponse(job satisfies ComputeJobState, 200);
}

async function handleRoot(request: Request, context: ServerContext) {
    if (request.method !== "GET") {
        return methodNotAllowed(request, ["GET", "OPTIONS"]);
    }

    return jsonResponse({
        service: context.config.providerName,
        endpoints: {
            health: "/health",
            metadata: "/metadata",
            compute: "/compute",
            computeStatus: "/compute/:jobId",
        },
        algorithms: getCoreMetadata().algorithms.map((algorithm) => ({
            name: algorithm.name,
        })),
    });
}

export async function routeRequest(request: Request, context: ServerContext) {
    if (request.method === "OPTIONS") {
        return emptyResponse(204);
    }

    const url = new URL(request.url);
    if (url.pathname === "/") {
        return handleRoot(request, context);
    }

    if (url.pathname === "/health") {
        return handleHealth(request, context);
    }

    if (url.pathname === "/metadata") {
        return handleMetadata(request, context);
    }

    if (url.pathname === "/compute") {
        return handleComputeSubmission(request, context);
    }

    const jobMatch = /^\/compute\/([^/]+)$/.exec(url.pathname);
    const jobId = jobMatch?.[1];
    if (jobId) {
        return handleComputeStatus(request, context, decodeURIComponent(jobId));
    }

    return notFound();
}
