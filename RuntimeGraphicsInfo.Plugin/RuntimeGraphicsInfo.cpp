// RuntimeGraphicsInfo.cpp : Defines the exported functions for the DLL.
//

#include <string>
#include <psapi.h>
#include "pch.h"
#include "RuntimeGraphicsInfo.h"

#include "IUnityInterface.h"
#include "IUnityGraphics.h"

static void UNITY_INTERFACE_API
OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

RunTimeGraphicsMemoryInfo GetDeviceStatsD3D11(IUnityInterfaces* pUnityInterface);
void InitMetal(IUnityInterfaces* pUnityInterfaces);
RunTimeGraphicsMemoryInfo GetDeviceStatsMetal();

static IUnityInterfaces* s_UnityInterfaces = nullptr;
static IUnityGraphics* s_Graphics = nullptr;
static UnityGfxRenderer s_RendererType = UnityGfxRenderer::kUnityGfxRendererNull;
static RunTimeGraphicsMemoryInfo s_Stats;

struct Vector3
{
    float x;
    float y;
    float z;
};

enum TransformChangeType : uint32_t
{
    TransformChangeDispatch = 0,
    Position = 1
};

struct TransformChangedCallbackData
{
    void* transform;
    void* hierarchy;
    void* extra;
    TransformChangeType type;
public:
    TransformChangedCallbackData(void* transform, void* hierarchy, void* extra, TransformChangeType type) : transform(transform), hierarchy(hierarchy), extra(extra), type(type) { }
};

void Transform_set_Position_Intercept(void* transform, Vector3* position);
void (*onTransformChangeCallback)(TransformChangedCallbackData* data) = nullptr;

void* SearchByPattern(void* address, uint64_t pattern, uint64_t mask, uint32_t maxDistance)
{
    auto ptr = (uint64_t)address;
    auto end = ptr + maxDistance;
    for (; ptr < end; ++ptr)
    {
        if (((*(uint64_t*)ptr) & mask) == pattern)
            return (void*)ptr;
    }
    return nullptr;
}

void DebugBreakIfAttached()
{
    if (IsDebuggerPresent())
        DebugBreak();
}

struct PatchSequence
{
public:
    const uint8_t* bytesToReplace;
    const size_t bytesCount;

    explicit PatchSequence(const uint8_t* bytesToReplace, size_t bytesCount) : bytesToReplace(bytesToReplace), bytesCount(bytesCount) { }
};

const uint8_t MovRbxRsp8Bytes[5] = { 0x48, 0x89, 0x5c, 0x24, 0x8  };
PatchSequence MovRbxRsp8(MovRbxRsp8Bytes, sizeof(MovRbxRsp8Bytes));

const uint8_t PushRbxSubRsp20Bytes[6] = { 0x40, 0x53, 0x48, 0x83, 0xec, 0x20 };
PatchSequence PushRbxSubRsp20(PushRbxSubRsp20Bytes, sizeof(PushRbxSubRsp20Bytes));

const uint8_t PushRdiSubRsp30Bytes[6] = { 0x40, 0x57, 0x48, 0x83, 0xec, 0x30 };
PatchSequence PushRdiSubRsp30(PushRdiSubRsp30Bytes, sizeof(PushRdiSubRsp30Bytes));

struct Patch
{
    const uint64_t rva;
    const void* interceptor;
    void** returnAddress;
    PatchSequence patchSequence;
    void* farJumpAddress;

    uint64_t GetPatchAddress(void* rvaBase) const
    {
        return (uint64_t)rvaBase + rva;
    }

public:
    Patch(const uint64_t rva, PatchSequence patchSequence, const void *interceptor, void** returnAddress) : rva(rva), patchSequence(patchSequence), interceptor(interceptor), returnAddress(returnAddress), farJumpAddress(nullptr) {
    }

    /*
     * We use that fact what functions aligned for 0x10 and gaps filled with 0xcc (int 3) op codes.
     * We can try to find a big enough gap (size <= 15)
     */
    static void* FindGap(void* start, size_t size)
    {
        auto aligned = (uint8_t*)start;

        auto address = aligned - 1;
        auto gapStart = aligned - size;
        do {
            if (*(uint8_t *) address != 0xcc)
            {
                // wrong instruction, try next aligned section
                aligned += 0x10;
                address = aligned - 1;
                gapStart = aligned - size;
            }
            else
                --address;
        } while (address >= gapStart);
        return gapStart;
    }

    void* FarJumpPatch(void* patchAddress) const
    {
        static const size_t farJumpPatchSize = 12;

        // far_jump: movabs rax, interceptor
        // jmp rax
        DWORD oldProtect;
        void* gap = FindGap((void*)patchAddress, farJumpPatchSize);
        VirtualProtect((void*) gap, farJumpPatchSize, PAGE_EXECUTE_READWRITE, &oldProtect);
        uint8_t farJumpPatch[farJumpPatchSize] = { 0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xe0 };
        *(uint64_t*)(farJumpPatch + 2) = (uint64_t)interceptor;
        memcpy((void*)gap, farJumpPatch, farJumpPatchSize);
        VirtualProtect((void*) gap, farJumpPatchSize, oldProtect, &oldProtect);
        return gap;
    }

    void Apply(void* rvaBase)
    {
        DebugBreakIfAttached();
        auto address = GetPatchAddress(rvaBase);
        auto cmp = memcmp((void*)address, patchSequence.bytesToReplace, patchSequence.bytesCount);
        if (cmp != 0)
        {
            MessageBox(nullptr, "Failed to apply patch, because bytes to replace doesn't match", "Error", MB_OK);
            return;
        }

        if (farJumpAddress == nullptr)
            farJumpAddress = FarJumpPatch((void*)address);

        // jmp far_jump
        DWORD oldProtect;
        VirtualProtect((void*) address, patchSequence.bytesCount, PAGE_EXECUTE_READWRITE, &oldProtect);
        uint8_t patch[patchSequence.bytesCount];
        memset(patch, 0xcc, patchSequence.bytesCount);
        auto offset = (int64_t)((uint64_t)farJumpAddress - address - 2); // jmp byte offset has 2 bytes size
        if (offset >= -128 && offset <= 127)
        {
            patch[0] = 0xeb;
            patch[1] = offset;
        }
        else
        {
            patch[0] = 0xe9;
            *(uint32_t*)(patch + 1) = offset - 3;                       // extra 3 bytes for 5 byte size jmp with 32-bit offset
        }
        memcpy((void*)address, patch, patchSequence.bytesCount);
        VirtualProtect((void*) address, patchSequence.bytesCount, oldProtect, &oldProtect);

        *returnAddress = (void*)(address + patchSequence.bytesCount);
    }

    void Rollback(void* rvaBase)
    {
        auto address = GetPatchAddress(rvaBase);

        DWORD oldProtect;
        VirtualProtect((void*) address, patchSequence.bytesCount, PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy((void*) address, patchSequence.bytesToReplace, patchSequence.bytesCount);
        VirtualProtect((void*) address, patchSequence.bytesCount, oldProtect, &oldProtect);
    }
};

#define INTERCEPTOR_MOV_RBX_RSP_8(FunctionName)                                                                               \
void* FunctionName##_return; \
__declspec(naked) void FunctionName##Interceptor()                                                              \
{                                                                                                               \
    asm("movq        %%rbx, 0x8(%%rsp)\n\r"  \
        "movq        %%rcx, 0x10(%%rsp)\n\r" \
        "movq        %%rdx, 0x18(%%rsp)\n\r" \
        "push        %%rbp\n\r" \
        "mov         %%rsp, %%rbp\n\r" \
        "subq        $0x20, %%rsp\n\r" \
        "lea         %1, %%rax\n\r" \
        "call        *%%rax\n\r" \
        "addq        $0x20, %%rsp\n\r" \
        "pop         %%rbp\n\r" \
        "mov         0x10(%%rsp), %%rcx\n\r" \
        "mov         0x18(%%rsp), %%rdx\n\r" \
        "movq        %0, %%rax\n\r" \
        "jmp         *%%rax"::"m"(FunctionName##_return),"m"(FunctionName##_Intercept)); \
}

#define INTERCEPTOR_PUSH_RBX_SUB_RSP_20(FunctionName) \
void* FunctionName##_return; \
__declspec(naked) void FunctionName##Interceptor() \
{ \
    asm("push        %%rbx\n\r" \
        "subq        $0x40, %%rsp\n\r" \
        "movq        %%rcx, 0x20(%%rsp)\n\r" \
        "movq        %%rdx, 0x28(%%rsp)\n\r" \
        "lea         %1, %%rax\n\r" \
        "call        *%%rax\n\r" \
        "mov         0x20(%%rsp), %%rcx\n\r" \
        "mov         0x28(%%rsp), %%rdx\n\r" \
        "addq        $0x20, %%rsp\n\r" \
        "movq        %0, %%rax\n\r" \
        "jmp         *%%rax"::"m"(FunctionName##_return),"m"(FunctionName##_Intercept)); \
}

#define INTERCEPTOR_PUSH_RDI_SUB_RSP_30(FunctionName) \
void* FunctionName##_return; \
__declspec(naked) void FunctionName##Interceptor() \
{ \
    asm("push        %%rdi\n\r" \
        "subq        $0x50, %%rsp\n\r" \
        "movq        %%rcx, 0x20(%%rsp)\n\r" \
        "movq        %%rdx, 0x28(%%rsp)\n\r" \
        "lea         %1, %%rax\n\r" \
        "call        *%%rax\n\r" \
        "mov         0x20(%%rsp), %%rcx\n\r" \
        "mov         0x28(%%rsp), %%rdx\n\r" \
        "addq        $0x20, %%rsp\n\r" \
        "movq        %0, %%rax\n\r" \
        "jmp         *%%rax"::"m"(FunctionName##_return),"m"(FunctionName##_Intercept)); \
}

INTERCEPTOR_MOV_RBX_RSP_8(Transform_set_Position)

Patch playerPatch(0xb84b00, MovRbxRsp8, (void *) Transform_set_PositionInterceptor, &Transform_set_Position_return);
Patch il2cppPatch(0xB89B80, MovRbxRsp8, (void *) Transform_set_PositionInterceptor, &Transform_set_Position_return);
Patch editorPatch(0xd40300, MovRbxRsp8, (void *) Transform_set_PositionInterceptor, &Transform_set_Position_return);

inline bool SameAddressSpace(uint64_t x, uint64_t y)
{
    return (x & 0xfffffff000000000) == (y & 0xfffffff000000000);
}

bool IsTransformForHierarchy(void* transform, void* transformHierarchy, size_t offset)
{
    return SameAddressSpace((uint64_t) transform, (uint64_t)transformHierarchy) && *(void**)((uint64_t)transform + offset) == transformHierarchy;
}

void TransformChangeDispatch_QueueTransformChangeIfHasChanged_Intercept(void* transformQueue, void* transformHierarchy)
{
    register void* _rbx asm("%rbx");
    register void* _rdi asm("%rdi");
    if (onTransformChangeCallback == nullptr)
        return;

    TransformChangedCallbackData data(nullptr, transformHierarchy, nullptr, TransformChangeType::TransformChangeDispatch);
    if (IsTransformForHierarchy(_rbx, transformHierarchy, 0x50))
        data.transform = _rbx;
    else if (IsTransformForHierarchy(_rdi, transformHierarchy, 0x50))
        data.transform = _rdi;
    onTransformChangeCallback(&data);
}

void TransformChangeDispatch_QueueTransformChangeIfHasChanged_Editor_Intercept(void* transformQueue, void* transformHierarchy)
{
    register void* _rbx asm("%rbx");
    register void* _rdi asm("%rdi");
    if (onTransformChangeCallback == nullptr)
        return;

    TransformChangedCallbackData data(nullptr, transformHierarchy, nullptr, TransformChangeType::TransformChangeDispatch);
    if (IsTransformForHierarchy(_rbx, transformHierarchy, 0x78))
        data.transform = _rbx;
    else if (IsTransformForHierarchy(_rdi, transformHierarchy, 0x78))
        data.transform = _rdi;
    onTransformChangeCallback(&data);
}

INTERCEPTOR_PUSH_RBX_SUB_RSP_20(TransformChangeDispatch_QueueTransformChangeIfHasChanged)
INTERCEPTOR_PUSH_RDI_SUB_RSP_30(TransformChangeDispatch_QueueTransformChangeIfHasChanged_Editor)

Patch il2cppDispatchPatch(0xB80190, PushRbxSubRsp20, (void *) TransformChangeDispatch_QueueTransformChangeIfHasChangedInterceptor, &TransformChangeDispatch_QueueTransformChangeIfHasChanged_return);
Patch monoDispatchPatch(0xB7B110, PushRbxSubRsp20, (void *) TransformChangeDispatch_QueueTransformChangeIfHasChangedInterceptor, &TransformChangeDispatch_QueueTransformChangeIfHasChanged_return);
Patch editorDispatchPatch(0xD3B960, PushRdiSubRsp30, (void *) TransformChangeDispatch_QueueTransformChangeIfHasChanged_EditorInterceptor, &TransformChangeDispatch_QueueTransformChangeIfHasChanged_Editor_return);

Patch* activePatch;

uint64_t GetModuleRVABase(const char* dllName)
{
    return (uint64_t)GetModuleHandle(dllName);
}

void ApplyPatches()
{
    auto rvaBasePlayer = GetModuleRVABase("UnityPlayer.dll");
    auto rvaBaseEditor = GetModuleRVABase("Unity.exe");
    auto monoRuntime = GetModuleHandle("mono-2.0-bdwgc.dll") != nullptr;

    if (rvaBasePlayer != 0)
    {
        activePatch = &(monoRuntime ? monoDispatchPatch : il2cppDispatchPatch);
        activePatch->Apply(reinterpret_cast<void *>(rvaBasePlayer));
    }
    else if (rvaBaseEditor != 0)
    {
        activePatch = &editorDispatchPatch;
        activePatch->Apply(reinterpret_cast<void *>(rvaBaseEditor));
    }
    else
        MessageBox(nullptr, "Failed to patch Transform.set_Position, no UnityPlayer.dll", "Test", MB_OK);
}

void RollbackPatches()
{
    auto rvaBasePlayer = GetModuleRVABase("UnityPlayer.dll");
    auto rvaBaseEditor = GetModuleRVABase("Unity.exe");

    if (rvaBasePlayer != 0)
        activePatch->Rollback(reinterpret_cast<void *>(rvaBasePlayer));
    else if (rvaBaseEditor != 0)
        activePatch->Rollback(reinterpret_cast<void *>(rvaBaseEditor));
}

class MonoAssembly
{
    HMODULE hModule;

    void* (*mono_domain_get)();
    void* (*mono_domain_assembly_open)(void *domain, const char *name);
    void* (*mono_assembly_get_image)(void *assembly);
    void* (*mono_method_desc_new)(const char *name, int32_t include_namespace);
    void* (*mono_method_desc_search_in_image)(void *desc, void *image);
    void (*mono_method_desc_free)(void *desc);
    void* (*mono_lookup_internal_call)(void *monoMethod);
    void (*mono_add_internal_call)(const char *name, const void* method);

    void* image;

public:
    bool Init(const char* assemblyPath)
    {
        hModule = LoadLibrary("mono-2.0-bdwgc.dll");
        if (hModule == nullptr)
            return false;

        mono_domain_get = (void* (*)()) GetProcAddress(hModule, "mono_domain_get");
        mono_domain_assembly_open = (void* (*)(void *domain, const char *name))GetProcAddress(hModule, "mono_domain_assembly_open");
        mono_assembly_get_image = (void* (*)(void *assembly))GetProcAddress(hModule, "mono_assembly_get_image");
        mono_method_desc_new = (void* (*)(const char *name, int32_t include_namespace))GetProcAddress(hModule, "mono_method_desc_new");
        mono_method_desc_search_in_image = (void* (*)(void *desc, void *image))GetProcAddress(hModule, "mono_method_desc_search_in_image");
        mono_method_desc_free = (void (*)(void *desc))GetProcAddress(hModule, "mono_method_desc_free");
        mono_lookup_internal_call = (void* (*)(void *monoMethod))GetProcAddress(hModule, "mono_lookup_internal_call");
        mono_add_internal_call = (void (*)(const char *name, const void* method))GetProcAddress(hModule, "mono_add_internal_call");

        auto domain = mono_domain_get();
        auto assembly = mono_domain_assembly_open(domain, assemblyPath);
        image = mono_assembly_get_image(assembly);

        return true;
    }

    void* LookupInternalMethod(const char* methodSpec)
    {
        auto desc = mono_method_desc_new(methodSpec, 1);
        auto method = mono_method_desc_search_in_image(desc, image);
        auto ptr = mono_lookup_internal_call(method);
        mono_method_desc_free(desc);
        return ptr;
    }

    void Dispose()
    {
        if (hModule != nullptr)
            FreeLibrary(hModule);
        hModule = nullptr;
    }
};

void* SearchAndResolveByRelativeCall(void* caller, uint64_t offset)
{
    auto callAddress = SearchByPattern(caller, 0xe8 | (offset << 8),  0xffffffffff, 1024);
    return callAddress != nullptr ? (void*)((uint64_t) callAddress + offset + 5) : nullptr;
}

void PatchTransforms()
{
    auto assembly = (MonoAssembly*) std::malloc(sizeof(MonoAssembly));
    auto inited = assembly->Init("D:/projects/testing/UnityTest/Build VS/build/bin/UnityTest_Data/Managed/UnityEngine.CoreModule.dll");
    if (inited)
    {
        auto monoFuncAddress = assembly->LookupInternalMethod("UnityEngine.Transform:set_position_Injected");

        if (monoFuncAddress != nullptr)
        {
            auto rvaBasePlayer = GetModuleRVABase("UnityPlayer.dll");
            auto rvaBaseEditor = GetModuleRVABase("Unity.exe");

            uint64_t rvaBase;
            void* funcAddress;
            if ((rvaBase = rvaBasePlayer) != 0)
                funcAddress = SearchAndResolveByRelativeCall(monoFuncAddress, 0x00aaf45e);
            else if ((rvaBase = rvaBaseEditor) != 0)
                funcAddress = SearchAndResolveByRelativeCall(monoFuncAddress, 0x00c62ba1);
            else
                MessageBox(nullptr, "Can't resolve module", "Test", MB_OK);

            if (funcAddress != nullptr) {
                char message[1000];
                message[0] = 0;
                sprintf_s(message, sizeof(message), "set_Position VA: 0x%x RVA: 0x%x RVABase: 0x%x",
                          (uint64_t) funcAddress,
                          (uint64_t) funcAddress - rvaBase, rvaBase);
                MessageBox(nullptr, message, "Test", MB_OK);
            } else
                MessageBox(nullptr, "Can't find call by pattern", "Test", MB_OK);
        }
        else
            MessageBoxA(nullptr, "No Func", "Test", MB_OK);
    }
    else
        MessageBox(nullptr, "Failed to Init Mono", "Test", MB_OK);

    assembly->Dispose();
    std::free(assembly);

    ApplyPatches();
}

void Transform_set_Position_Intercept(void* transform, Vector3* position)
{
    /*if (IsDebuggerPresent())
        DebugBreak();*/
    //MessageBox(nullptr, "OMG", "Test", MB_OK);
    TransformChangedCallbackData data(transform, nullptr, position, TransformChangeType::Position);
    if (onTransformChangeCallback != nullptr)
        onTransformChangeCallback(&data);
    //RollbackPatches();
}

// Unity plugin load event
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(IUnityInterfaces * unityInterfaces)
{
    s_UnityInterfaces = unityInterfaces;
    s_Graphics = unityInterfaces->Get<IUnityGraphics>();

    s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);

    // Run OnGraphicsDeviceEvent(initialize) manually on plugin load
    // to not miss the event in case the graphics device is already initialized
    OnGraphicsDeviceEvent(UnityGfxDeviceEventType::kUnityGfxDeviceEventInitialize);
}

// Unity plugin unload event
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginUnload()
{
    s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

RunTimeGraphicsMemoryInfo GetStatsForDevice(UnityGfxRenderer renderer)
{
    switch (renderer)
    {
    case UnityGfxRenderer::kUnityGfxRendererD3D11:
        return GetDeviceStatsD3D11(s_UnityInterfaces);
    case UnityGfxRenderer::kUnityGfxRendererMetal:
        return GetDeviceStatsMetal();
    case UnityGfxRenderer::kUnityGfxRendererVulkan:
        return {};
    default:
        return {};
    }
}

static void UNITY_INTERFACE_API
OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
    switch (eventType)
    {
    case UnityGfxDeviceEventType::kUnityGfxDeviceEventInitialize:
    {
        s_RendererType = s_Graphics->GetRenderer();
        if (s_RendererType == UnityGfxRenderer::kUnityGfxRendererMetal)
            InitMetal(s_UnityInterfaces);

        s_Stats = GetStatsForDevice(s_RendererType);

        break;
    }
    case UnityGfxDeviceEventType::kUnityGfxDeviceEventShutdown:
    {
        s_RendererType = UnityGfxRenderer::kUnityGfxRendererNull;
        //TODO: user shutdown code
        break;
    }
    case UnityGfxDeviceEventType::kUnityGfxDeviceEventBeforeReset:
    {
        //TODO: user Direct3D 9 code
        break;
    }
    case UnityGfxDeviceEventType::kUnityGfxDeviceEventAfterReset:
    {
        //TODO: user Direct3D 9 code
        break;
    }
    }
}

extern "C" uint64_t UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetDedicatedVideoMemory()
{
    return s_Stats.DedicatedVideoMemory;
}

extern "C" uint64_t UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetDedicatedSystemMemory()
{
    return s_Stats.DedicatedSystemMemory;
}

extern "C" uint64_t UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetSharedSystemMemory()
{
    return s_Stats.SharedSystemMemory;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTransformChangeCallback(void (*onTransformChange)(TransformChangedCallbackData* transform))
{
#if WINAPI_FAMILY_PARTITION (WINAPI_PARTITION_APP)
    if (::onTransformChangeCallback != nullptr)
        RollbackPatches();
    ::onTransformChangeCallback = onTransformChange;
    if (::onTransformChangeCallback != nullptr)
        PatchTransforms();
#endif
}