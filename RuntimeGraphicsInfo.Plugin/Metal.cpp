#include "pch.h"
#include "RuntimeGraphicsInfo.h"
#include "IUnityInterface.h"

#if !SUPPORT_METAL
void InitMetal(IUnityInterfaces*) { }
RunTimeGraphicsMemoryInfo GetDeviceStatsMetal() { return {}; }

unsigned short SetMaxTessellationFactorMetal(IUnityInterfaces* pUnityInterface, unsigned short tessellationLevel) { return 16; }
#endif