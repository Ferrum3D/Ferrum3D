#include <Framework/Input/MappingStack.h>
#include <cmath>

namespace FE::Input
{
    namespace
    {
        struct Controls final
        {
            festd::bitset<256> m_keys;
            festd::bitset<5> m_buttons;
            bool m_deltaX = false;
            bool m_deltaY = false;
            bool m_wheel = false;
        };


        struct MappingValue final
        {
            Vector2 m_value{ 0, 0 };
            bool m_pressed = false;
            Controls m_controls;
        };


        MappingValue Evaluate(const ActionMapping& mapping, const Core::KeyboardState& keyboard, const Core::MouseState& mouse,
                              const Controls& consumed)
        {
            MappingValue result;
            if (mapping.m_mouseButtonGate >= 0)
            {
                const uint32_t gate = static_cast<uint32_t>(mapping.m_mouseButtonGate);
                if (gate >= 5 || consumed.m_buttons.test(gate) || !mouse.m_buttonDownMap.test(gate))
                    return result;

                result.m_controls.m_buttons.set(gate);
            }

            uint32_t activeCount = 0;
            const uint32_t count = mapping.m_keyboard.size() + mapping.m_mouse.size();
            const auto addValue = [&result, &activeCount, &mapping](const Vector2 value, const bool active) {
                if (!active)
                    return;

                ++activeCount;
                if (mapping.m_operation != MappingOperation::kAny || activeCount == 1)
                    result.m_value = result.m_value + value;
            };

            for (const KeyboardMapping& binding : mapping.m_keyboard)
            {
                const uint32_t key = static_cast<uint32_t>(binding.m_key);
                if (key == 0 || key >= 256 || consumed.m_keys.test(key))
                    continue;

                const bool active = keyboard.m_keyDownMap.test(key) || keyboard.m_keyPressMap.test(key);
                addValue(binding.m_value, active);
                result.m_pressed |= keyboard.m_keyPressMap.test(key);
                if (active)
                    result.m_controls.m_keys.set(key);
            }

            for (const MouseMapping& binding : mapping.m_mouse)
            {
                float value = 0;
                switch (binding.m_source)
                {
                case MouseSource::kButton:
                    if (binding.m_button < 5 && !consumed.m_buttons.test(binding.m_button))
                    {
                        value = mouse.m_buttonDownMap.test(binding.m_button) || mouse.m_buttonPressMap.test(binding.m_button)
                            ? 1.0f
                            : 0.0f;
                        result.m_pressed |= mouse.m_buttonPressMap.test(binding.m_button);
                        if (value != 0)
                            result.m_controls.m_buttons.set(binding.m_button);
                    }
                    break;

                case MouseSource::kDeltaX:
                    value = consumed.m_deltaX ? 0 : mouse.m_delta.x;
                    result.m_controls.m_deltaX |= value != 0;
                    result.m_pressed |= value != 0;
                    break;

                case MouseSource::kDeltaY:
                    value = consumed.m_deltaY ? 0 : mouse.m_delta.y;
                    result.m_controls.m_deltaY |= value != 0;
                    result.m_pressed |= value != 0;
                    break;

                case MouseSource::kWheel:
                    value = consumed.m_wheel ? 0 : mouse.m_wheelDelta;
                    result.m_controls.m_wheel |= value != 0;
                    result.m_pressed |= value != 0;
                    break;

                default:
                    break;
                }

                addValue(binding.m_value * value, value != 0);
            }

            if (mapping.m_operation == MappingOperation::kAll && (count == 0 || activeCount != count))
                return {};

            const float length = std::sqrt(result.m_value.x * result.m_value.x + result.m_value.y * result.m_value.y);
            if ((mapping.m_flags & MappingFlags::kNormalizeDirection) != MappingFlags::kNone && length > 1)
                result.m_value = result.m_value / length;

            return result;
        }


        void Consume(Controls& consumed, const Controls& used)
        {
            consumed.m_keys |= used.m_keys;
            consumed.m_buttons |= used.m_buttons;
            consumed.m_deltaX |= used.m_deltaX;
            consumed.m_deltaY |= used.m_deltaY;
            consumed.m_wheel |= used.m_wheel;
        }
    } // namespace


    void MappingStack::PushTable(IO::AssetHandle<MappingTable> table, const MappingStackFlags flags)
    {
        if (!table.IsValid())
            return;

        RemoveTable(table);
        m_tables.push_back({ std::move(table), (flags & MappingStackFlags::kEnabled) != MappingStackFlags::kNone });
    }


    void MappingStack::SetEnabled(const IO::AssetHandle<MappingTable>& table, const bool enabled)
    {
        for (TableEntry& entry : m_tables)
        {
            if (entry.m_table.GetAssetSlot() == table.GetAssetSlot() && entry.m_enabled != enabled)
            {
                entry.m_enabled = enabled;
            }
        }
    }


    void MappingStack::Enable(const IO::AssetHandle<MappingTable>& table)
    {
        SetEnabled(table, true);
    }


    void MappingStack::Disable(const IO::AssetHandle<MappingTable>& table)
    {
        SetEnabled(table, false);
    }


    void MappingStack::RemoveTable(const IO::AssetHandle<MappingTable>& table)
    {
        for (auto it = m_tables.begin(); it != m_tables.end(); ++it)
        {
            if (it->m_table.GetAssetSlot() == table.GetAssetSlot())
            {
                m_tables.erase(it);
                return;
            }
        }
    }


    void MappingStack::Clear()
    {
        m_tables.clear();
        m_runtime.clear();
        m_results.clear();
        m_frame = Constants::kMaxU64;
    }


    festd::span<const ResolvedAction> MappingStack::Resolve(const Core::KeyboardState& keyboard, const Core::MouseState& mouse,
                                                            const float deltaSeconds, const uint64_t frame)
    {
        FE_Assert(std::isfinite(deltaSeconds) && deltaSeconds >= 0);
        if (m_frame == frame)
            return { m_results.data(), m_results.size() };

        m_frame = frame;
        m_results.clear();
        for (RuntimeAction& runtime : m_runtime)
            runtime.m_visited = false;

        Controls consumed;
        for (auto it = m_tables.rbegin(); it != m_tables.rend(); ++it)
        {
            if (!it->m_enabled)
                continue;

            const auto table = it->m_table.Read();
            if (!table)
                continue;

            Controls tableConsumed;
            for (const ActionMapping& mapping : table->m_mappings)
            {
                const IO::AssetID id = mapping.m_action.GetAssetID();
                RuntimeAction* runtime = nullptr;
                for (RuntimeAction& candidate : m_runtime)
                {
                    if (candidate.m_id == id)
                        runtime = &candidate;
                }

                if (runtime != nullptr && runtime->m_visited)
                    continue;

                const auto handle = mapping.m_action.GetAssetHandle();
                const auto action = handle.Read();
                if (!action)
                    continue;

                if (runtime == nullptr)
                {
                    m_runtime.push_back({});
                    runtime = &m_runtime.back();
                    runtime->m_id = id;
                }

                const bool changed = runtime->m_owner != it->m_table.GetAssetSlot()
                    || runtime->m_tableGeneration != it->m_table.GetGeneration()
                    || runtime->m_actionGeneration != handle.GetGeneration();
                if (changed)
                    runtime->m_context = {};

                runtime->m_owner = it->m_table.GetAssetSlot();
                runtime->m_tableGeneration = it->m_table.GetGeneration();
                runtime->m_actionGeneration = handle.GetGeneration();
                runtime->m_visited = true;

                const MappingValue value = Evaluate(mapping, keyboard, mouse, consumed);
                auto& context = runtime->m_context;
                const bool wasCompleted = context.m_completed;
                context.m_active = value.m_value.x != 0 || value.m_value.y != 0;
                context.m_pressed = value.m_pressed;
                context.m_deltaSeconds = deltaSeconds;

                ResolvedAction result;
                result.m_action = id;
                result.m_type = action->m_type;
                result.m_value = value.m_value;
                if (action->m_type == ActionType::kBoolean)
                    result.m_value = { context.m_active ? 1.0f : 0.0f, 0 };
                else if (action->m_type == ActionType::kScalar)
                    result.m_value.y = 0;

                context.m_active = result.m_value.x != 0 || result.m_value.y != 0;
                result.m_active = context.m_active;
                if (action->m_processor)
                {
                    result.m_state = action->m_processor->Process(context);
                    result.m_hasEvent = (context.m_active || context.m_wasActive) && !wasCompleted;
                }
                else
                {
                    result.m_state = ActionState::kFinished;
                    result.m_hasEvent = context.m_pressed && context.m_active;
                }

                context.m_wasActive = context.m_active;
                if (!context.m_active)
                {
                    context.m_heldSeconds = 0;
                    context.m_completed = false;
                }

                runtime->m_result = result;
                m_results.push_back(result);
                if (context.m_active && (action->m_flags & ActionFlags::kConsume) != ActionFlags::kNone)
                    Consume(tableConsumed, value.m_controls);
            }

            Consume(consumed, tableConsumed);
        }

        for (auto it = m_runtime.begin(); it != m_runtime.end();)
        {
            if (it->m_visited)
            {
                ++it;
                continue;
            }

            if (it->m_context.m_wasActive && !it->m_context.m_completed)
            {
                ResolvedAction canceled = it->m_result;
                canceled.m_value = { 0, 0 };
                canceled.m_active = false;
                canceled.m_state = ActionState::kCanceled;
                canceled.m_hasEvent = true;
                m_results.push_back(canceled);
            }

            it = m_runtime.erase(it);
        }

        return { m_results.data(), m_results.size() };
    }


    const ResolvedAction* MappingStack::FindAction(const IO::AssetID action) const
    {
        for (const ResolvedAction& result : m_results)
        {
            if (result.m_action == action)
                return &result;
        }

        return nullptr;
    }
} // namespace FE::Input
