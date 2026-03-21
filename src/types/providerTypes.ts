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
    id: number;
    name: string;
    paramType: AlgoParamType;
    enumValues?: string[];
    defaultValue?: string;
    section?: MetadataParamSection;
    appHandler?: string;
}

export interface DebugLayerStyleAttribute {
    id: number;
    name: string;
    value: string | null;
}

export type DebugLayerType = "Point" | "Line" | "Polygon";

export interface DebugLayerLabelEnumValue {
    value: string;
    color: string | null;
}

export interface DebugLayerLabel {
    key: string;
    enumValues: DebugLayerLabelEnumValue[];
}

export interface MetadataDebugLayerResponse {
    id: number;
    zIndex: number;
    key: string;
    name: string;
    type: DebugLayerType;
    style: DebugLayerStyleAttribute[];
    label: DebugLayerLabel | null;
}

export interface MetadataAlgorithmResponse {
    id: number;
    name: string;
    parameters: MetadataParamResponse[];
    debugLayers?: MetadataDebugLayerResponse[];
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
    result?: Record<string, unknown>;
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
