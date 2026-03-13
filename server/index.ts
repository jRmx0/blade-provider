import { createProviderServer } from "../src/http/server";

const server = createProviderServer();

console.log(`Blade Provider listening on ${server.url}`);