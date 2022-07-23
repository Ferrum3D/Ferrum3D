#include <FeCore/Systems/CallbackSystem.h>

namespace FE::ECS
{
    void CallbackSystem::OnCreate()
    {
        if (CreateCallback)
        {
            CreateCallback();
        }
    }

    void CallbackSystem::OnUpdate(const FrameEventArgs& args)
    {
        if (UpdateCallback)
        {
            UpdateCallback(&args);
        }
    }

    void CallbackSystem::OnDestroy()
    {
        if (DestroyCallback)
        {
            DestroyCallback();
        }
    }
} // namespace FE::ECS
