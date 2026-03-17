/**
 * bcd.h
 *
 * Public interface for the Boustrophedon Cellular Decomposition algorithm.
 * The only header exposed outside of this module.
 * Declares the two entry points: metadata and compute.
 *
 * Included by: dispatcher.c
 */

#ifndef BCD_H
#define BCD_H

char *bcd_get_metadata_json(void);
char *bcd_compute(const char *input_environment_json);

#endif // BCD_H