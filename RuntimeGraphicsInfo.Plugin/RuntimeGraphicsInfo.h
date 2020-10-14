// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the RUNTIMEGRAPHICSINFO_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// RUNTIMEGRAPHICSINFO_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#include <cstdint>

#ifdef RUNTIMEGRAPHICSINFO_EXPORTS
#define RUNTIMEGRAPHICSINFO_API __declspec(dllexport)
#else
#define RUNTIMEGRAPHICSINFO_API __declspec(dllimport)
#endif


struct RunTimeGraphicsMemoryInfo
{
    uint64_t DedicatedVideoMemory;
    uint64_t DedicatedSystemMemory;
    uint64_t SharedSystemMemory;
    uint16_t MaxTessellationFactor;

public:
    RunTimeGraphicsMemoryInfo()
        : DedicatedVideoMemory(-1)
        , DedicatedSystemMemory(-1)
        , SharedSystemMemory(-1)
        , MaxTessellationFactor(64)
    {
    }
};
