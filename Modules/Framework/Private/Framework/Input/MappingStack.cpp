#include <Core/Logging/Logger.h>
#include <Framework/Input/MappingStack.h>
#include <atomic>

namespace FE::Input
{
    namespace
    {
        std::atomic<uint64_t> GNextContext = 0;


        struct Controls final
        {
            festd::bitset<256> m_keys;
            festd::bitset<5> m_buttons;
            bool m_motion = false;
            bool m_wheel = false;

            void Merge(const Controls& other)
            {
                m_keys |= other.m_keys;
                m_buttons |= other.m_buttons;
                m_motion |= other.m_motion;
                m_wheel |= other.m_wheel;
            }
        };


        struct MappingValue final
        {
            Vector2 m_value{ 0, 0 };
            Vector2 m_actuation{ 0, 0 };
            Vector2 m_direction{ 0, 0 };
            Controls m_controls;
            bool m_participates = false;
        };


        bool IsHeld(const Control& control, const Core::Sample& sample, const Controls& consumed)
        {
            if (control.m_code < 256)
            {
                const uint32_t key = control.m_code;
                return key > 0 && key < 256 && !consumed.m_keys.test(key) && sample.m_keyboard.m_keyDownMap.test(key);
            }

            const uint32_t button = control.m_code - 256;
            return button < 5 && !consumed.m_buttons.test(button) && sample.m_mouse.m_buttonDownMap.test(button);
        }


        void Use(const Control& control, Controls& used)
        {
            if (control.m_code < 256)
                used.m_keys.set(control.m_code);
            else
                used.m_buttons.set(control.m_code - 256);
        }


        MappingValue EvaluateBinding(const Binding& binding, const Core::Sample& sample, const Controls& consumed)
        {
            FE_PROFILER_ZONE();

            MappingValue result;
            for (const Control& condition : binding.m_conditions)
            {
                if (!IsHeld(condition, sample, consumed))
                    return {};
            }

            const auto readControl = [&](const Control control) {
                const bool held = IsHeld(control, sample, consumed);
                if (held)
                {
                    Use(control, result.m_controls);
                    result.m_participates = true;
                }

                return held ? 1.0f : 0.0f;
            };

            festd::visit(
                [&]<class T>(const T& source) {
                    if constexpr (std::same_as<T, ButtonBinding>)
                    {
                        result.m_direction = Vector2(readControl(source.m_control));
                    }
                    else if constexpr (std::same_as<T, Axis1DBinding>)
                    {
                        result.m_direction.x = readControl(source.m_positive) - readControl(source.m_negative);
                    }
                    else if constexpr (std::same_as<T, Axis2DBinding>)
                    {
                        result.m_direction = { readControl(source.m_right) - readControl(source.m_left),
                                               readControl(source.m_forward) - readControl(source.m_back) };
                    }
                    else if constexpr (std::same_as<T, ChordBinding>)
                    {
                        for (const Control& control : source.m_controls)
                        {
                            if (!IsHeld(control, sample, consumed))
                                return;
                        }

                        for (const Control& control : source.m_controls)
                            Use(control, result.m_controls);

                        result.m_participates = true;
                        result.m_direction = { 1, 1 };
                    }
                    else if constexpr (std::same_as<T, MouseDeltaBinding>)
                    {
                        result.m_direction = consumed.m_motion ? Vector2::kZero : sample.m_mouse.m_delta;
                        result.m_participates = result.m_controls.m_motion = result.m_direction != Vector2::kZero;
                    }
                    else
                    {
                        result.m_direction.x = consumed.m_wheel ? 0 : sample.m_mouse.m_wheelDelta;
                        result.m_participates = result.m_controls.m_wheel = result.m_direction.x != 0;
                    }
                },
                binding.m_source);

            if (!result.m_participates)
                return {};

            for (const Control& condition : binding.m_conditions)
                Use(condition, result.m_controls);

            result.m_direction = result.m_direction * binding.m_scale;
            result.m_actuation = result.m_direction * binding.m_sensitivity;
            return result;
        }


        MappingValue Evaluate(const ActionMapping& mapping, const ActionType type, const Core::Sample& sample,
                              const Controls& consumed)
        {
            FE_PROFILER_ZONE();

            MappingValue result;
            for (const Binding& binding : mapping.m_bindings)
            {
                const MappingValue value = EvaluateBinding(binding, sample, consumed);
                if (mapping.m_operation == MappingOperation::kSum)
                {
                    result.m_actuation = result.m_actuation + value.m_actuation;
                    result.m_direction = result.m_direction + value.m_direction;
                    result.m_controls.Merge(value.m_controls);
                    result.m_participates |= value.m_participates;
                }
                else if (value.m_participates)
                {
                    const bool stronger = Math::LengthSquared(value.m_actuation) > Math::LengthSquared(result.m_actuation);
                    if (!result.m_participates || stronger)
                        result = value;
                }
            }

            if (mapping.m_clampMagnitude && Math::LengthSquared(result.m_direction) > 1)
                result.m_actuation = result.m_actuation / Math::Length(result.m_direction);

            result.m_value = result.m_actuation;
            if (mapping.m_bindings.front().m_integrate)
                result.m_value = result.m_value * sample.m_deltaSeconds;

            if (type == ActionType::kBoolean)
            {
                result.m_value = { result.m_value != Vector2::kZero ? 1.0f : 0.0f, 0 };
                result.m_actuation = { result.m_actuation != Vector2::kZero ? 1.0f : 0.0f, 0 };
            }
            else if (type == ActionType::kScalar)
            {
                result.m_value.y = 0;
                result.m_actuation.y = 0;
            }

            return result;
        }
    } // namespace


    ContextHandle MappingStack::Activate(IO::AssetHandle<MappingTable> table, const int32_t priority, const bool modal)
    {
        FE_PROFILER_ZONE();

        FE_Assert(table.IsValid());

        const ContextHandle handle{ ++GNextContext };

        TableEntry entry;
        entry.m_table = std::move(table);
        entry.m_handle = handle;
        entry.m_priority = priority;
        entry.m_modal = modal;
        m_tables.push_back(std::move(entry));
        festd::sort(m_tables.begin(), m_tables.end(), [](const TableEntry& lhs, const TableEntry& rhs) {
            return lhs.m_priority == rhs.m_priority ? lhs.m_handle.m_value > rhs.m_handle.m_value
                                                    : lhs.m_priority > rhs.m_priority;
        });
        return handle;
    }


    void MappingStack::Deactivate(const ContextHandle context)
    {
        FE_PROFILER_ZONE();

        festd::erase_if(m_tables, [context](const TableEntry& entry) {
            return entry.m_handle == context;
        });
    }


    void MappingStack::Clear()
    {
        FE_PROFILER_ZONE();

        m_tables.clear();
        m_runtime.clear();
        m_inputFrame.m_indices.clear();
        m_inputFrame.m_actions.clear();
        m_inputFrame.m_events.clear();
        m_frame = Constants::kMaxU64;
    }


    uint32_t MappingStack::Bind(const IO::AssetID action)
    {
        FE_PROFILER_ZONE();

        const auto found = m_inputFrame.m_indices.find(action);
        if (found != m_inputFrame.m_indices.end())
            return found->second;

        const uint32_t index = m_runtime.size();
        RuntimeAction runtime;
        runtime.m_id = action;
        runtime.m_asset = IO::Link<Action>(action).GetAssetHandle();
        m_runtime.push_back(std::move(runtime));
        m_inputFrame.m_indices.emplace(action, index);
        m_inputFrame.m_actions.push_back({});
        m_inputFrame.m_actions.back().m_action = action;
        return index;
    }


    void MappingStack::Cancel(const uint32_t index)
    {
        FE_PROFILER_ZONE();

        RuntimeAction& runtime = m_runtime[index];
        if (runtime.m_context.m_wasActive)
            m_inputFrame.m_events.push_back({ runtime.m_id, ActionState::kCanceled, Vector2::kZero });

        runtime.m_context = {};
        runtime.m_processor.Reset();
        runtime.m_owner = 0;
    }


    void MappingStack::Prepare()
    {
        FE_PROFILER_ZONE();

        for (RuntimeAction& runtime : m_runtime)
        {
            runtime.m_visited = false;
            runtime.m_displacement = { 0, 0 };
            runtime.m_triggeredThisFrame = false;
        }

        for (TableEntry& entry : m_tables)
        {
            const auto table = entry.m_table.Read();
            if (!table)
            {
                if (entry.m_modal)
                    break;

                continue;
            }

            if (entry.m_generation != entry.m_table.GetGeneration())
            {
                entry.m_compiled.clear();
                entry.m_generation = entry.m_table.GetGeneration();
                if (!table->Validate())
                {
                    Logger::LogError("Invalid input mapping table {}", entry.m_table.GetAssetSlot()->m_assetId);
                    if (entry.m_modal)
                        break;

                    continue;
                }

                for (const ActionMapping& mapping : table->m_mappings)
                    entry.m_compiled.push_back({ mapping, Bind(mapping.m_action.GetAssetID()) });
            }

            for (CompiledMapping& compiled : entry.m_compiled)
            {
                compiled.m_index = Bind(compiled.m_mapping.m_action.GetAssetID());
                RuntimeAction& runtime = m_runtime[compiled.m_index];
                if (runtime.m_visited)
                    continue;

                runtime.m_visited = true;
                if (!runtime.m_asset.IsValid())
                    runtime.m_asset = compiled.m_mapping.m_action.GetAssetHandle();

                const bool ownerChanged = runtime.m_owner != entry.m_handle.m_value;
                const bool tableChanged = runtime.m_tableGeneration != entry.m_generation;
                const bool actionChanged = runtime.m_actionGeneration != runtime.m_asset.GetGeneration();
                if (!ownerChanged && !tableChanged && !actionChanged)
                    continue;

                Cancel(compiled.m_index);
                const auto action = runtime.m_asset.Read();
                if (!action)
                    continue;

                runtime.m_owner = entry.m_handle.m_value;
                runtime.m_tableGeneration = entry.m_generation;
                runtime.m_actionGeneration = runtime.m_asset.GetGeneration();
                runtime.m_type = action->m_type;
                runtime.m_processor = action->m_processor;
                runtime.m_discrete = compiled.m_mapping.m_bindings.front().IsDisplacement();
                runtime.m_valid = action->Validate();
                for (uint32_t bindingIndex = 0; bindingIndex < compiled.m_mapping.m_bindings.size(); ++bindingIndex)
                {
                    if (!compiled.m_mapping.m_bindings[bindingIndex].Supports(action->m_type))
                    {
                        runtime.m_valid = false;
                        Logger::LogError("Input action {} binding {} is incompatible with the action value type",
                                         runtime.m_id,
                                         bindingIndex);
                    }
                }
                if (!runtime.m_valid)
                    Logger::LogError("Input action {} failed preparation", runtime.m_id);
            }

            if (entry.m_modal)
                break;
        }

        for (uint32_t i = 0; i < m_runtime.size(); ++i)
        {
            if (!m_runtime[i].m_visited)
                Cancel(i);
        }

        const uint32_t removed = festd::erase_if(m_runtime, [](const RuntimeAction& runtime) {
            return !runtime.m_visited;
        });
        if (removed != 0)
        {
            m_inputFrame.m_indices.clear();
            for (uint32_t i = 0; i < m_runtime.size(); ++i)
                m_inputFrame.m_indices.emplace(m_runtime[i].m_id, i);
            m_inputFrame.m_actions.resize(m_runtime.size());
            for (TableEntry& entry : m_tables)
            {
                for (CompiledMapping& compiled : entry.m_compiled)
                {
                    const auto found = m_inputFrame.m_indices.find(compiled.m_mapping.m_action.GetAssetID());
                    compiled.m_index = found == m_inputFrame.m_indices.end() ? Constants::kMaxU32 : found->second;
                }
            }
        }

        for (uint32_t i = 0; i < m_runtime.size(); ++i)
        {
            m_inputFrame.m_actions[i] = {};
            m_inputFrame.m_actions[i].m_action = m_runtime[i].m_id;
            m_inputFrame.m_actions[i].m_type = m_runtime[i].m_type;
        }
    }


    void MappingStack::ProcessSample(const Core::Sample& sample, const bool publishEvents)
    {
        FE_PROFILER_ZONE();

        Controls consumed;
        for (const TableEntry& entry : m_tables)
        {
            Controls tableConsumed;
            for (const CompiledMapping& compiled : entry.m_compiled)
            {
                if (compiled.m_index == Constants::kMaxU32)
                    continue;

                RuntimeAction& runtime = m_runtime[compiled.m_index];
                if (runtime.m_owner != entry.m_handle.m_value || !runtime.m_valid)
                    continue;

                const MappingValue value = Evaluate(compiled.m_mapping, runtime.m_type, sample, consumed);

                auto& context = runtime.m_context;
                context.m_active = value.m_actuation != Vector2::kZero;
                context.m_pressed = context.m_active && !context.m_wasActive;
                context.m_deltaSeconds = sample.m_deltaSeconds;

                const bool started = context.m_pressed;
                const bool ended = !context.m_active && context.m_wasActive;
                const bool wasCompleted = context.m_completed;
                auto emit = [&](const ActionState state) {
                    if (publishEvents)
                        m_inputFrame.m_events.push_back({ runtime.m_id, state, value.m_value });
                };
                if (started)
                    emit(ActionState::kStarted);

                const ActionState state = runtime.m_processor ? runtime.m_processor->Process(context) : ActionState::kTriggered;
                const bool triggered = state == ActionState::kTriggered && context.m_active;
                const bool canTrigger = !wasCompleted || !context.m_completed;
                if (triggered && canTrigger)
                {
                    const bool continuing = runtime.m_processor && sample.m_deltaSeconds > 0;
                    if (started || (continuing && !runtime.m_triggeredThisFrame))
                    {
                        emit(ActionState::kTriggered);
                        runtime.m_triggeredThisFrame = true;
                    }
                }
                else if (ended)
                {
                    emit(runtime.m_processor ? state : ActionState::kCompleted);
                }

                if (runtime.m_discrete)
                    runtime.m_displacement = runtime.m_displacement + value.m_value;

                ResolvedAction& result = m_inputFrame.m_actions[compiled.m_index];
                result.m_state = state;
                result.m_value = runtime.m_discrete ? runtime.m_displacement : value.m_value;
                result.m_active = result.m_value != Vector2::kZero;
                context.m_wasActive = context.m_active;
                if (!context.m_active)
                {
                    context.m_heldSeconds = 0;
                    context.m_completed = false;
                }

                if (compiled.m_mapping.m_consume)
                    tableConsumed.Merge(value.m_controls);
            }

            consumed.Merge(tableConsumed);
            if (entry.m_modal)
                break;
        }
    }


    void MappingStack::Resolve(const Core::KeyboardState& keyboard, const Core::MouseState& mouse, const float deltaSeconds,
                               const uint64_t frame, const festd::span<const Core::Sample> samples,
                               const bool publishSampleEvents)
    {
        FE_PROFILER_ZONE();

        FE_Assert(deltaSeconds >= 0);
        if (m_frame == frame)
            return;

        m_frame = frame;
        m_inputFrame.m_number = frame;
        m_inputFrame.m_events.clear();
        Prepare();
        if (samples.empty())
        {
            ProcessSample({ keyboard, mouse, deltaSeconds }, publishSampleEvents);
        }
        else
        {
            for (const Core::Sample& sample : samples)
                ProcessSample(sample, publishSampleEvents);
        }

        for (const ActionEvent& event : m_inputFrame.m_events)
        {
            const auto found = m_inputFrame.m_indices.find(event.m_action);
            if (found == m_inputFrame.m_indices.end())
                continue;

            const uint32_t index = found->second;
            m_inputFrame.m_actions[index].m_state = event.m_state;
            m_inputFrame.m_actions[index].m_hasEvent = true;
        }
    }


    const ResolvedAction* MappingStack::FindAction(const IO::AssetID action) const
    {
        FE_PROFILER_ZONE();

        const auto found = m_inputFrame.m_indices.find(action);
        return found == m_inputFrame.m_indices.end() ? nullptr : &m_inputFrame.m_actions[found->second];
    }
} // namespace FE::Input
