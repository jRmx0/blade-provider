import { CString, dlopen, suffix, type Pointer } from "bun:ffi";
import { existsSync, readFileSync, readdirSync } from "node:fs";
import { join } from "node:path";

import type {
    ComputeResult,
    MetadataAlgorithmResponse,
    MetadataParamResponse,
    MetadataResponse,
    Point,
} from "../../types/providerTypes";

interface NativeProviderSymbols {
    dispatch_metadata_json(): Pointer;
    dispatch_compute_json(requestJson: Uint8Array): Pointer;
    dispatch_string_free(pointer: Pointer): void;
}

interface NativeProviderLibrary {
    symbols: NativeProviderSymbols;
}

let nativeLibrary: NativeProviderLibrary | null = null;
let metadataCache: MetadataResponse | null = null;

interface NativeLibraryManifest {
    fileName: string;
    builtAt?: string;
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

function cloneParameter(parameter: MetadataParamResponse): MetadataParamResponse {
    return {
        ...parameter,
        enumValues: parameter.enumValues ? [...parameter.enumValues] : undefined,
    };
}

function cloneAlgorithm(algorithm: MetadataAlgorithmResponse): MetadataAlgorithmResponse {
    return {
        ...algorithm,
        parameters: algorithm.parameters.map(cloneParameter),
    };
}

function cloneMetadata(metadata: MetadataResponse): MetadataResponse {
    return {
        algorithms: metadata.algorithms.map(cloneAlgorithm),
    };
}

function isRecord(value: unknown): value is Record<string, unknown> {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}

function expectNonEmptyString(value: unknown, path: string): string {
    if (typeof value !== "string" || value.trim() === "") {
        throw new Error(`Expected ${path} to be a non-empty string.`);
    }

    return value;
}

function expectFiniteNumber(value: unknown, path: string): number {
    if (typeof value !== "number" || !Number.isFinite(value)) {
        throw new Error(`Expected ${path} to be a finite number.`);
    }

    return value;
}

function parseParamType(value: unknown, path: string): MetadataParamResponse["paramType"] {
    const rawValue = expectNonEmptyString(value, path).trim();

    switch (rawValue) {
        case "Integer":
        case "Decimal":
        case "Boolean":
        case "String":
        case "Enum":
            return rawValue;
        default:
            throw new Error(`Expected ${path} to be a supported param type.`);
    }
}

function parseAppHandler(value: unknown, path: string): MetadataParamResponse["appHandler"] {
    if (value === undefined) {
        return undefined;
    }

    return expectNonEmptyString(value, path).trim();
}

function parseParameter(parameter: unknown, index: number): MetadataParamResponse {
    if (!isRecord(parameter)) {
        throw new Error(`Expected algorithms[0].parameters[${index}] to be an object.`);
    }

    const section = parameter.section;
    if (section !== undefined && (typeof section !== "string" || section.trim() === "")) {
        throw new Error(`Expected algorithms[0].parameters[${index}].section to be a non-empty string when provided.`);
    }

    const enumValues = parameter.enumValues;
    if (enumValues !== undefined && (!Array.isArray(enumValues) || enumValues.some((value) => typeof value !== "string"))) {
        throw new Error(`Expected algorithms[0].parameters[${index}].enumValues to be a string array.`);
    }

    const defaultValue = parameter.defaultValue;
    if (defaultValue !== undefined && typeof defaultValue !== "string") {
        throw new Error(`Expected algorithms[0].parameters[${index}].defaultValue to be a string.`);
    }

    const paramType = parseParamType(parameter.paramType, `algorithms[0].parameters[${index}].paramType`);
    const appHandler = parseAppHandler(parameter.appHandler, `algorithms[0].parameters[${index}].appHandler`);

    return {
        section: typeof section === "string" ? section as MetadataParamResponse["section"] : undefined,
        name: expectNonEmptyString(parameter.name, `algorithms[0].parameters[${index}].name`),
        paramType,
        enumValues: enumValues ? [...enumValues] : undefined,
        defaultValue,
        appHandler,
    };
}

function parseAlgorithm(algorithm: unknown, index: number): MetadataAlgorithmResponse {
    if (!isRecord(algorithm)) {
        throw new Error(`Expected algorithms[${index}] to be an object.`);
    }

    if (!Array.isArray(algorithm.parameters)) {
        throw new Error(`Expected algorithms[${index}].parameters to be an array.`);
    }

    return {
        name: expectNonEmptyString(algorithm.name, `algorithms[${index}].name`),
        parameters: algorithm.parameters.map((parameter, parameterIndex) => parseParameter(parameter, parameterIndex)),
    };
}

function parseMetadataResponse(value: unknown): MetadataResponse {
    if (!isRecord(value)) {
        throw new Error("Expected native metadata payload to be an object.");
    }

    if (!Array.isArray(value.algorithms)) {
        throw new Error("Expected native metadata payload to contain an algorithms array.");
    }

    return {
        algorithms: value.algorithms.map((algorithm, index) => parseAlgorithm(algorithm, index)),
    };
}

function parsePoint(value: unknown, path: string): Point {
    if (!isRecord(value)) {
        throw new Error(`Expected ${path} to be an object.`);
    }

    return {
        x: expectFiniteNumber(value.x, `${path}.x`),
        y: expectFiniteNumber(value.y, `${path}.y`),
    };
}

function parseIntermediateCalculations(value: unknown, path: string): Record<string, unknown> {
    if (!isRecord(value)) {
        throw new Error(`Expected ${path} to be an object.`);
    }

    return structuredClone(value) as Record<string, unknown>;
}

function parseComputeResult(value: unknown): ComputeResult {
    if (!isRecord(value)) {
        throw new Error("Expected native compute payload to be an object.");
    }

    const status = expectNonEmptyString(value.status, "native compute payload.status").trim().toLowerCase();
    if (status === "error") {
        const code = typeof value.code === "string" && value.code.trim() !== ""
            ? value.code.trim()
            : "native_compute_error";
        const message = typeof value.message === "string" && value.message.trim() !== ""
            ? value.message
            : "Native compute returned an error.";

        throw new NativeComputeError(code, message);
    }

    if (status !== "ok") {
        throw new Error('Expected native compute payload.status to be "ok" or "error".');
    }

    if (!Array.isArray(value.route)) {
        throw new Error("Expected native compute payload.route to be an array.");
    }

    return {
        zoneCoverage: expectFiniteNumber(value.zoneCoverage, "native compute payload.zoneCoverage"),
        routeOverlap: expectFiniteNumber(value.routeOverlap, "native compute payload.routeOverlap"),
        turnCount: expectFiniteNumber(value.turnCount, "native compute payload.turnCount"),
        route: value.route.map((point, index) => parsePoint(point, `native compute payload.route[${index}]`)),
        intermediateCalculations: parseIntermediateCalculations(
            value.intermediateCalculations,
            "native compute payload.intermediateCalculations",
        ),
    };
}

function isNativeLibraryManifest(value: unknown): value is NativeLibraryManifest {
    return isRecord(value) && typeof value.fileName === "string" && value.fileName.trim() !== "";
}

function getNativeOutputDirectory(): string {
    return join(process.cwd(), "out", "native");
}

function getManifestLibraryPath(): string | null {
    const manifestPath = join(getNativeOutputDirectory(), "provider_core.manifest.json");
    if (!existsSync(manifestPath)) {
        return null;
    }

    const manifest = JSON.parse(readFileSync(manifestPath, "utf8")) as unknown;
    if (!isNativeLibraryManifest(manifest)) {
        throw new Error(`Native provider manifest at ${manifestPath} is invalid.`);
    }

    return join(getNativeOutputDirectory(), manifest.fileName);
}

function getLatestVersionedLibraryPath(): string | null {
    const outDir = getNativeOutputDirectory();
    if (!existsSync(outDir)) {
        return null;
    }

    const candidates = readdirSync(outDir)
        .filter((entry) => /^provider_core\.\d+\./.test(entry) && entry.endsWith(`.${suffix}`))
        .sort((left, right) => right.localeCompare(left, undefined, { numeric: true }));

    const latestCandidate = candidates.at(0);
    return latestCandidate ? join(outDir, latestCandidate) : null;
}

function getLibraryPath(): string {
    const manifestPath = getManifestLibraryPath();
    if (manifestPath !== null) {
        return manifestPath;
    }

    const latestVersionedPath = getLatestVersionedLibraryPath();
    if (latestVersionedPath !== null) {
        return latestVersionedPath;
    }

    return join(getNativeOutputDirectory(), `provider_core.${suffix}`);
}

function getNativeLibrary(): NativeProviderLibrary {
    if (nativeLibrary !== null) {
        return nativeLibrary;
    }

    const libraryPath = getLibraryPath();
    if (!existsSync(libraryPath)) {
        throw new Error(`Native provider library was not found at ${libraryPath}. Run \"bun run build:native\" first.`);
    }

    nativeLibrary = dlopen(libraryPath, {
        dispatch_metadata_json: {
            args: [],
            returns: "ptr",
        },
        dispatch_compute_json: {
            args: ["cstring", "cstring"],
            returns: "ptr",
        },
        dispatch_string_free: {
            args: ["ptr"],
            returns: "void",
        },
    }) as unknown as NativeProviderLibrary;

    return nativeLibrary;
}

function readNativeJson(pointer: Pointer, nullMessage: string): unknown {
    if (!pointer) {
        throw new Error(nullMessage);
    }

    const library = getNativeLibrary();
    const json = new CString(pointer).toString();
    library.symbols.dispatch_string_free(pointer);

    try {
        return JSON.parse(json);
    } catch (error) {
        throw new Error(`Failed to parse native JSON payload: ${error instanceof Error ? error.message : String(error)}`);
    }
}

function loadMetadataFromNative(): MetadataResponse {
    const library = getNativeLibrary();
    return parseMetadataResponse(
        readNativeJson(
            library.symbols.dispatch_metadata_json(),
            "Native provider metadata bridge returned a null pointer.",
        ),
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
    return cloneMetadata(getCachedMetadata());
}

export function getNativeProviderAlgorithm(name: string): MetadataAlgorithmResponse | undefined {
    const algorithm = getCachedMetadata().algorithms.find((candidate) => candidate.name === name);
    return algorithm ? cloneAlgorithm(algorithm) : undefined;
}

export function executeNativeProviderCompute(requestPayload: unknown): ComputeResult {
    const library = getNativeLibrary();
    const requestJson = JSON.stringify(requestPayload);
    const requestJsonCString = Buffer.from(`${requestJson}\0`, "utf8");

    return parseComputeResult(
        readNativeJson(
            library.symbols.dispatch_compute_json(requestJsonCString),
            "Native provider compute bridge returned a null pointer.",
        ),
    );
}