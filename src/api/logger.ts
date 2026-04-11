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

const RESET = "\x1b[0m";
const COLORS: Record<LogLevel, string> = {
    debug: "\x1b[90m",   // gray
    info: "\x1b[36m",   // cyan
    warn: "\x1b[33m",   // yellow
    error: "\x1b[31m",   // red
};

function tag(level: LogLevel): string {
    return `${COLORS[level]}[${level}]${RESET}`;
}

export const log = {
    debug: (...args: unknown[]) => { if (shouldLog("debug")) console.log(tag("debug"), ...args); },
    info: (...args: unknown[]) => { if (shouldLog("info")) console.log(tag("info"), ...args); },
    warn: (...args: unknown[]) => { if (shouldLog("warn")) console.warn(tag("warn"), ...args); },
    error: (...args: unknown[]) => { if (shouldLog("error")) console.error(tag("error"), ...args); },
};
