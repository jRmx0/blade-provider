import { createProviderServer } from "./http/server";

const server = createProviderServer();

console.log(`Blade Provider listening on ${server.url}`);