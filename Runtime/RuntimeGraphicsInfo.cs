using System;
using System.Runtime.InteropServices;

#if UNITY_STANDALONE_WIN || UNITY_EDITOR_WIN || UNITY_STANDALONE_OSX || UNITY_EDITOR_OSX
internal class RuntimeGraphicsInfoInternal
{
    [DllImport("RuntimeGraphicsInfoNative")]
    public static extern ulong GetDedicatedVideoMemory();

    [DllImport("RuntimeGraphicsInfoNative")]
    public static extern ulong GetDedicatedSystemMemory();

    [DllImport("RuntimeGraphicsInfoNative")]
    public static extern ulong GetSharedSystemMemory();

    [DllImport("RuntimeGraphicsInfoNative")]
    public static extern ushort GetMaxTessellationFactor();

    [DllImport("RuntimeGraphicsInfoNative")]
    public static extern ushort SetMaxTessellationFactor(ushort factor);
}
#else
internal class RuntimeGraphicsInfoInternal
{
    public static ulong GetDedicatedVideoMemory() { return ulong.MaxValue; }

    public static ulong GetDedicatedSystemMemory() { return ulong.MaxValue; }

    public static ulong GetSharedSystemMemory() { return ulong.MaxValue; }

    public static ushort GetMaxTessellationFactor() { return 64; }

    public static ushort SetMaxTessellationFactor(ushort factor) { return 64; }
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

    /// <summary> Returns the max tessellation factor supported/activated in the Graphics API. </summary>
    public static ushort GetMaxTessellationFactor() => RuntimeGraphicsInfoInternal.GetMaxTessellationFactor();

    /// <summary> Tries to set the max tessellation factor (if supported by Graphics API), returns value after modification (it may be less than or greater than requested). </summary>
    public static uint SetMaxTessellationFactor(uint factor) => RuntimeGraphicsInfoInternal.GetMaxTessellationFactor();
}