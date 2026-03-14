export type MetadataParamSection =
    | "General"
    | "Coverage path"
    | "Environment"
    | "Object"
    | "Execution";

export type AlgoParamType =
    | "integer"
    | "decimal"
    | "boolean"
    | "string"
    | "enum"
    | "format"
    | "type"
    | "coordsystem";

export interface MetadataParamResponse {
    section?: MetadataParamSection;
    name: string;
    label: string;
    paramType: AlgoParamType;
    enumValues?: string[];
    defaultValue?: string;
}

export interface MetadataAlgorithmResponse {
    name: string;
    label: string;
    parameters: MetadataParamResponse[];
}

export interface MetadataResponse {
    algorithms: MetadataAlgorithmResponse[];
}

export interface HealthResponse {
    status: "ok";
    service: string;
    timestamp: string;
}

export interface Point {
    x: number;
    y: number;
}

export interface EnvironmentPolygonInput {
    id?: string;
    label?: string;
    vertices: Point[];
}

export interface ComputeEnvironmentInput {
    id?: string;
    name?: string;
    format?: "polygon" | "grid";
    type?: "known" | "unknown";
    zones: EnvironmentPolygonInput[];
    obstacles: EnvironmentPolygonInput[];
}

export type ComputeParameterValue = string | number | boolean;

export interface ComputeRequest {
    algorithmName: string;
    parameters?: Record<string, unknown>;
    environment: ComputeEnvironmentInput;
    requestId?: string;
}

export interface ResolvedComputeRequest {
    algorithm: MetadataAlgorithmResponse;
    parameters: Record<string, ComputeParameterValue>;
    environment: ComputeEnvironmentInput;
    requestId?: string;
}

export type ComputeJobStatus = "queued" | "running" | "completed" | "failed";

export interface ComputeResult {
    zoneCoverage: number;
    routeOverlap: number;
    turnCount: number;
    route: Point[];
    intermediateCalculations: Record<string, unknown>;
}

export interface ComputeJobError {
    code: string;
    message: string;
}

export interface ComputeJobState {
    jobId: string;
    status: ComputeJobStatus;
    algorithmName: string;
    createdAt: string;
    startedAt?: string;
    completedAt?: string;
    requestId?: string;
    error?: ComputeJobError;
    result?: ComputeResult;
}

export interface ComputeAcceptedResponse {
    jobId: string;
    status: "queued" | "running";
    createdAt: string;
    pollUrl: string;
}

export interface ErrorResponse {
    error: {
        code: string;
        message: string;
        details?: unknown;
    };
}
