/**
 * Provider-defined metadata section label.
 * `General` is reserved as the consumer-side fallback when the provider leaves the section undefined.
 */
export type AlgoParamType =
    | "Integer"
    | "Decimal"
    | "Boolean"
    | "String"
    | "Enum";

export type MetadataParamSection =
    | "General"
    | (string & {});

export type AppHandler =
    | "env.format"
    | "env.type"
    | "env.coordsystem";

export interface MetadataParamResponse {
    id: number;
    name: string;
    paramType: AlgoParamType;
    enumValues: string[];
    defaultValue?: string;
    section?: MetadataParamSection;
    appHandler: AppHandler | null;
}

export type StyleAttributeKey =
    | "Z-Index"
    // Point — Marker Shape
    | "Point Shape"
    | "Point Radius"
    // Point — Overlap
    | "Point Overlap Spacing"
    | "Point Overlap Layout"
    // Point — Border
    | "Point Border Color"
    | "Point Border Width"
    | "Point Border Style"
    // Point — Fill
    | "Point Fill Color"
    // Point — Id Label
    | "Point ID Color"
    | "Point ID Font Size"
    | "Point ID Font Weight"
    | "Point ID Placement"
    | "Point ID Offset"
    // Point — Text Label
    | "Point Label Color"
    | "Point Label Font Size"
    | "Point Label Font Weight"
    | "Point Label Placement"
    | "Point Label Offset"
    // Line — Edge
    | "Line Edge Color"
    | "Line Edge Width"
    | "Line Edge Style"
    // Line — Arrow
    | "Line Arrow Start"
    | "Line Arrow End"
    | "Line Arrow Mid"
    | "Line Arrow Mid Spacing"
    | "Line Arrow Size"
    // Polygon — Edge
    | "Polygon Edge Color"
    | "Polygon Edge Width"
    | "Polygon Edge Style"
    // Polygon — Fill
    | "Polygon Fill Color"
    | "Polygon Fill Style"
    // Polygon — ID
    | "Polygon ID Color"
    | "Polygon ID Font Size"
    | "Polygon ID Font Weight"
    | "Polygon ID Shape"
    | "Polygon ID Radius"
    | "Polygon ID Border Color"
    | "Polygon ID Border Width"
    | "Polygon ID Border Style"
    | "Polygon ID Fill Color"
    | "Polygon ID Placement"
    | "Polygon ID Offset";

export type StyleType =
    | "Integer"
    | "Color"
    | "Spacing"
    | "PointShapeEnum"
    | "PolygonIDShapeEnum"
    | "StrokeStyleEnum"
    | "FillStyleEnum"
    | "OverlapLayoutEnum"
    | "PlacementEnum"
    | "FontSizeEnum"
    | "FontWeightEnum"
    | "LineArrowStartEnum"
    | "LineArrowEndEnum"
    | "LineArrowMidEnum";

export type LayerType = "Point" | "Line" | "Polygon";

export interface LayerStyleAttribute {
    key: StyleAttributeKey;
    styleType: StyleType;
    defaultValue: string | null;
}

export interface PointLabelColorEntry {
    value: string;
    color: string | null;
}

export interface LayerStyle {
    generalStyleAttributes: LayerStyleAttribute[];
    pointStyleAttributes?: LayerStyleAttribute[];
    lineStyleAttributes?: LayerStyleAttribute[];
    polygonStyleAttributes?: LayerStyleAttribute[];
    pointLabelColorMapping?: PointLabelColorEntry[];
}

export interface MetadataLayerResponse {
    id: number;
    computeLayer: string;
    name: string;
    layerType: LayerType;
    style: LayerStyle;
    pointLabelEnumValues?: string[];
}

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
