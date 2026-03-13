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

#ifdef _WIN32
#define BCD_API __declspec(dllexport)
#else
#define BCD_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

BCD_API char *bcd_get_metadata_json(void);
BCD_API char *bcd_compute(const char *input_environment_json);

#ifdef __cplusplus
}
#endif

#endif // BCD_H