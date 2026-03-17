import { cc, CString } from "bun:ffi";

const { symbols: { dispatch_metadata_json, dispatch_string_free } } = cc({
    source: "./src/core/dispatcher.c",
    include: ["./dependencies/tcc-headers"],
    flags: ["-w"],
    symbols: {
        dispatch_metadata_json: {
            args: [],
            returns: "ptr",
        },
        dispatch_string_free: {
            args: ["ptr"],
            returns: "void",
        },
    } as const,
});

const ptr = dispatch_metadata_json()!;
const result = new CString(ptr).toString();
dispatch_string_free(ptr);
console.log("[JS] metadata:", result);
