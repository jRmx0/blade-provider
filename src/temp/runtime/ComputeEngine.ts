import type { ComputeResult, ResolvedComputeRequest } from "../domain/providerTypes";

export interface ComputeEngine {
    execute(request: ResolvedComputeRequest): Promise<ComputeResult>;
}
