#pragma once
#include <Framework/Entities/WorldService.h>
#include <Framework/Input/MappingStack.h>

namespace FE::Input
{
    struct WorldInputService final : Framework::WorldService
    {
        FE_RTTI("752DB03C-995A-4524-B593-72639A788807");

        void Init(Framework::EntityWorld& world) override;
        void Shutdown(Framework::EntityWorld& world) override;
        void Update(Framework::EntityWorld& world) override;
        void SetBackend(festd::unique_ptr<Core::Backend> backend);
        [[nodiscard]] festd::span<const ResolvedAction> Resolve(MappingStack& stack) const;
        [[nodiscard]] float GetDeltaSeconds() const
        {
            return m_deltaSeconds;
        }
        [[nodiscard]] const Core::Backend& GetBackend() const
        {
            return *m_backend;
        }

    private:
        festd::unique_ptr<Core::Backend> m_backend;
        float m_deltaSeconds = 0;
        uint64_t m_frame = 0;
    };
} // namespace FE::Input
