import { cc } from "bun:ffi";

// Type-safe FFI definition
const { symbols: { hello } } = cc({
    source: "./hello.c",
    include: ["../../dependencies/tcc-headers"], // TinyCC-compatible Windows C headers (vendored)
    symbols: {
        hello: {
            args: ["cstring"],
            returns: "void"
        }
    } as const,
});

// Create a CString from TypeScript string
const name = "World";
const cString = Buffer.from(name);

hello(cString); // Output: "Hello World from C!"