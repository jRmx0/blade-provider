import type { MetadataAlgorithmResponse, MetadataResponse } from "./providerTypes";

const ALGORITHMS: MetadataAlgorithmResponse[] = [
    {
        name: "mock_cpp_bcd",
        label: "Mock Coverage Planner (BCD)",
        parameters: [
            {
                name: "path_width",
                label: "Path Width",
                paramType: "decimal",
                defaultValue: "3.0",
            },
            {
                name: "path_overlap",
                label: "Path Overlap",
                paramType: "decimal",
                defaultValue: "0.15",
            },
            {
                name: "allow_reverse",
                label: "Allow Reverse Traversal",
                paramType: "boolean",
                defaultValue: "true",
            },
            {
                name: "route_mode",
                label: "Route Mode",
                paramType: "enum",
                enumValues: ["balanced", "greedy"],
                defaultValue: "balanced",
            },
        ],
    },
];

const ALGORITHM_BY_NAME = new Map(ALGORITHMS.map((algorithm) => [algorithm.name, algorithm]));

export const algorithmCatalog = {
    list(): MetadataAlgorithmResponse[] {
        return ALGORITHMS.map((algorithm) => ({
            ...algorithm,
            parameters: algorithm.parameters.map((parameter) => ({ ...parameter })),
        }));
    },

    get(name: string): MetadataAlgorithmResponse | undefined {
        const algorithm = ALGORITHM_BY_NAME.get(name);
        if (!algorithm) {
            return undefined;
        }

        return {
            ...algorithm,
            parameters: algorithm.parameters.map((parameter) => ({ ...parameter })),
        };
    },

    metadata(): MetadataResponse {
        return {
            algorithms: this.list(),
        };
    },
};
