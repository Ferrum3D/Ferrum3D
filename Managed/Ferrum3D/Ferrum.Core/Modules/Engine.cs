using System.Runtime.InteropServices;
using Ferrum.Core.Utils;

namespace Ferrum.Core.Modules;

public sealed partial class Engine : IDisposable
{
    public static DynamicLibrary CoreModuleLibrary { get; } = new();
    public static nint Environment { get; private set; }

    public Engine()
    {
        unsafe
        {
            if (sizeof(nint) != sizeof(ulong))
            {
                throw new Exception("The engine can only work on 64 bit systems");
            }
        }

        CoreModuleLibrary.LoadFrom("FeCoreBindings");
        Engine_Construct();
        Environment = Engine_GetEnvironment();
    }

    public void Dispose()
    {
        ReleaseUnmanagedResources();
        GC.SuppressFinalize(this);
    }

    [LibraryImport(Libraries.FerrumCore)]
    private static partial void Engine_Construct();

    [LibraryImport(Libraries.FerrumCore)]
    private static partial void Engine_Destruct();

    [LibraryImport(Libraries.FerrumCore)]
    private static partial nint Engine_GetEnvironment();

    private static void ReleaseUnmanagedResources()
    {
        if (Environment == nint.Zero)
        {
            return;
        }

        Engine_Destruct();
        Environment = nint.Zero;
        CoreModuleLibrary.Dispose();
    }

    ~Engine()
    {
        ReleaseUnmanagedResources();
    }
}
