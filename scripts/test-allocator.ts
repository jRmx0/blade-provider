import { cc } from "bun:ffi";

const { symbols } = cc({
    source: "./scripts/test_allocator.c",
    include: ["./dependencies/tcc-headers", "./dependencies/tcc-headers/winapi"],
    flags: ["-w"],
    symbols: {
        run_allocator_tests: {
            args: [],
            returns: "void",
        },
    } as const,
});

symbols.run_allocator_tests();
