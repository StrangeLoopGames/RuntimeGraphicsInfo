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
        RuntimeGraphicsInfo.h
        DirectX11.cpp
        Metal.cpp
)

if (APPLE)
    target_precompile_headers(RuntimeGraphicsInfoNative PRIVATE $<$<COMPILE_LANGUAGE:OBJC>:pch.h>)
else()
    target_precompile_headers(RuntimeGraphicsInfoNative PRIVATE pch.h)
endif()

if (WIN32)
    target_sources(RuntimeGraphicsInfoNative PRIVATE dllmain.cpp)
    add_compile_definitions(SUPPORT_VULKAN=1)
endif()

if (APPLE)
    set(CMAKE_C_FLAGS "-x objective-c")
    add_compile_options("-dynamiclib")
    target_sources(RuntimeGraphicsInfoNative PRIVATE Metal.mm)
elseif(UNIX)
    add_compile_definitions(UNITY_LINUX=1 SUPPORT_VULKAN=1)
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS} -Wl,-undefined,dynamic_lookup")
endif()