import type { ErrorResponse } from "../types/providerTypes";

export type ParsedRequestJson =
    | { ok: true; value: unknown }
    | { ok: false; errorBody: ErrorResponse };

export function isRecord(value: unknown): value is Record<string, unknown> {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}

export function parseJsonString<T>(json: string, sourceLabel: string): T {
    try {
        return JSON.parse(json) as T;
    } catch (error) {
        throw new Error(`Failed to parse ${sourceLabel} JSON payload: ${error instanceof Error ? error.message : String(error)}`);
    }
}

export async function parseRequestJsonBody(request: Request): Promise<ParsedRequestJson> {
    try {
        return { ok: true, value: await request.json() };
    } catch {
        return {
            ok: false,
            errorBody: {
                error: {
                    code: "invalid_json",
                    message: "Request body must contain valid JSON.",
                },
            },
        };
    }
}