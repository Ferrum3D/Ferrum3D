using System.Runtime.InteropServices;

namespace Ferrum.Core.Console;

public partial class ConsoleLogger : IDisposable
{
    private static nint handle;

    public ConsoleLogger()
    {
        handle = IConsoleLogger_Construct();
    }

    public void Dispose()
    {
        ReleaseUnmanagedResources();
        GC.SuppressFinalize(this);
    }

    public static void Log(string message, LogMessageType messageType)
    {
        IConsoleLogger_Log(handle, message, (int)messageType);
    }

    public static void LogMessage(string message)
    {
        Log(message, LogMessageType.Message);
    }

    public static void LogWarning(string message)
    {
        Log(message, LogMessageType.Warning);
    }

    public static void LogError(string message)
    {
        Log(message, LogMessageType.Error);
    }

    private static void ReleaseUnmanagedResources()
    {
        IConsoleLogger_Destruct(handle);
        handle = nint.Zero;
    }

    [LibraryImport("FeCoreBindings")]
    private static partial nint IConsoleLogger_Construct();

    [DllImport("FeCoreBindings")]
    private static extern void IConsoleLogger_Destruct(nint self);

    [LibraryImport("FeCoreBindings", StringMarshalling = StringMarshalling.Utf8)]
    private static partial void IConsoleLogger_Log(nint self, string message, int logType);

    ~ConsoleLogger()
    {
        ReleaseUnmanagedResources();
    }
}
