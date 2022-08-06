using System;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using Ferrum.Core.Containers;

namespace Ferrum.Core.Console
{
    public class ConsoleLogger : IDisposable
    {
        private static IntPtr handle;

        public ConsoleLogger()
        {
            handle = ConstructNative();
        }

        public void Dispose()
        {
            ReleaseUnmanagedResources();
            GC.SuppressFinalize(this);
        }

        public static void Log(string message, LogMessageType messageType)
        {
            var s = new NativeString(message);
            LogNative(handle, s.DataPointer, (int)messageType);
            s.Dispose();
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
            DestructNative(handle);
            handle = IntPtr.Zero;
        }

        [DllImport("FeCoreBindings", EntryPoint = "IConsoleLogger_Construct")]
        private static extern IntPtr ConstructNative();

        [DllImport("FeCoreBindings", EntryPoint = "IConsoleLogger_Destruct")]
        private static extern void DestructNative(IntPtr self);

        [DllImport("FeCoreBindings", EntryPoint = "IConsoleLogger_Log")]
        private static extern void LogNative(IntPtr self, IntPtr message, int logType);

        ~ConsoleLogger()
        {
            ReleaseUnmanagedResources();
        }
    }
}
