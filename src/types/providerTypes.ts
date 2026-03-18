/**
 * Provider-defined metadata section label.
 * `General` is reserved as the consumer-side fallback when the provider leaves the section undefined.
 */
export type MetadataParamSection =
    | "General"
    | (string & {});

export type AlgoParamType =
    | "Integer"
    | "Decimal"
    | "Boolean"
    | "String"
    | "Enum";

export interface MetadataParamResponse {
    section?: MetadataParamSection;
    name: string;
    paramType: AlgoParamType;
    enumValues?: string[];
    defaultValue?: string;
    appHandler?: string;
}

export interface MetadataAlgorithmResponse {
    id: number;
    name: string;
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
