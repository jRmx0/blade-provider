import { createProviderServer } from "./server";
import { warmCoreMetadata } from "./coreBridge";

try {
	warmCoreMetadata();
} catch (error) {
	console.error("Failed to initialize provider core metadata.", error);
	process.exit(1);
}

const server = createProviderServer();

console.log(`Blade Provider listening on ${server.url}`);