namespace Ferrum.Core.Console
{
    public enum LogMessageType
    {
        None = 0,
        Message = 1 << 0,
        Warning = 1 << 1,
        Error = 1 << 2,
        All = Message | Warning | Error
    }
}
