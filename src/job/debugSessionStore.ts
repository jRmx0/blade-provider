// Public session descriptor — returned by create/restart/get.
export interface DebugSession {
    sessionId: string;
    totalSteps: number;
    stepIndex: number; // 0 = not started; N = N segments revealed
    createdAt: string;
}

/**
 * A debug step snapshot.
 *
 * Identical in structure to a full compute response, but with
 * `coveragePathPlan.segments` containing only the segments revealed so far
 * (indices 0 to stepIndex - 1). Extra `_debug` metadata is merged at the
 * top level so the client can read progress without any separate envelope.
 */
export type DebugStepSnapshot = Record<string, unknown> & {
    _debug: {
        sessionId: string;
        stepIndex: number;
        totalSteps: number;
        done: boolean;
    };
};

function extractSegments(result: Record<string, unknown>): unknown[] {
    const plan = result["coveragePathPlan"];
    if (!plan || typeof plan !== "object") return [];
    const segments = (plan as Record<string, unknown>)["segments"];
    if (!Array.isArray(segments)) return [];
    return segments;
}

interface StoredSession {
    meta: DebugSession;
    fullResult: Record<string, unknown>;
    segments: unknown[];
}

export class DebugSessionStore {
    private sessions = new Map<string, StoredSession>();

    create(result: Record<string, unknown>): DebugSession {
        const segments = extractSegments(result);
        const meta: DebugSession = {
            sessionId: crypto.randomUUID(),
            totalSteps: segments.length,
            stepIndex: 0,
            createdAt: new Date().toISOString(),
        };
        this.sessions.set(meta.sessionId, { meta, fullResult: result, segments });
        return { ...meta };
    }

    get(sessionId: string): DebugSession | undefined {
        const stored = this.sessions.get(sessionId);
        if (!stored) return undefined;
        return { ...stored.meta };
    }

    /**
     * Advance the session by one step and return a snapshot.
     *
     * The snapshot is the full compute result with `coveragePathPlan.segments`
     * sliced to the newly revealed count. Returns `undefined` when the session
     * does not exist or is already exhausted.
     */
    step(sessionId: string): DebugStepSnapshot | undefined {
        const stored = this.sessions.get(sessionId);
        if (!stored) return undefined;
        const { meta, fullResult, segments } = stored;
        if (meta.stepIndex >= segments.length) return undefined;

        meta.stepIndex += 1;
        const done = meta.stepIndex >= segments.length;

        const snapshot: DebugStepSnapshot = {
            ...fullResult,
            coveragePathPlan: {
                ...(fullResult["coveragePathPlan"] as Record<string, unknown> ?? {}),
                segments: segments.slice(0, meta.stepIndex),
            },
            _debug: {
                sessionId: meta.sessionId,
                stepIndex: meta.stepIndex,
                totalSteps: meta.totalSteps,
                done,
            },
        };
        return snapshot;
    }

    /** Advance to the final step in one go and return the complete snapshot. Returns `undefined` if the session does not exist or is already exhausted. */
    fastForward(sessionId: string): DebugStepSnapshot | undefined {
        const stored = this.sessions.get(sessionId);
        if (!stored) return undefined;
        const { meta, fullResult, segments } = stored;
        if (meta.stepIndex >= segments.length) return undefined;

        meta.stepIndex = segments.length;

        const snapshot: DebugStepSnapshot = {
            ...fullResult,
            coveragePathPlan: {
                ...(fullResult["coveragePathPlan"] as Record<string, unknown> ?? {}),
                segments,
            },
            _debug: {
                sessionId: meta.sessionId,
                stepIndex: meta.stepIndex,
                totalSteps: meta.totalSteps,
                done: true,
            },
        };
        return snapshot;
    }

    restart(sessionId: string): DebugSession | undefined {
        const stored = this.sessions.get(sessionId);
        if (!stored) return undefined;
        stored.meta.stepIndex = 0;
        return { ...stored.meta };
    }

    delete(sessionId: string): boolean {
        return this.sessions.delete(sessionId);
    }
}
