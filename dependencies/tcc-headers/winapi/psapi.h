/**
 * psapi.h - Minimal Process Status API stub (TCC/MinGW compat)
 *
 * Declares only the subset used by the allocator tracking layer:
 *   PROCESS_MEMORY_COUNTERS, GetProcessMemoryInfo.
 */
#ifndef _PSAPI_H_
#define _PSAPI_H_

#include <windows.h>

typedef struct _PROCESS_MEMORY_COUNTERS {
    DWORD  cb;
    DWORD  PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS, *PPROCESS_MEMORY_COUNTERS, *LPPROCESS_MEMORY_COUNTERS;

/* On Windows Vista+, GetProcessMemoryInfo is forwarded to K32GetProcessMemoryInfo
 * in kernel32.dll (always linked by TCC). Declare the kernel32 variant directly
 * so we avoid a dependency on psapi.lib. */
WINBASEAPI BOOL WINAPI K32GetProcessMemoryInfo(HANDLE Process,
                                               PPROCESS_MEMORY_COUNTERS ppsmemCounters,
                                               DWORD cb);
#define GetProcessMemoryInfo K32GetProcessMemoryInfo

#endif /* _PSAPI_H_ */
