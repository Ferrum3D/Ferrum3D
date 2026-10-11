#pragma once
#include <Framework/Input/Core/Backend.h>
#include <Framework/Input/MappingTable.h>

namespace FE::Input
{
    enum class MappingStackFlags : uint32_t
    {
        kNone = 0,
        kEnabled = 1 << 0,
    };

    FE_ENUM_OPERATORS(MappingStackFlags);


    //! Last enabled table wins for duplicate actions; consuming actions also reserve their physical controls.
    //! Asset handles borrow residency from the caller. Resolve and mutations must run on the same thread.
    //! Results are cached for a frame; stack edits take effect when the next frame is resolved.
    struct MappingStack final
    {
        void PushTable(IO::AssetHandle<MappingTable> table, MappingStackFlags flags = MappingStackFlags::kNone);
        void Enable(const IO::AssetHandle<MappingTable>& table);
        void Disable(const IO::AssetHandle<MappingTable>& table);
        void RemoveTable(const IO::AssetHandle<MappingTable>& table);
        void Clear();
        [[nodiscard]] festd::span<const ResolvedAction> Resolve(const Core::KeyboardState& keyboard,
                                                                const Core::MouseState& mouse, float deltaSeconds,
                                                                uint64_t frame);
        [[nodiscard]] const ResolvedAction* FindAction(IO::AssetID action) const;

    private:
        struct TableEntry final
        {
            IO::AssetHandle<MappingTable> m_table;
            bool m_enabled = false;
        };


        struct RuntimeAction final
        {
            IO::AssetID m_id = IO::AssetID::kNull;
            IO::AssetSlot* m_owner = nullptr;
            uint32_t m_tableGeneration = 0;
            uint32_t m_actionGeneration = 0;
            ActionProcessorContext m_context;
            ResolvedAction m_result;
            bool m_visited = false;
        };

        festd::inline_vector<TableEntry> m_tables;
        festd::inline_vector<RuntimeAction> m_runtime;
        festd::inline_vector<ResolvedAction> m_results;
        uint64_t m_frame = Constants::kMaxU64;

        void SetEnabled(const IO::AssetHandle<MappingTable>& table, bool enabled);
    };
} // namespace FE::Input
