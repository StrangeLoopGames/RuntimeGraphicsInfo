include_directories(PluginAPI)

add_library(RuntimeGraphicsInfoNative SHARED
        PluginAPI/IUnityEventQueue.h
        PluginAPI/IUnityGraphics.h
        PluginAPI/IUnityGraphicsD3D11.h
        PluginAPI/IUnityGraphicsD3D12.h
        PluginAPI/IUnityGraphicsMetal.h
        PluginAPI/IUnityGraphicsVulkan.h
        PluginAPI/IUnityInterface.h
        PluginAPI/IUnityProfilerCallbacks.h
        PluginAPI/IUnityRenderingExtensions.h
        PluginAPI/IUnityShaderCompilerAccess.h
        framework.h
        RuntimeGraphicsInfo.cpp
        RuntimeGraphicsInfo.h)

target_precompile_headers(RuntimeGraphicsInfoNative PRIVATE pch.h)

if (WIN32)
    target_sources(RuntimeGraphicsInfoNative PRIVATE DirectX11.cpp dllmain.cpp)
endif()

if (APPLE)
    target_sources(RuntimeGraphicsInfoNative PRIVATE Metal.cpp)
endif()