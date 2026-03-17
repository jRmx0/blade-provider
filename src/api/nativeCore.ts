import { cc, CString, type Pointer } from "bun:ffi";

import type { ComputeResult, MetadataResponse } from "../types/providerTypes";

const nativeSymbols = cc({
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

interface NativeErrorPayload {
    status: "error";
    code?: string;
    message?: string;
}

export class NativeComputeError extends Error {
    constructor(
        readonly code: string,
        message: string,
    ) {
        super(message);
        this.name = "NativeComputeError";
    }
}

function isRecord(value: unknown): value is Record<string, unknown> {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}

function isNativeErrorPayload(value: unknown): value is NativeErrorPayload {
    return isRecord(value) && value.status === "error";
}

function readNativeJson(pointer: Pointer | null, nullMessage: string): string {
    if (!pointer) {
        throw new Error(nullMessage);
    }

    const json = new CString(pointer).toString();
    nativeSymbols.dispatch_string_free(pointer);
    return json;
}

function parseNativeJson<T>(pointer: Pointer | null, nullMessage: string): T {
    const json = readNativeJson(pointer, nullMessage);

    try {
        return JSON.parse(json) as T;
    } catch (error) {
        throw new Error(`Failed to parse native JSON payload: ${error instanceof Error ? error.message : String(error)}`);
    }
}

function loadMetadataFromNative(): MetadataResponse {
    return parseNativeJson<MetadataResponse>(
        nativeSymbols.dispatch_metadata_json(),
        "Native provider metadata returned a null pointer.",
    );
}

function getCachedMetadata(): MetadataResponse {
    if (metadataCache === null) {
        metadataCache = loadMetadataFromNative();
    }

    return metadataCache;
}

export function warmNativeProviderMetadata(): void {
    void getCachedMetadata();
}

export function getNativeProviderMetadata(): MetadataResponse {
    return getCachedMetadata();
}

export function executeNativeProviderCompute(requestPayload: unknown): ComputeResult {
    const requestJsonCString = Buffer.from(`${JSON.stringify(requestPayload)}\0`, "utf8");
    const payload = parseNativeJson<ComputeResult | NativeErrorPayload>(
        nativeSymbols.dispatch_compute_json(requestJsonCString),
        "Native provider compute returned a null pointer.",
    );

    if (isNativeErrorPayload(payload)) {
        const code = typeof payload.code === "string" && payload.code.trim() !== ""
            ? payload.code.trim()
            : "native_compute_error";
        const message = typeof payload.message === "string" && payload.message.trim() !== ""
            ? payload.message
            : "Native compute returned an error.";

        throw new NativeComputeError(code, message);
    }

    return payload as ComputeResult;
}