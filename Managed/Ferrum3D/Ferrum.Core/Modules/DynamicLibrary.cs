using System.Diagnostics;
using System.Runtime.InteropServices;
using Ferrum.Core.Utils;

namespace Ferrum.Core.Modules;

public sealed class DynamicLibrary : IDisposable
{
    public string? Name { get; private set; }
    public nint Handle { get; private set; }

    public static DynamicLibrary FromPath(string name)
    {
        var result = new DynamicLibrary();
        result.LoadFrom(name);
        return result;
    }

    public void LoadFrom(string name)
    {
        Name = name;
        Handle = LoadLibrary(name);
    }

    public void Dispose()
    {
        if (Handle == nint.Zero)
        {
            return;
        }

        FreeLibrary(Handle);
        Handle = nint.Zero;
    }

    public Delegate GetFunction(string name, Type delegateType)
    {
        var address = GetProcAddress(Handle, name);
        return Marshal.GetDelegateForFunctionPointer(address, delegateType);
    }

    public T GetFunction<T>(string name)
        where T : Delegate
    {
        var address = GetProcAddress(Handle, name);
        return Marshal.GetDelegateForFunctionPointer<T>(address);
    }

    public static nint GetLoadedModule(string moduleName)
    {
        moduleName = moduleName.ToLower();
        var mods = Process.GetCurrentProcess().Modules;
        for (var i = 0; i < mods.Count; i++)
        {
            if (mods[i].ModuleName.ToLower() == moduleName)
            {
                return mods[i].BaseAddress;
            }
        }

        return nint.Zero;
    }

    [DllImport(Libraries.Kernel32)]
    private static extern bool FreeLibrary(nint handle);

    [DllImport(Libraries.Kernel32)]
    private static extern nint LoadLibrary(string path);

    [DllImport(Libraries.Kernel32)]
    private static extern nint GetProcAddress(nint handle, string name);
}
