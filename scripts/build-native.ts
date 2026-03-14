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
const sources = [
    join(projectRoot, "src", "core", "dispatcher.c"),
    join(projectRoot, "src", "core", "boustrophedon_cellular_decomposition", "bcd.c"),
    join(projectRoot, "src", "core", "boustrophedon_cellular_decomposition", "bcd_metadata.c"),
    join(projectRoot, "src", "core", "boustrophedon_cellular_decomposition", "compute", "bcd_compute.c"),
    join(projectRoot, "src", "core", "mock_algo", "mock_algo.c"),
    join(projectRoot, "src", "core", "mock_algo", "mock_algo_metadata.c"),
    join(projectRoot, "src", "core", "mock_algo", "mock_algo_compute.c"),
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
