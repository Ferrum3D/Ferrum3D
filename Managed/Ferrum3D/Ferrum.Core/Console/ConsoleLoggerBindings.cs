using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace Ferrum.Core.Console
{
    internal static unsafe class ConsoleLoggerBindings
    {
        private static void* handle;

        public static void Log(string message, LogMessageType messageType)
        {
            var bytes = Encoding.Default.GetBytes(message);
            bytes = Encoding.Convert(Encoding.Default, Encoding.UTF8, bytes)
                .Append((byte)0)
                .ToArray();
            fixed (byte* ptr = bytes)
            {
                LogNative(handle, ptr, (int)messageType);
            }
        }

        public static void InitLogger()
        {
            handle = InitLoggerNative();
        }

        public static void DeinitLogger()
        {
            DeinitLoggerNative(handle);
            handle = (void*)0;
        }

        [DllImport("FeCoreBindings", EntryPoint = "InitLogger")]
        private static extern void* InitLoggerNative();

        [DllImport("FeCoreBindings", EntryPoint = "DeinitLogger")]
        private static extern void DeinitLoggerNative(void* logger);

        [DllImport("FeCoreBindings", EntryPoint = "ConsoleLogger_Log")]
        private static extern void LogNative(void* logger, byte* message, int logType);
    }
}
