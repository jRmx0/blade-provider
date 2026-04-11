import type { LogLevel } from "../types/apiTypes";

const LEVELS: Record<LogLevel, number> = { error: 0, warn: 1, info: 2, debug: 3 };

const DEFAULT_LEVEL: LogLevel = "info";

function parseLevel(): LogLevel {
    const raw = process.env.LOG_LEVEL?.toLowerCase().trim();
    if (raw !== undefined && raw in LEVELS) return raw as LogLevel;
    return DEFAULT_LEVEL;
}

const currentLevel = parseLevel();

function shouldLog(level: LogLevel): boolean {
    return LEVELS[level] <= LEVELS[currentLevel];
}

export const log = {
    debug: (...args: unknown[]) => { if (shouldLog("debug")) console.log("[debug]", ...args); },
    info: (...args: unknown[]) => { if (shouldLog("info")) console.log("[info]", ...args); },
    warn: (...args: unknown[]) => { if (shouldLog("warn")) console.warn("[warn]", ...args); },
    error: (...args: unknown[]) => { if (shouldLog("error")) console.error("[error]", ...args); },
};
