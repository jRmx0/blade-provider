import type { ServerContext } from "../types/apiTypes";
import { InMemoryJobStore } from "../job/jobStore";
import { DebugSessionStore } from "../job/debugSessionStore";
import { getProviderConfig } from "./envParser";
import { log } from "./logger";
import { routeRequest } from "./server";

const context: ServerContext = {
	config: getProviderConfig(),
	jobs: new InMemoryJobStore(),
	debugSessions: new DebugSessionStore(),
	processHandle: { activeProcess: null },
};

const server = Bun.serve({
	port: context.config.port,
	idleTimeout: 30,
	async fetch(request) {
		return routeRequest(request, context);
	},
	error(error) {
		console.error("Unhandled provider error", error);
		return new Response(JSON.stringify({
			error: {
				code: "internal_error",
				message: "Unhandled provider error.",
			},
		}, null, 2), {
			status: 500,
			headers: {
				"Content-Type": "application/json",
			},
		});
	},
});

log.info(`Blade Provider listening on ${server.url}`);