import type { MetadataAlgorithmResponse, MetadataResponse } from "./providerTypes";
import {
    getNativeProviderAlgorithm,
    getNativeProviderMetadata,
    warmNativeProviderMetadata,
} from "../../api/native/providerMetadataBridge";

export const algorithmCatalog = {
    list(): MetadataAlgorithmResponse[] {
        return getNativeProviderMetadata().algorithms;
    },

    get(name: string): MetadataAlgorithmResponse | undefined {
        return getNativeProviderAlgorithm(name);
    },

    metadata(): MetadataResponse {
        return getNativeProviderMetadata();
    },
};

export function warmAlgorithmCatalog(): void {
    warmNativeProviderMetadata();
}
