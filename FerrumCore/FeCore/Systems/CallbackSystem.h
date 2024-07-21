#pragma once
#include <FeCore/ECS/ISystem.h>
#include <functional>

namespace FE::ECS
{
    class CallbackSystem final : public ISystem
    {
    public:
        typedef void (*CreateProc)();
        typedef void (*UpdateProc)(const FrameEventArgs* args);
        typedef void (*DestroyProc)();

        std::function<void()> CreateCallback;
        std::function<void(const FrameEventArgs* args)> UpdateCallback;
        std::function<void()> DestroyCallback;

        FE_CLASS_RTTI(CallbackSystem, "484CC480-1C59-4FF0-B994-09A87C4E18DF");

        ~CallbackSystem() override = default;

        void OnCreate() override;
        void OnUpdate(const FrameEventArgs& args) override;
        void OnDestroy() override;
    };
} // namespace FE::ECS
