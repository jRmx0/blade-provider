import { CString, dlopen, suffix, type Pointer } from "bun:ffi";
import { existsSync, readFileSync, readdirSync } from "node:fs";
import { join } from "node:path";

import type { MetadataAlgorithmResponse, MetadataParamResponse, MetadataResponse } from "../../temp/domain/providerTypes";

interface NativeProviderSymbols {
    dispatch_metadata_json(): Pointer;
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

function parseParameter(parameter: unknown, index: number): MetadataParamResponse {
    if (!isRecord(parameter)) {
        throw new Error(`Expected algorithms[0].parameters[${index}] to be an object.`);
    }

    const enumValues = parameter.enumValues;
    if (enumValues !== undefined && (!Array.isArray(enumValues) || enumValues.some((value) => typeof value !== "string"))) {
        throw new Error(`Expected algorithms[0].parameters[${index}].enumValues to be a string array.`);
    }

    const defaultValue = parameter.defaultValue;
    if (defaultValue !== undefined && typeof defaultValue !== "string") {
        throw new Error(`Expected algorithms[0].parameters[${index}].defaultValue to be a string.`);
    }

    return {
        name: expectNonEmptyString(parameter.name, `algorithms[0].parameters[${index}].name`),
        label: expectNonEmptyString(parameter.label, `algorithms[0].parameters[${index}].label`),
        paramType: expectNonEmptyString(parameter.paramType, `algorithms[0].parameters[${index}].paramType`) as MetadataParamResponse["paramType"],
        enumValues: enumValues ? [...enumValues] : undefined,
        defaultValue,
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
        label: expectNonEmptyString(algorithm.label, `algorithms[${index}].label`),
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
        dispatch_string_free: {
            args: ["ptr"],
            returns: "void",
        },
    }) as NativeProviderLibrary;

    return nativeLibrary;
}

function loadMetadataFromNative(): MetadataResponse {
    const library = getNativeLibrary();
    const pointer = library.symbols.dispatch_metadata_json();

    if (!pointer) {
        throw new Error("Native provider metadata bridge returned a null pointer.");
    }

    const json = new CString(pointer).toString();
    library.symbols.dispatch_string_free(pointer);

    let parsed: unknown;

    try {
        parsed = JSON.parse(json);
    } catch (error) {
        throw new Error(`Failed to parse native metadata JSON: ${error instanceof Error ? error.message : String(error)}`);
    }

    return parseMetadataResponse(parsed);
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