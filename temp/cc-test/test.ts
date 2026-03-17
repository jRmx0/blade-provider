import { cc, CString } from "bun:ffi";

const { symbols: { greet } } = cc({
    source: "./temp/cc-test/entry.c",
    include: ["./dependencies/tinycc-mob/win32/include"],
    symbols: {
        greet: {
            args: [],
            returns: "ptr",
        },
    },
});

const ptr = greet()!;
const result = new CString(ptr);
console.log("[JS] Received from C:", result.toString());
