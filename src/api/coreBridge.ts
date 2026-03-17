import { cc, CString, type Pointer } from "bun:ffi";

import type { ComputeResult, MetadataResponse } from "../types/providerTypes";
import { isRecord, parseJsonString } from "./jsonUtil";

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

function readCorePayload<T>(pointer: Pointer | null, nullMessage: string): T {
    if (!pointer) {
        throw new Error(nullMessage);
    }

    const json = new CString(pointer).toString();
    coreSymbols.dispatch_string_free(pointer);
    return parseJsonString<T>(json, "core");
}

export function getCoreMetadata(): MetadataResponse {
    return readCorePayload<MetadataResponse>(
        coreSymbols.dispatch_metadata_json(),
        "Provider core metadata returned a null pointer.",
    );
}

export function executeCoreCompute(requestPayload: unknown): ComputeResult {
    const requestJsonCString = Buffer.from(`${JSON.stringify(requestPayload)}\0`, "utf8");
    const payload = readCorePayload<ComputeResult | CoreErrorPayload>(
        coreSymbols.dispatch_compute_json(requestJsonCString),
        "Provider core compute returned a null pointer.",
    );

    if (isRecord(payload) && payload.status === "error") {
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