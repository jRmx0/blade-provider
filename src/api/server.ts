import type { ServerContext } from "../types/apiTypes";
import { getProviderConfig } from "./envParser";
import {
    executeNativeProviderCompute,
    getNativeProviderMetadata,
    NativeComputeError,
} from "./nativeCore";
import { InMemoryJobStore } from "../job/jobStore";
import type {
    ComputeAcceptedResponse,
    ComputeJobState,
    ErrorResponse,
    HealthResponse,
} from "../types/providerTypes";

function isRecord(value: unknown): value is Record<string, unknown> {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}

function getQueuedAlgorithmName(rawBody: unknown): string {
    if (!isRecord(rawBody)) {
        return "Unknown algorithm";
    }

    return typeof rawBody.algorithmName === "string" && rawBody.algorithmName.trim() !== ""
        ? rawBody.algorithmName.trim()
        : "Unknown algorithm";
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

async function parseJsonBody(request: Request) {
    try {
        return { ok: true as const, value: await request.json() };
    } catch {
        const body: ErrorResponse = {
            error: {
                code: "invalid_json",
                message: "Request body must contain valid JSON.",
            },
        };

        return { ok: false as const, response: jsonResponse(body, 400) };
    }
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

    return jsonResponse(getNativeProviderMetadata(), 200);
}

async function runComputeJob(
    jobId: string,
    rawBody: unknown,
    context: ServerContext,
) {
    context.jobs.markRunning(jobId);

    try {
        const result = executeNativeProviderCompute(rawBody);
        context.jobs.markCompleted(jobId, result);
    } catch (error) {
        context.jobs.markFailed(jobId, {
            code: error instanceof NativeComputeError ? error.code : "compute_failed",
            message: error instanceof Error ? error.message : String(error),
        });
    }
}

async function handleComputeSubmission(request: Request, context: ServerContext) {
    if (request.method !== "POST") {
        return methodNotAllowed(request, ["POST", "OPTIONS"]);
    }

    const parsedBody = await parseJsonBody(request);
    if (!parsedBody.ok) {
        return parsedBody.response;
    }

    const queuedJob = context.jobs.createQueued({
        algorithmName: getQueuedAlgorithmName(parsedBody.value),
        requestId: getQueuedRequestId(parsedBody.value),
    });

    void runComputeJob(queuedJob.jobId, parsedBody.value, context);

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
        algorithms: getNativeProviderMetadata().algorithms.map((algorithm) => ({
            name: algorithm.name,
        })),
    });
}

async function routeRequest(request: Request, context: ServerContext) {
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

export function createProviderServer() {
    const context: ServerContext = {
        config: getProviderConfig(),
        jobs: new InMemoryJobStore(),
    };

    return Bun.serve({
        port: context.config.port,
        idleTimeout: 30,
        async fetch(request) {
            return routeRequest(request, context);
        },
        error(error) {
            console.error("Unhandled provider error", error);
            return new Response(JSON.stringify({
                error: {
                    code: "internal_error",
                    message: "Unhandled provider error.",
                },
            }, null, 2), {
                status: 500,
                headers: {
                    "Content-Type": "application/json",
                },
            });
        },
    });
}
