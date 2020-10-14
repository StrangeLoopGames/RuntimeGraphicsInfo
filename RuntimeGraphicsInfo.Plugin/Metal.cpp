#include "pch.h"
#include "RuntimeGraphicsInfo.h"
#include "IUnityInterface.h"

#if !SUPPORT_METAL
RunTimeGraphicsMemoryInfo GetDeviceStatsMetal(IUnityInterfaces* pUnityInterface) { return {}; }
#endif