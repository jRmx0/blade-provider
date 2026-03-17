import { cc, CString, type Pointer } from "bun:ffi";

import type { ComputeResult, MetadataResponse } from "../types/providerTypes";

const coreSymbols = cc({
    source: "./src/core/dispatcher.c",
    include: ["./dependencies/tcc-headers"],
    flags: ["-w"],
    symbols: {
        dispatch_metadata_json: {
            args: [],
            returns: "ptr",
        },
        dispatch_compute_json: {
            args: ["cstring"],
            returns: "ptr",
        },
        dispatch_string_free: {
            args: ["ptr"],
            returns: "void",
        },
    } as const,
}).symbols;

let metadataCache: MetadataResponse | null = null;

interface CoreErrorPayload {
    status: "error";
    code?: string;
    message?: string;
}

export class CoreComputeError extends Error {
    constructor(
        readonly code: string,
        message: string,
    ) {
        super(message);
        this.name = "CoreComputeError";
    }
}

function isRecord(value: unknown): value is Record<string, unknown> {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}

function isCoreErrorPayload(value: unknown): value is CoreErrorPayload {
    return isRecord(value) && value.status === "error";
}

function readCoreJson(pointer: Pointer | null, nullMessage: string): string {
    if (!pointer) {
        throw new Error(nullMessage);
    }

    const json = new CString(pointer).toString();
    coreSymbols.dispatch_string_free(pointer);
    return json;
}

function parseCoreJson<T>(pointer: Pointer | null, nullMessage: string): T {
    const json = readCoreJson(pointer, nullMessage);

    try {
        return JSON.parse(json) as T;
    } catch (error) {
        throw new Error(`Failed to parse core JSON payload: ${error instanceof Error ? error.message : String(error)}`);
    }
}

function loadMetadataFromCore(): MetadataResponse {
    return parseCoreJson<MetadataResponse>(
        coreSymbols.dispatch_metadata_json(),
        "Provider core metadata returned a null pointer.",
    );
}

function getCachedMetadata(): MetadataResponse {
    if (metadataCache === null) {
        metadataCache = loadMetadataFromCore();
    }

    return metadataCache;
}

export function warmCoreMetadata(): void {
    void getCachedMetadata();
}

export function getCoreMetadata(): MetadataResponse {
    return getCachedMetadata();
}

export function executeCoreCompute(requestPayload: unknown): ComputeResult {
    const requestJsonCString = Buffer.from(`${JSON.stringify(requestPayload)}\0`, "utf8");
    const payload = parseCoreJson<ComputeResult | CoreErrorPayload>(
        coreSymbols.dispatch_compute_json(requestJsonCString),
        "Provider core compute returned a null pointer.",
    );

    if (isCoreErrorPayload(payload)) {
        const code = typeof payload.code === "string" && payload.code.trim() !== ""
            ? payload.code.trim()
            : "core_compute_error";
        const message = typeof payload.message === "string" && payload.message.trim() !== ""
            ? payload.message
            : "Provider core returned an error.";

        throw new CoreComputeError(code, message);
    }

    return payload as ComputeResult;
}