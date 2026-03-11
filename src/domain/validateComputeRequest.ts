import { algorithmCatalog } from "./catalog";
import type {
    AlgoParamType,
    ComputeEnvironmentInput,
    ComputeParameterValue,
    ComputeRequest,
    EnvironmentPolygonInput,
    Point,
    ResolvedComputeRequest,
} from "./providerTypes";

export type ComputeValidationResult =
    | { ok: true; value: ResolvedComputeRequest }
    | { ok: false; status: number; code: string; message: string; details?: unknown };

function isRecord(value: unknown): value is Record<string, unknown> {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}

function isFiniteNumber(value: unknown): value is number {
    return typeof value === "number" && Number.isFinite(value);
}

function parsePoint(value: unknown): Point | null {
    if (!isRecord(value) || !isFiniteNumber(value.x) || !isFiniteNumber(value.y)) {
        return null;
    }

    return { x: value.x, y: value.y };
}

function parsePolygonCollection(name: string, value: unknown): ComputeValidationResult | EnvironmentPolygonInput[] {
    if (!Array.isArray(value)) {
        return {
            ok: false,
            status: 400,
            code: `invalid_${name}`,
            message: `${name} must be an array of polygons.`,
        };
    }

    const polygons: EnvironmentPolygonInput[] = [];

    for (const [index, item] of value.entries()) {
        if (!isRecord(item)) {
            return {
                ok: false,
                status: 400,
                code: `invalid_${name}`,
                message: `${name}[${index}] must be an object.`,
            };
        }

        if (!Array.isArray(item.vertices)) {
            return {
                ok: false,
                status: 400,
                code: `invalid_${name}`,
                message: `${name}[${index}].vertices must be an array of points.`,
            };
        }

        const vertices = item.vertices.map((vertex) => parsePoint(vertex));
        if (vertices.some((vertex) => vertex === null)) {
            return {
                ok: false,
                status: 400,
                code: `invalid_${name}`,
                message: `${name}[${index}] contains an invalid point.`,
            };
        }

        if (vertices.length < 3) {
            return {
                ok: false,
                status: 400,
                code: `invalid_${name}`,
                message: `${name}[${index}] must contain at least 3 vertices.`,
            };
        }

        polygons.push({
            id: typeof item.id === "string" ? item.id : undefined,
            label: typeof item.label === "string" ? item.label : undefined,
            vertices: vertices.filter((vertex): vertex is Point => vertex !== null),
        });
    }

    return polygons;
}

function parseParameterValue(
    paramType: AlgoParamType,
    rawValue: unknown,
    enumValues: string[] | undefined,
    parameterName: string,
): ComputeValidationResult | ComputeParameterValue {
    switch (paramType) {
        case "integer": {
            const parsedValue = typeof rawValue === "number"
                ? rawValue
                : typeof rawValue === "string"
                    ? Number(rawValue)
                    : NaN;

            if (!Number.isInteger(parsedValue)) {
                return {
                    ok: false,
                    status: 400,
                    code: "invalid_parameter",
                    message: `Parameter ${parameterName} must be an integer.`,
                };
            }

            return parsedValue;
        }
        case "decimal": {
            const parsedValue = typeof rawValue === "number"
                ? rawValue
                : typeof rawValue === "string"
                    ? Number(rawValue)
                    : NaN;

            if (!Number.isFinite(parsedValue)) {
                return {
                    ok: false,
                    status: 400,
                    code: "invalid_parameter",
                    message: `Parameter ${parameterName} must be a decimal number.`,
                };
            }

            return parsedValue;
        }
        case "boolean": {
            if (typeof rawValue === "boolean") {
                return rawValue;
            }

            if (rawValue === "true") return true;
            if (rawValue === "false") return false;

            return {
                ok: false,
                status: 400,
                code: "invalid_parameter",
                message: `Parameter ${parameterName} must be a boolean.`,
            };
        }
        case "string":
        case "enum":
        case "format":
        case "type":
        case "coordsystem": {
            if (typeof rawValue !== "string" || rawValue.trim() === "") {
                return {
                    ok: false,
                    status: 400,
                    code: "invalid_parameter",
                    message: `Parameter ${parameterName} must be a non-empty string.`,
                };
            }

            if (enumValues && enumValues.length > 0 && !enumValues.includes(rawValue)) {
                return {
                    ok: false,
                    status: 400,
                    code: "invalid_parameter",
                    message: `Parameter ${parameterName} must be one of: ${enumValues.join(", ")}.`,
                };
            }

            return rawValue;
        }
        default:
            return {
                ok: false,
                status: 400,
                code: "invalid_parameter",
                message: `Parameter ${parameterName} uses an unsupported type.`,
            };
    }
}

export function validateComputeRequest(rawValue: unknown): ComputeValidationResult {
    if (!isRecord(rawValue)) {
        return {
            ok: false,
            status: 400,
            code: "invalid_request",
            message: "Request body must be a JSON object.",
        };
    }

    const algorithmName = rawValue.algorithmName;
    if (typeof algorithmName !== "string" || algorithmName.trim() === "") {
        return {
            ok: false,
            status: 400,
            code: "missing_algorithm",
            message: "algorithmName is required.",
        };
    }

    const algorithm = algorithmCatalog.get(algorithmName);
    if (!algorithm) {
        return {
            ok: false,
            status: 404,
            code: "unknown_algorithm",
            message: `Unknown algorithm: ${algorithmName}`,
        };
    }

    const environment = rawValue.environment;
    if (!isRecord(environment)) {
        return {
            ok: false,
            status: 400,
            code: "missing_environment",
            message: "environment is required.",
        };
    }

    const zones = parsePolygonCollection("zones", environment.zones);
    if (!Array.isArray(zones)) {
        return zones;
    }

    if (zones.length === 0) {
        return {
            ok: false,
            status: 400,
            code: "missing_zones",
            message: "environment.zones must contain at least one polygon.",
        };
    }

    const obstacles = parsePolygonCollection("obstacles", environment.obstacles ?? []);
    if (!Array.isArray(obstacles)) {
        return obstacles;
    }

    const providedParameters = isRecord(rawValue.parameters) ? rawValue.parameters : {};
    const knownParameterNames = new Set(algorithm.parameters.map((parameter) => parameter.name));
    const unknownParameters = Object.keys(providedParameters).filter((parameterName) => !knownParameterNames.has(parameterName));
    if (unknownParameters.length > 0) {
        return {
            ok: false,
            status: 400,
            code: "unknown_parameter",
            message: `Unknown parameter(s): ${unknownParameters.join(", ")}`,
        };
    }

    const normalizedParameters: Record<string, ComputeParameterValue> = {};

    for (const parameter of algorithm.parameters) {
        const rawParameterValue = providedParameters[parameter.name] ?? parameter.defaultValue;
        if (rawParameterValue === undefined) {
            return {
                ok: false,
                status: 400,
                code: "missing_parameter",
                message: `Parameter ${parameter.name} is required.`,
            };
        }

        const parsedParameterValue = parseParameterValue(
            parameter.paramType,
            rawParameterValue,
            parameter.enumValues,
            parameter.name,
        );

        if (typeof parsedParameterValue === "object" && parsedParameterValue !== null && "ok" in parsedParameterValue) {
            return parsedParameterValue;
        }

        normalizedParameters[parameter.name] = parsedParameterValue;
    }

    return {
        ok: true,
        value: {
            algorithm,
            parameters: normalizedParameters,
            environment: {
                id: typeof environment.id === "string" ? environment.id : undefined,
                name: typeof environment.name === "string" ? environment.name : undefined,
                format: environment.format === "grid" ? "grid" : "polygon",
                type: environment.type === "unknown" ? "unknown" : "known",
                zones,
                obstacles,
            },
            requestId: typeof rawValue.requestId === "string" ? rawValue.requestId : undefined,
        },
    };
}
