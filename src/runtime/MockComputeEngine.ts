import type { Point, ComputeResult, ResolvedComputeRequest } from "../domain/providerTypes";
import type { ComputeEngine } from "./ComputeEngine";

function clamp(value: number, min: number, max: number) {
    return Math.min(max, Math.max(min, value));
}

function round(value: number, decimals = 2) {
    return Number(value.toFixed(decimals));
}

function delay(ms: number) {
    return new Promise<void>((resolve) => {
        setTimeout(resolve, ms);
    });
}

function buildRoute(request: ResolvedComputeRequest): Point[] {
    const route = request.environment.zones.flatMap((zone) => {
        const firstVertex = zone.vertices[0];

        if (!firstVertex) {
            return [];
        }

        return [...zone.vertices, firstVertex];
    });

    return route.length > 0 ? route : [{ x: 0, y: 0 }];
}

export class MockComputeEngine implements ComputeEngine {
    async execute(request: ResolvedComputeRequest): Promise<ComputeResult> {
        await delay(125);

        const zoneCount = request.environment.zones.length;
        const obstacleCount = request.environment.obstacles.length;
        const totalZoneVertices = request.environment.zones.reduce(
            (sum, zone) => sum + zone.vertices.length,
            0,
        );
        const totalObstacleVertices = request.environment.obstacles.reduce(
            (sum, obstacle) => sum + obstacle.vertices.length,
            0,
        );

        const pathWidth = typeof request.parameters.path_width === "number"
            ? request.parameters.path_width
            : 3;
        const pathOverlapRatio = typeof request.parameters.path_overlap === "number"
            ? request.parameters.path_overlap
            : 0.15;
        const routeMode = typeof request.parameters.route_mode === "string"
            ? request.parameters.route_mode
            : "balanced";
        const allowReverse = request.parameters.allow_reverse === true;

        const route = buildRoute(request);
        const turnCount = Math.max(route.length - zoneCount - 1, 0);

        return {
            zoneCoverage: round(clamp(88 + zoneCount * 2.4 - obstacleCount * 1.3, 0, 100)),
            routeOverlap: round(clamp(pathOverlapRatio * 100, 0, 100)),
            turnCount,
            route,
            intermediateCalculations: {
                engine: "mock",
                algorithmName: request.algorithm.name,
                zoneCount,
                obstacleCount,
                totalZoneVertices,
                totalObstacleVertices,
                pathWidth,
                pathOverlapRatio,
                routeMode,
                allowReverse,
                generatedAt: new Date().toISOString(),
            },
        };
    }
}
