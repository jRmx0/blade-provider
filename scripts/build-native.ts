/**
 * Native provider build script.
 *
 * This script compiles the C entry points that are exposed through the
 * Bun FFI bridge in `src/api/native/providerMetadataBridge.ts`.
 *
 * Adding a new algorithm module:
 * 1. Create the algorithm's public entry point `.c` file under `src/core/<algorithm>/`
 *    plus any helper translation units it depends on (for example metadata/compute files).
 * 2. Wire the algorithm into `src/core/dispatcher.c` so metadata requests append the
 *    algorithm JSON and compute requests route to the algorithm by name.
 * 3. Add every new `.c` file that must be compiled for that algorithm to the `sources`
 *    array below. If a file is omitted here, it will not be included in the shared library.
 * 4. Rebuild with `bun run build:native` and verify the metadata bridge still loads the
 *    generated library successfully.
 *
 * Keep the `sources` array explicit rather than auto-discovering files so the native build
 * remains predictable and only ships translation units intentionally exposed by the provider.
 */

import { spawnSync } from "node:child_process";
import { mkdirSync, readdirSync, rmSync, writeFileSync } from "node:fs";
import { join } from "node:path";

const projectRoot = process.cwd();
const outDir = join(projectRoot, "out", "native");
const manifestPath = join(outDir, "provider_core.manifest.json");
const outputBaseName = "provider_core";
const outputExtension = process.platform === "win32"
    ? "dll"
    : process.platform === "darwin"
        ? "dylib"
        : "so";
const compiler = process.env.CC || "gcc";
const buildId = `${Date.now()}`;
const outputFileName = `${outputBaseName}.${buildId}.${outputExtension}`;
const outputPath = join(outDir, outputFileName);

// Every algorithm must list all of its required C translation units here.
// The dispatcher is the shared entry point, while the remaining sources are
// linked into the versioned native library consumed by the Bun FFI bridge.
const sources = [
    join(projectRoot, "src", "core", "dispatcher.c"),
    join(projectRoot, "src", "core", "boustrophedon_cellular_decomposition", "bcd.c"),
    join(projectRoot, "src", "core", "boustrophedon_cellular_decomposition", "metadata", "bcd_metadata.c"),
    join(projectRoot, "src", "core", "boustrophedon_cellular_decomposition", "check", "bcd_check.c"),
    join(projectRoot, "src", "core", "boustrophedon_cellular_decomposition", "compute", "bcd_compute.c"),
    join(projectRoot, "src", "core", "mock_algo", "mock_algo.c"),
    join(projectRoot, "src", "core", "mock_algo", "check", "mock_algo_check.c"),
    join(projectRoot, "src", "core", "mock_algo", "metadata", "mock_algo_metadata.c"),
    join(projectRoot, "src", "core", "mock_algo", "compute", "mock_algo_compute.c"),
    join(projectRoot, "dependencies", "cJSON", "cJSON.c"),
];
const compilerArgs = process.platform === "win32"
    ? ["-shared", "-o", outputPath, ...sources]
    : ["-shared", "-fPIC", "-o", outputPath, ...sources];

function cleanupOldBuilds(currentFileName: string) {
    const buildPrefix = `${outputBaseName}.`;
    const buildSuffix = `.${outputExtension}`;

    for (const entry of readdirSync(outDir, { withFileTypes: true })) {
        if (!entry.isFile()) {
            continue;
        }

        if (entry.name === currentFileName || entry.name === "provider_core.manifest.json") {
            continue;
        }

        if (!entry.name.startsWith(buildPrefix) || !entry.name.endsWith(buildSuffix)) {
            continue;
        }

        try {
            rmSync(join(outDir, entry.name), { force: true });
        } catch {
            // Ignore locked native libraries still in use by a running process.
        }
    }
}

mkdirSync(outDir, { recursive: true });

const result = spawnSync(compiler, compilerArgs, {
    cwd: projectRoot,
    stdio: "inherit",
});

if (result.error) {
    console.error(`Failed to run ${compiler}:`, result.error.message);
    process.exit(1);
}

if (result.status !== 0) {
    process.exit(result.status ?? 1);
}

writeFileSync(manifestPath, JSON.stringify({
    fileName: outputFileName,
    builtAt: new Date().toISOString(),
}, null, 2));

cleanupOldBuilds(outputFileName);

console.log(`Native provider library built at ${outputPath}`);
