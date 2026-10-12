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

        using Configuration = InputConfiguration;
        bool Configure(const Configuration& settings);
        Configuration CaptureConfiguration() const;

        void Init(Framework::EntityWorld& world) override;
        void Shutdown(Framework::EntityWorld& world) override;
        void Update(Framework::EntityWorld& world) override;
        void SetBackend(festd::unique_ptr<Core::Backend> backend);

        [[nodiscard]] ContextHandle Activate(const IO::Link<MappingTable>& table, int32_t priority = 0, bool modal = false);
        void Deactivate(ContextHandle context);

        [[nodiscard]] festd::vector<ContextDefinition> GetContexts() const;

        [[nodiscard]] const InputFrame& GetFrame() const
        {
            return m_stack.GetFrame();
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

        festd::unique_ptr<Core::Backend> m_backend;
        MappingStack m_stack;
        uint64_t m_updateNumber = 0;
        festd::vector<Context> m_contexts;
        float m_deltaSeconds = 0;
        uint64_t m_lastSampleSequence = 0;
    };
} // namespace FE::Input
