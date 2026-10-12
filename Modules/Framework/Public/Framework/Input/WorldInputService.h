#pragma once
#include <Framework/Entities/WorldService.h>
#include <Framework/Input/InputFrame.h>
#include <Framework/Input/MappingStack.h>

namespace FE::Input
{
    struct InputConfiguration final
    {
        FE_RTTI_Reflect("AD95366E-4AA3-46C4-BB93-63FAFFFF3142");
        FE_RTTI_Serialize();

        festd::vector<ContextDefinition> m_contexts;
        [[nodiscard]] bool Validate() const;
    };


    struct WorldInputService final : Framework::WorldService
    {
        FE_RTTI("752DB03C-995A-4524-B593-72639A788807");

        bool Configure(const Rtti::Any& settings) override;
        Rtti::Any CaptureConfiguration() const override;

        void Init(Framework::EntityWorld& world) override;
        void Shutdown(Framework::EntityWorld& world) override;
        void Update(Framework::EntityWorld& world) override;
        void SetBackend(festd::unique_ptr<Core::Backend> backend);

        [[nodiscard]] ContextHandle Activate(const IO::Link<MappingTable>& table, int32_t priority = 0, bool modal = false);
        void Deactivate(ContextHandle context);

        [[nodiscard]] festd::vector<ContextDefinition> GetContexts() const;

        template<class T>
        [[nodiscard]] ActionHandle<T> Bind(const IO::Link<Action>& action)
        {
            return { BindInternal(action, kActionType<T>), m_inputFrame.m_generation };
        }

        [[nodiscard]] const InputFrame& GetFrame() const
        {
            return m_inputFrame;
        }

        [[nodiscard]] float GetDeltaSeconds() const
        {
            return m_deltaSeconds;
        }

        [[nodiscard]] const Core::Backend& GetBackend() const;

    private:
        struct Context final
        {
            ContextHandle m_handle;
            ContextDefinition m_definition;
            IO::AssetRequest m_residency;
        };

        InputConfiguration m_configuration;
        festd::unique_ptr<Core::Backend> m_backend;
        MappingStack m_stack;
        InputFrame m_inputFrame;
        festd::vector<Context> m_contexts;
        float m_deltaSeconds = 0;
        uint64_t m_lastSampleSequence = 0;
        uint32_t BindInternal(const IO::Link<Action>& action, ActionType type);
    };
} // namespace FE::Input
