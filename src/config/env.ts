export interface ProviderConfig {
    port: number;
    providerName: string;
    allowedOrigins: string[] | "*";
}

const DEFAULT_PORT = 8080;
const DEFAULT_ALLOWED_ORIGINS = [
    "http://localhost:3000",
    "http://127.0.0.1:3000",
];

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

function parseAllowedOrigins(rawOrigins: string | undefined): string[] | "*" {
    if (!rawOrigins || rawOrigins.trim() === "") {
        return DEFAULT_ALLOWED_ORIGINS;
    }

    if (rawOrigins.trim() === "*") {
        return "*";
    }

    const origins = rawOrigins
        .split(",")
        .map((origin) => origin.trim())
        .filter(Boolean);

    return origins.length > 0 ? origins : DEFAULT_ALLOWED_ORIGINS;
}

export function getProviderConfig(): ProviderConfig {
    return {
        port: parsePort(process.env.PORT),
        providerName: "blade-provider",
        allowedOrigins: parseAllowedOrigins(process.env.BLADE_PROVIDER_ALLOWED_ORIGINS),
    };
}
