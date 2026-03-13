import { getProviderConfig, type ProviderConfig } from "../config/env";
import { algorithmCatalog } from "../domain/catalog";
import { InMemoryJobStore } from "../domain/jobStore";
import type {
    ComputeAcceptedResponse,
    ComputeJobState,
    ErrorResponse,
    HealthResponse,
} from "../domain/providerTypes";
import { validateComputeRequest } from "../domain/validateComputeRequest";
import type { ComputeEngine } from "../runtime/ComputeEngine";
import { MockComputeEngine } from "../runtime/MockComputeEngine";

interface ServerContext {
    config: ProviderConfig;
    engine: ComputeEngine;
    jobs: InMemoryJobStore;
}

function jsonResponse(request: Request, config: ProviderConfig, body: unknown, status = 200) {
    const headers = buildCorsHeaders(request, config);
    headers.set("Content-Type", "application/json");

    return new Response(JSON.stringify(body, null, 2), {
        status,
        headers,
    });
}

function emptyResponse(request: Request, config: ProviderConfig, status = 204) {
    return new Response(null, {
        status,
        headers: buildCorsHeaders(request, config),
    });
}

function buildCorsHeaders(_request: Request, _config: ProviderConfig): Headers {
    const headers = new Headers();
    headers.set("Access-Control-Allow-Origin", "*");
    headers.set("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    headers.set("Access-Control-Allow-Headers", "Content-Type, Authorization");
    headers.set("Access-Control-Max-Age", "86400");

    return headers;
}

function notFound(request: Request, config: ProviderConfig) {
    const body: ErrorResponse = {
        error: {
            code: "not_found",
            message: "Route not found.",
        },
    };

    return jsonResponse(request, config, body, 404);
}

function methodNotAllowed(request: Request, config: ProviderConfig, allowedMethods: string[]) {
    const body: ErrorResponse = {
        error: {
            code: "method_not_allowed",
            message: `Method ${request.method} is not allowed for this route.`,
            details: { allowedMethods },
        },
    };

    return jsonResponse(request, config, body, 405);
}

async function parseJsonBody(request: Request, config: ProviderConfig) {
    try {
        return { ok: true as const, value: await request.json() };
    } catch {
        const body: ErrorResponse = {
            error: {
                code: "invalid_json",
                message: "Request body must contain valid JSON.",
            },
        };

        return { ok: false as const, response: jsonResponse(request, config, body, 400) };
    }
}

async function handleHealth(request: Request, context: ServerContext) {
    if (request.method !== "GET") {
        return methodNotAllowed(request, context.config, ["GET", "OPTIONS"]);
    }

    const body: HealthResponse = {
        status: "ok",
        service: context.config.providerName,
        timestamp: new Date().toISOString(),
    };

    return jsonResponse(request, context.config, body, 200);
}

async function handleMetadata(request: Request, context: ServerContext) {
    if (request.method !== "GET") {
        return methodNotAllowed(request, context.config, ["GET", "OPTIONS"]);
    }

    return jsonResponse(request, context.config, algorithmCatalog.metadata(), 200);
}

async function runComputeJob(
    jobId: string,
    rawBody: unknown,
    context: ServerContext,
) {
    context.jobs.markRunning(jobId);

    try {
        const validation = validateComputeRequest(rawBody);
        if (!validation.ok) {
            context.jobs.markFailed(jobId, {
                code: validation.code,
                message: validation.message,
            });
            return;
        }

        const result = await context.engine.execute(validation.value);
        context.jobs.markCompleted(jobId, result);
    } catch (error) {
        context.jobs.markFailed(jobId, {
            code: "compute_failed",
            message: error instanceof Error ? error.message : String(error),
        });
    }
}

async function handleComputeSubmission(request: Request, context: ServerContext) {
    if (request.method !== "POST") {
        return methodNotAllowed(request, context.config, ["POST", "OPTIONS"]);
    }

    const parsedBody = await parseJsonBody(request, context.config);
    if (!parsedBody.ok) {
        return parsedBody.response;
    }

    const validation = validateComputeRequest(parsedBody.value);
    if (!validation.ok) {
        const body: ErrorResponse = {
            error: {
                code: validation.code,
                message: validation.message,
                details: validation.details,
            },
        };

        return jsonResponse(request, context.config, body, validation.status);
    }

    const queuedJob = context.jobs.createQueued({
        algorithmName: validation.value.algorithm.name,
        requestId: validation.value.requestId,
    });

    void runComputeJob(queuedJob.jobId, parsedBody.value, context);

    const origin = new URL(request.url).origin;
    const responseBody: ComputeAcceptedResponse = {
        jobId: queuedJob.jobId,
        status: "queued",
        createdAt: queuedJob.createdAt,
        pollUrl: `${origin}/compute/${queuedJob.jobId}`,
    };

    return jsonResponse(request, context.config, responseBody, 202);
}

async function handleComputeStatus(request: Request, context: ServerContext, jobId: string) {
    if (request.method !== "GET") {
        return methodNotAllowed(request, context.config, ["GET", "OPTIONS"]);
    }

    const job = context.jobs.get(jobId);
    if (!job) {
        const body: ErrorResponse = {
            error: {
                code: "job_not_found",
                message: `No compute job found for id ${jobId}.`,
            },
        };

        return jsonResponse(request, context.config, body, 404);
    }

    return jsonResponse(request, context.config, job satisfies ComputeJobState, 200);
}

async function handleRoot(request: Request, context: ServerContext) {
    if (request.method !== "GET") {
        return methodNotAllowed(request, context.config, ["GET", "OPTIONS"]);
    }

    return jsonResponse(request, context.config, {
        service: context.config.providerName,
        endpoints: {
            health: "/health",
            metadata: "/metadata",
            compute: "/compute",
            computeStatus: "/compute/:jobId",
        },
        algorithms: algorithmCatalog.list().map((algorithm) => ({
            name: algorithm.name,
            label: algorithm.label,
        })),
    });
}

async function routeRequest(request: Request, context: ServerContext) {
    if (request.method === "OPTIONS") {
        return emptyResponse(request, context.config, 204);
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

    return notFound(request, context.config);
}

export function createProviderServer() {
    const context: ServerContext = {
        config: getProviderConfig(),
        engine: new MockComputeEngine(),
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
