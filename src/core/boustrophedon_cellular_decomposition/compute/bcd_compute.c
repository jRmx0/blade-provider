/**
 * bcd_compute.c
 *
 * Orchestrates the BCD computation pipeline.
 * Receives input and a progress callback from bcd.c,
 * executes computation steps in sequence, reports progress
 * after each step, and returns the final result.
 *
 * Adding a new step:
 *   1. Implement the step file in compute/
 *   2. Call it in sequence here and report progress
 *
 * Dependencies: internal.h, step files
 */