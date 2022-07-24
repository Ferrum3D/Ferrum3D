using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Core.EventBus
{
    public class FrameEventBus : EventBusBase
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void EventDelegateNative(in FrameEventArgs eventArgs);

        public static EventDelegateNative OnFrameStart { get; private set; }
        public static EventDelegateNative OnUpdate { get; private set; }
        public static EventDelegateNative OnLateUpdate { get; private set; }
        public static EventDelegateNative OnFrameEnd { get; private set; }

        public FrameEventBus() : base("FrameEvents")
        {
            OnFrameStart = GetSendEventEntryPoint<EventDelegateNative>(nameof(OnFrameStart));
            OnUpdate = GetSendEventEntryPoint<EventDelegateNative>(nameof(OnUpdate));
            OnLateUpdate = GetSendEventEntryPoint<EventDelegateNative>(nameof(OnLateUpdate));
            OnFrameEnd = GetSendEventEntryPoint<EventDelegateNative>(nameof(OnFrameEnd));
        }
    }
}
