#ifndef MOCK_ALGO_H
#define MOCK_ALGO_H

#ifdef _WIN32
#define MOCK_ALGO_API __declspec(dllexport)
#else
#define MOCK_ALGO_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

MOCK_ALGO_API char *mock_algo_get_metadata_json(void);
MOCK_ALGO_API char *mock_algo_compute(const char *input_environment_json);

#ifdef __cplusplus
}
#endif

#endif // MOCK_ALGO_H
