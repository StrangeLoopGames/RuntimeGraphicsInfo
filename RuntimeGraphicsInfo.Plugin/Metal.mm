#include "pch.h"
#include "RuntimeGraphicsInfo.h"
#include "IUnityInterface.h"

#if SUPPORT_METAL
#include "IUnityGraphicsMetal.h"
#import <Metal/Metal.h>

IUnityGraphicsMetalV1* g_MetalGraphics;

unsigned short SetMaxTessellationFactorMetal(IUnityInterfaces* pUnityInterface, unsigned short tessellationLevel)
{
    return 16;
}

void InitMetal(IUnityInterfaces* pUnityInterfaces)
{
    g_MetalGraphics = pUnityInterfaces->Get<IUnityGraphicsMetalV1>();
}

RunTimeGraphicsMemoryInfo GetDeviceStatsMetal()
{
    auto stats = RunTimeGraphicsMemoryInfo();
    id<MTLDevice> device = g_MetalGraphics->MetalDevice();
    if (!device)
    {
        // Log
        return stats;
    }

    stats.DedicatedSystemMemory = [device recommendedMaxWorkingSetSize];

    /*IDXGIDevice* pDXGIDevice = nullptr;
    HRESULT hr = pD3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDXGIDevice);

    if (FAILED(hr))
    {
        // Log
        return stats;
    }

    IDXGIAdapter* pDXGIAdapter = nullptr;
    pDXGIDevice->GetAdapter(&pDXGIAdapter);

    DXGI_ADAPTER_DESC adapterDesc;
    pDXGIAdapter->GetDesc(&adapterDesc);

    pDXGIDevice->Release();

    stats.DedicatedSystemMemory = adapterDesc.DedicatedVideoMemory;
    stats.DedicatedVideoMemory = adapterDesc.DedicatedVideoMemory;
    stats.SharedSystemMemory = adapterDesc.SharedSystemMemory;*/

    return stats;
}
#endif