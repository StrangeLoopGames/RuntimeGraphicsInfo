#include "pch.h"
#include "RuntimeGraphicsInfo.h"
#include "IUnityInterface.h"

#if SUPPORT_METAL
#include "IUnityGraphicsMetal.h"

RunTimeGraphicsMemoryInfo GetDeviceStatsMetal(IUnityInterfaces* pUnityInterface)
{
    auto stats = RunTimeGraphicsMemoryInfo();

    auto pUnityGraphicsMetal = pUnityInterface->Get<IUnityGraphicsMetalV1>();
    if (!pUnityGraphicsMetal)
    {
        // Log
        return stats;
    }

    auto device = pUnityGraphicsMetal->MetalDevice();
    if (!device)
    {
        // Log
        return stats;
    }

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