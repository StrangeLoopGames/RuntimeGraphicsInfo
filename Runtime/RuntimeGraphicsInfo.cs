using System;
using System.Runtime.InteropServices;

#if UNITY_STANDALONE_WIN || UNITY_EDITOR_WIN || UNITY_STANDALONE_OSX || UNITY_EDITOR_OSX || UNITY_STANDALONE_LINUX || UNITY_EDITOR_LINUX
internal class RuntimeGraphicsInfoInternal
{
    [DllImport("RuntimeGraphicsInfoNative")]
    public static extern ulong GetDedicatedVideoMemory();

    [DllImport("RuntimeGraphicsInfoNative")]
    public static extern ulong GetDedicatedSystemMemory();

    [DllImport("RuntimeGraphicsInfoNative")]
    public static extern ulong GetSharedSystemMemory();
}
#else
internal class RuntimeGraphicsInfoInternal
{
    public static ulong GetDedicatedVideoMemory() { return ulong.MaxValue; }

    public static ulong GetDedicatedSystemMemory() { return ulong.MaxValue; }

    public static ulong GetSharedSystemMemory() { return ulong.MaxValue; }
}
#endif

public class RuntimeGraphicsInfo
{
    /// <summary> Returns the dedicated video memory, or -1 if it can't be determined. </summary>
    public static ulong GetDedicatedVideoMemory() => RuntimeGraphicsInfoInternal.GetDedicatedSystemMemory();

    /// <summary> Returns the dedicated system memory, or -1 if it can't be determined. </summary>
    public static ulong GetDedicatedSystemMemory() => RuntimeGraphicsInfoInternal.GetDedicatedSystemMemory();

    /// <summary> Returns the shared system memory, or -1 if it can't be determined. </summary>
    public static ulong GetSharedSystemMemory() => RuntimeGraphicsInfoInternal.GetSharedSystemMemory();
}