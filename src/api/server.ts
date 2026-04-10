import { fileURLToPath } from "url";
import type { ServerContext } from "../types/apiTypes";
import { getCoreMetadata } from "./coreBridge";
import { isRecord, parseRequestJsonBody } from "./jsonUtil";
import type {
    ComputeAcceptedResponse,
    ComputeJobState,
    ComputeProcessRequest,
    ComputeProcessResponse,
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

async function launchComputeProcess(
    jobId: string,
    rawBody: unknown,
    context: ServerContext,
) {
    if (context.processHandle.activeProcess !== null) {
        context.processHandle.activeProcess.kill();
        context.processHandle.activeProcess = null;
    }

    const processScriptPath = fileURLToPath(new URL("../job/computeProcess.ts", import.meta.url));
    console.log("[compute] Spawning process:", processScriptPath);
    const proc = Bun.spawn(["bun", processScriptPath], {
        stdin: "pipe",
        stdout: "pipe",
        stderr: "pipe",
    });

    context.processHandle.activeProcess = proc;
    context.jobs.markRunning(jobId);

    const request: ComputeProcessRequest = { jobId, payload: rawBody };
    const stdinPayload = JSON.stringify(request) + "\n";
    console.log("[compute] Writing to child stdin:", stdinPayload.length, "bytes");
    proc.stdin.write(stdinPayload);
    await proc.stdin.end();
    console.log("[compute] stdin closed");

    void new Response(proc.stderr).text().then((stderrText) => {
        if (stderrText.trim()) {
            console.error("[compute] Process stderr:", stderrText.trim());
        }
    });

    let responded = false;
    const [stdoutText, exitCode] = await Promise.all([
        new Response(proc.stdout).text(),
        proc.exited,
    ]);

    console.log("[compute] Process exited with code:", exitCode);
    console.log("[compute] Raw stdout (", stdoutText.length, "bytes):", JSON.stringify(stdoutText.slice(0, 500)));

    context.processHandle.activeProcess = null;

    const lines = stdoutText.split("\n").map((l) => l.trim()).filter(Boolean);
    let jsonLine: string | undefined;
    for (let i = lines.length - 1; i >= 0; i--) {
        const l = lines[i];
        if (l !== undefined && l.startsWith("{")) {
            jsonLine = l;
            break;
        }
    }

    if (jsonLine) {
        try {
            const response = JSON.parse(jsonLine) as ComputeProcessResponse;
            responded = true;
            if (response.type === "completed") {
                context.jobs.markCompleted(response.jobId, response.result);
            } else {
                context.jobs.markFailed(response.jobId, response.error);
            }
        } catch {
            // JSON line found but failed to parse — fall through to crash handler
        }
    }

    if (!responded) {
        context.jobs.markFailed(jobId, {
            code: "process_crash",
            message: `Compute process exited with code ${exitCode} without returning a result.`,
        });
    }
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

    void launchComputeProcess(queuedJob.jobId, parsedBody.value, context);

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
