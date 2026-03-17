import type { ProviderConfig } from "../types/apiTypes";

const DEFAULT_PORT = 8080;
const DEFAULT_PROVIDER_NAME = "blade-provider";

function parsePort(rawPort: string | undefined): number {
    if (!rawPort) {
        return DEFAULT_PORT;
    }

    const parsedPort = Number(rawPort);
    if (!Number.isInteger(parsedPort) || parsedPort < 1 || parsedPort > 65535) {
        throw new Error(`Invalid PORT value: ${rawPort}`);
    }

    return parsedPort;
}

function parseProviderName(rawProviderName: string | undefined): string {
    const providerName = rawProviderName?.trim();

    return providerName && providerName.length > 0
        ? providerName
        : DEFAULT_PROVIDER_NAME;
}

export function getProviderConfig(): ProviderConfig {
    return {
        port: parsePort(process.env.PORT),
        providerName: parseProviderName(process.env.BLADE_PROVIDER_NAME),
    };
}
