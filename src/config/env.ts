export interface ProviderConfig {
    port: number;
    providerName: string;
}

const DEFAULT_PORT = 8080;

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

export function getProviderConfig(): ProviderConfig {
    return {
        port: parsePort(process.env.PORT),
        providerName: "blade-provider",
    };
}
