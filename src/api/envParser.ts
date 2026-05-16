import type { LogLevel, ProviderConfig } from "../types/apiTypes";

const DEFAULT_PORT = 8080;
const DEFAULT_PROVIDER_NAME = "blade-provider";
const DEFAULT_LOG_LEVEL: LogLevel = "info";
const VALID_LOG_LEVELS: LogLevel[] = ["error", "warn", "info", "debug"];

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

function parseLogLevel(raw: string | undefined): LogLevel {
    const normalized = raw?.toLowerCase().trim();
    if (normalized !== undefined && VALID_LOG_LEVELS.includes(normalized as LogLevel)) {
        return normalized as LogLevel;
    }
    return DEFAULT_LOG_LEVEL;
}

export function getProviderConfig(): ProviderConfig {
    return {
        port: parsePort(process.env.PORT),
        providerName: parseProviderName(process.env.BLADE_PROVIDER_NAME),
        logLevel: parseLogLevel(process.env.LOG_LEVEL),
    };
}
