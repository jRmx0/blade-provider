import { createProviderServer } from "./http/server";
import { warmNativeProviderMetadata } from "./native/nativeCore";

try {
	warmNativeProviderMetadata();
} catch (error) {
	console.error("Failed to initialize native provider metadata.", error);
	process.exit(1);
}

const server = createProviderServer();

console.log(`Blade Provider listening on ${server.url}`);