

using System;
using System.Runtime.InteropServices;

internal class RuntimeGraphicsInfoInternal
{
    [DllImport("RuntimeGraphicsInfo")]
    public static extern ulong GetDedicatedVideoMemory();

    [DllImport("RuntimeGraphicsInfo")]
    public static extern ulong GetDedicatedSystemMemory();

    [DllImport("RuntimeGraphicsInfo")]
    public static extern ulong GetSharedSystemMemory();
}

public class RuntimeGraphicsInfo
{
    public static ulong GetDedicatedVideoMemory() => RuntimeGraphicsInfoInternal.GetDedicatedSystemMemory();

    public static ulong GetDedicatedSystemMemory() => RuntimeGraphicsInfoInternal.GetDedicatedSystemMemory();

    public static ulong GetSharedSystemMemory() => RuntimeGraphicsInfoInternal.GetSharedSystemMemory();
}