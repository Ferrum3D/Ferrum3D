namespace Ferrum.Core.Console
{
    public static class ConsoleLogger
    {
        public static void Init()
        {
            ConsoleLoggerBindings.InitLogger();
        }

        public static void Deinit()
        {
            ConsoleLoggerBindings.DeinitLogger();
        }

        public static void Log(string message, LogMessageType messageType)
        {
            ConsoleLoggerBindings.Log(message, messageType);
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
    }
}
