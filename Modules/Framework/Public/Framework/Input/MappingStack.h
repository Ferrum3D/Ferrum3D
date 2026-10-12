#pragma once
#include <Framework/Input/Core/Backend.h>
#include <Framework/Input/InputFrame.h>
#include <Framework/Input/MappingTable.h>
#include <festd/unordered_map.h>

namespace FE::Input
{
    struct ContextHandle final
    {
        uint64_t m_value = 0;
        bool operator==(const ContextHandle&) const = default;
    };


    //! One world owns one resolver. Equal priorities are ordered by activation, newest first.
    struct MappingStack final
    {
        ContextHandle Activate(IO::AssetHandle<MappingTable> table, int32_t priority = 0, bool modal = false);
        void Deactivate(ContextHandle context);
        void Clear();

        void Resolve(const Core::KeyboardState& keyboard, const Core::MouseState& mouse, float deltaSeconds, uint64_t frame,
                     festd::span<const Core::Sample> samples = {}, bool publishSampleEvents = true);
        [[nodiscard]] const ResolvedAction* FindAction(IO::AssetID action) const;

        [[nodiscard]] festd::span<const ResolvedAction> GetActions() const
        {
            return m_inputFrame.m_actions;
        }

        [[nodiscard]] festd::span<const ActionEvent> GetEvents() const
        {
            return m_inputFrame.m_events;
        }

        [[nodiscard]] const InputFrame& GetFrame() const
        {
            return m_inputFrame;
        }

    private:
        struct CompiledMapping final
        {
            ActionMapping m_mapping;
            uint32_t m_index = 0;
        };

        struct TableEntry final
        {
            IO::AssetHandle<MappingTable> m_table;
            ContextHandle m_handle;
            int32_t m_priority = 0;
            bool m_modal = false;
            uint32_t m_generation = Constants::kMaxU32;
            festd::vector<CompiledMapping> m_compiled;
        };

        struct RuntimeAction final
        {
            IO::AssetID m_id = IO::AssetID::kNull;
            uint64_t m_owner = 0;
            uint32_t m_tableGeneration = 0;
            uint32_t m_actionGeneration = 0;
            ActionProcessorContext m_context;
            Rc<ActionProcessor> m_processor;
            ActionType m_type = ActionType::kBoolean;
            Vector2 m_displacement{ 0, 0 };
            bool m_visited = false;
            bool m_discrete = false;
            bool m_valid = false;
            bool m_triggeredThisFrame = false;
            IO::AssetHandle<Action> m_asset;
        };

        festd::vector<TableEntry> m_tables;
        festd::vector<RuntimeAction> m_runtime;
        InputFrame m_inputFrame;
        uint64_t m_frame = Constants::kMaxU64;

        void Prepare();
        void ProcessSample(const Core::Sample& sample, bool publishEvents);
        void Cancel(uint32_t index);
        uint32_t Bind(IO::AssetID action);
    };
} // namespace FE::Input
