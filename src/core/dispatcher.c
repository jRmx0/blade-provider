/**
 * dispatcher.c
 *
 * Central routing layer between the API and core algorithms.
 * Receives incoming requests from api/, determines which algorithm
 * to invoke based on input type, and delegates to the corresponding
 * algorithm's metadata or compute function.
 *
 * Adding a new algorithm:
 *   1. Include its header
 *   2. Add a case to dispatch_metadata() and dispatch_compute()
 *
 * Dependencies: algorithm public headers (bcd.h, ...)
 * Exposed to: api/
 */