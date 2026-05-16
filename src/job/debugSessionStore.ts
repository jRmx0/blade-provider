export interface DebugSegment {
    id: number;
    type: string;
    path: unknown[];
}

export interface DebugSession {
    sessionId: string;
    segments: DebugSegment[];
    stepIndex: number; // 0 = not started; N = N segments revealed
    createdAt: string;
}

export interface StepResult {
    sessionId: string;
    stepIndex: number;
    totalSteps: number;
    segment: DebugSegment;
    done: boolean;
}

function extractSegments(result: Record<string, unknown>): DebugSegment[] {
    const plan = result["coveragePathPlan"];
    if (!plan || typeof plan !== "object") return [];
    const segments = (plan as Record<string, unknown>)["segments"];
    if (!Array.isArray(segments)) return [];
    return segments as DebugSegment[];
}

export class DebugSessionStore {
    private sessions = new Map<string, DebugSession>();

    create(result: Record<string, unknown>): DebugSession {
        const session: DebugSession = {
            sessionId: crypto.randomUUID(),
            segments: extractSegments(result),
            stepIndex: 0,
            createdAt: new Date().toISOString(),
        };
        this.sessions.set(session.sessionId, session);
        return { ...session, segments: session.segments };
    }

    get(sessionId: string): DebugSession | undefined {
        const s = this.sessions.get(sessionId);
        if (!s) return undefined;
        return { ...s, segments: s.segments };
    }

    step(sessionId: string): StepResult | undefined {
        const s = this.sessions.get(sessionId);
        if (!s) return undefined;
        if (s.stepIndex >= s.segments.length) return undefined;

        s.stepIndex += 1;
        const segment = s.segments[s.stepIndex - 1]!;
        return {
            sessionId: s.sessionId,
            stepIndex: s.stepIndex,
            totalSteps: s.segments.length,
            segment,
            done: s.stepIndex >= s.segments.length,
        };
    }

    restart(sessionId: string): DebugSession | undefined {
        const s = this.sessions.get(sessionId);
        if (!s) return undefined;
        s.stepIndex = 0;
        return { ...s, segments: s.segments };
    }

    delete(sessionId: string): boolean {
        return this.sessions.delete(sessionId);
    }
}
