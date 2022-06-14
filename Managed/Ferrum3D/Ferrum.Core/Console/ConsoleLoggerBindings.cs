using System;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace Ferrum.Core.Console
{
    internal static class ConsoleLoggerBindings
    {
        private static IntPtr handle;

        public static void Log(string message, LogMessageType messageType)
        {
            var bytes = Encoding.Unicode.GetBytes(message);
            bytes = Encoding.Convert(Encoding.Unicode, Encoding.UTF8, bytes)
                .Append((byte)0)
                .ToArray();
            unsafe
            {
                fixed (byte* ptr = bytes)
                {
                    LogNative(handle, new IntPtr(ptr), (int)messageType);
                }
            }
        }

        public static void InitLogger()
        {
            handle = InitLoggerNative();
        }

        public static void DeinitLogger()
        {
            DeinitLoggerNative(handle);
            handle = IntPtr.Zero;
        }

        [DllImport("FeCoreBindings", EntryPoint = "InitLogger")]
        private static extern IntPtr InitLoggerNative();

        [DllImport("FeCoreBindings", EntryPoint = "DeinitLogger")]
        private static extern void DeinitLoggerNative(IntPtr self);

        [DllImport("FeCoreBindings", EntryPoint = "ConsoleLogger_Log")]
        private static extern void LogNative(IntPtr self, IntPtr message, int logType);
    }
}
