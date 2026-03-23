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
    enumValues: string[];
    defaultValue?: string;
    section?: MetadataParamSection;
    appHandler: string | null;
}

export interface LayerStyleAttribute {
    id: number;
    name: string;
    value: string | null;
}

export type LayerType = "Point" | "Line" | "Polygon";

export interface LayerLabelEnumValue {
    value: string;
    color: string | null;
}

export interface LayerLabel {
    key: string;
    enumValues: LayerLabelEnumValue[];
}

export interface MetadataCppLayerResponse {
    id: number;
    cppLayer: string;
    name: string;
    type: LayerType;
    style: LayerStyleAttribute[];
    label: LayerLabel | null;
}

export interface MetadataDebugLayerResponse {
    id: number;
    debugLayer: string;
    name: string;
    type: LayerType;
    style: LayerStyleAttribute[];
    label: LayerLabel | null;
}

export type MetadataLayerResponse = MetadataCppLayerResponse | MetadataDebugLayerResponse;

export interface MetadataAlgorithmResponse {
    id: number;
    name: string;
    parameters: MetadataParamResponse[];
    layers: MetadataLayerResponse[];
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
