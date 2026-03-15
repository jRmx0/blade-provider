#ifndef DISPATCHER_H
#define DISPATCHER_H

#ifdef _WIN32
#define PROVIDER_API __declspec(dllexport)
#else
#define PROVIDER_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

PROVIDER_API char *dispatch_metadata_json(void);
PROVIDER_API char *dispatch_compute_json(const char *request_json);
PROVIDER_API void dispatch_string_free(char *value);

#ifdef __cplusplus
}
#endif

#endif // DISPATCHER_H