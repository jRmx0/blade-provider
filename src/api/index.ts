import { createProviderServer } from "./http/server";
import { warmAlgorithmCatalog } from "../temp/domain/catalog";

try {
	warmAlgorithmCatalog();
} catch (error) {
	console.error("Failed to initialize native provider metadata.", error);
	process.exit(1);
}

const server = createProviderServer();

console.log(`Blade Provider listening on ${server.url}`);