#include <FeCore/EventBus/EventBus.h>
#include <FeCore/EventBus/FrameEvents.h>

namespace FE::ECS
{
    extern "C"
    {
        FE_DLL_EXPORT EventBus<FrameEvents>* EventBus_FrameEvents_Construct()
        {
            return MakeShared<EventBus<FrameEvents>>().Detach();
        }

        FE_DLL_EXPORT void EventBus_FrameEvents_SendEvent_OnFrameStart(FrameEventArgs* args)
        {
            EventBus<FrameEvents>::SendEvent(&FrameEvents::OnFrameStart, *args);
        }

        FE_DLL_EXPORT void EventBus_FrameEvents_SendEvent_OnUpdate(FrameEventArgs* args)
        {
            EventBus<FrameEvents>::SendEvent(&FrameEvents::OnUpdate, *args);
        }

        FE_DLL_EXPORT void EventBus_FrameEvents_SendEvent_OnLateUpdate(FrameEventArgs* args)
        {
            EventBus<FrameEvents>::SendEvent(&FrameEvents::OnLateUpdate, *args);
        }

        FE_DLL_EXPORT void EventBus_FrameEvents_SendEvent_OnFrameEnd(FrameEventArgs* args)
        {
            EventBus<FrameEvents>::SendEvent(&FrameEvents::OnFrameEnd, *args);
        }

        FE_DLL_EXPORT void EventBus_FrameEvents_Destruct(EventBus<FrameEvents>* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::ECS
