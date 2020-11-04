#include "pch.h"

#include "RuntimeGraphicsInfo.h"
#include "IUnityInterface.h"

#if SUPPORT_D3D11
#include <d3d11.h>

#include "IUnityGraphicsD3D11.h"

RunTimeGraphicsMemoryInfo GetDeviceStatsD3D11(IUnityInterfaces* pUnityInterface)
{
    auto stats = RunTimeGraphicsMemoryInfo();

    auto pUnityGraphicsD3D11 = pUnityInterface->Get<IUnityGraphicsD3D11>();
    if (!pUnityGraphicsD3D11)
    {
        // Log
        return stats;
    }

    auto pD3dDevice = pUnityGraphicsD3D11->GetDevice();
    if (!pD3dDevice)
    {
        // Log
        return stats;
    }

    IDXGIDevice* pDXGIDevice = nullptr;
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
    stats.SharedSystemMemory = adapterDesc.SharedSystemMemory;

    return stats;
}
#else
RunTimeGraphicsMemoryInfo GetDeviceStatsD3D11(IUnityInterfaces* pUnityInterface) { return {}; }
#endif