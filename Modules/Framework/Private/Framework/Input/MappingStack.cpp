#include <Core/Logging/Logger.h>
#include <Framework/Input/MappingStack.h>
#include <algorithm>
#include <atomic>
#include <cmath>

namespace FE::Input
{
    namespace
    {
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
            Controls m_controls;
        };


        bool Held(const Control& control, const Core::Sample& sample, const Controls& consumed)
        {
            if (control.m_kind == ControlKind::kKey)
            {
                const uint32_t key = static_cast<uint32_t>(control.m_key);
                return key > 0 && key < 256 && !consumed.m_keys.test(key) && sample.m_keyboard.m_keyDownMap.test(key);
            }

            return control.m_button < 5 && !consumed.m_buttons.test(control.m_button)
                && sample.m_mouse.m_buttonDownMap.test(control.m_button);
        }


        void Use(const Control& control, Controls& used)
        {
            if (control.m_kind == ControlKind::kKey)
                used.m_keys.set(static_cast<uint32_t>(control.m_key));
            else
                used.m_buttons.set(control.m_button);
        }


        MappingValue EvaluateBinding(const Binding& binding, const Core::Sample& sample, const Controls& consumed)
        {
            MappingValue result;
            for (const Control& condition : binding.m_conditions)
            {
                if (!Held(condition, sample, consumed))
                    return {};

                Use(condition, result.m_controls);
            }

            for (const Control& control : binding.m_controls)
            {
                if (Held(control, sample, consumed))
                    Use(control, result.m_controls);
            }

            const auto down = [&](const uint32_t index) {
                return Held(binding.m_controls[index], sample, consumed) ? 1.0f : 0.0f;
            };
            switch (binding.m_kind)
            {
            case BindingKind::kButton:
                result.m_value = Vector2(down(0));
                break;
            case BindingKind::kAxis1D:
                result.m_value.x = down(1) - down(0);
                break;
            case BindingKind::kAxis2D:
                result.m_value = { down(1) - down(0), down(3) - down(2) };
                break;
            case BindingKind::kChord:
                result.m_value = { 1, 1 };
                for (const Control& control : binding.m_controls)
                {
                    if (!Held(control, sample, consumed))
                        return {};
                }
                break;
            case BindingKind::kMouseDelta:
                result.m_value = consumed.m_motion ? Vector2::kZero : sample.m_mouse.m_delta;
                result.m_controls.m_motion = result.m_value != Vector2::kZero;
                break;
            case BindingKind::kMouseWheel:
                result.m_value.x = consumed.m_wheel ? 0 : sample.m_mouse.m_wheelDelta;
                result.m_controls.m_wheel = result.m_value.x != 0;
                break;
            }

            result.m_actuation = result.m_value * binding.m_scale * binding.m_sensitivity;
            result.m_value = binding.m_integrate ? result.m_actuation * sample.m_deltaSeconds : result.m_actuation;
            if (!std::isfinite(result.m_value.x) || !std::isfinite(result.m_value.y))
                return {};

            return result;
        }


        MappingValue Evaluate(const ActionMapping& mapping, const Core::Sample& sample, const Controls& consumed)
        {
            MappingValue result;
            for (const Binding& binding : mapping.m_bindings)
            {
                const MappingValue value = EvaluateBinding(binding, sample, consumed);
                if (mapping.m_operation == MappingOperation::kSum)
                {
                    result.m_value = result.m_value + value.m_value;
                    result.m_actuation = result.m_actuation + value.m_actuation;
                }
                else if (Math::LengthSquared(value.m_value) > Math::LengthSquared(result.m_value))
                    result.m_value = value.m_value;

                if (mapping.m_operation == MappingOperation::kStrongest
                    && Math::LengthSquared(value.m_actuation) > Math::LengthSquared(result.m_actuation))
                {
                    result.m_actuation = value.m_actuation;
                }

                result.m_controls.Merge(value.m_controls);
            }

            if (mapping.m_clampMagnitude && Math::LengthSquared(result.m_value) > 1)
                result.m_value = Math::Normalize(result.m_value);

            if (mapping.m_type == ActionType::kBoolean)
            {
                result.m_value = { result.m_value != Vector2::kZero ? 1.0f : 0.0f, 0 };
                result.m_actuation = { result.m_actuation != Vector2::kZero ? 1.0f : 0.0f, 0 };
            }
            else if (mapping.m_type == ActionType::kScalar)
            {
                result.m_value.y = 0;
                result.m_actuation.y = 0;
            }

            return result;
        }
    } // namespace


    ContextHandle MappingStack::Activate(IO::AssetHandle<MappingTable> table, const int32_t priority, const bool modal)
    {
        FE_Assert(table.IsValid());
        static std::atomic<uint64_t> nextContext = 0;
        const ContextHandle handle{ ++nextContext };
        TableEntry entry;
        entry.m_table = std::move(table);
        entry.m_handle = handle;
        entry.m_priority = priority;
        entry.m_modal = modal;
        m_tables.push_back(std::move(entry));
        std::sort(m_tables.begin(), m_tables.end(), [](const TableEntry& lhs, const TableEntry& rhs) {
            return lhs.m_priority == rhs.m_priority ? lhs.m_handle.m_value > rhs.m_handle.m_value
                                                    : lhs.m_priority > rhs.m_priority;
        });
        return handle;
    }


    void MappingStack::Deactivate(const ContextHandle context)
    {
        m_tables.erase(std::remove_if(m_tables.begin(),
                                      m_tables.end(),
                                      [context](const TableEntry& entry) {
                                          return entry.m_handle == context;
                                      }),
                       m_tables.end());
    }


    void MappingStack::Clear()
    {
        m_tables.clear();
        m_runtime.clear();
        m_indices.clear();
        m_results.clear();
        m_events.clear();
        m_frame = Constants::kMaxU64;
    }


    uint32_t MappingStack::Bind(const IO::AssetID action)
    {
        const auto found = m_indices.find(action);
        if (found != m_indices.end())
            return found->second;

        const uint32_t index = m_runtime.size();
        RuntimeAction runtime;
        runtime.m_id = action;
        runtime.m_asset = IO::Link<Action>(action).GetAssetHandle();
        m_runtime.push_back(std::move(runtime));
        m_indices.emplace(action, index);
        m_results.push_back({});
        m_results.back().m_action = action;
        return index;
    }


    uint32_t MappingStack::Bind(const IO::AssetID action, const ActionType expectedType)
    {
        const uint32_t index = Bind(action);
        RuntimeAction& runtime = m_runtime[index];
        const uint32_t mask = 1u << festd::to_underlying(expectedType);
        if ((runtime.m_expectedTypes & mask) == 0)
        {
            runtime.m_expectedTypes |= mask;
            runtime.m_reportedTypeGeneration = Constants::kMaxU32;
        }

        return index;
    }


    void MappingStack::Cancel(const uint32_t index)
    {
        RuntimeAction& runtime = m_runtime[index];
        if (runtime.m_context.m_wasActive)
            m_events.push_back({ runtime.m_id, ActionState::kCanceled, Vector2::kZero });

        runtime.m_context = {};
        runtime.m_processor.Reset();
        runtime.m_owner = 0;
    }


    void MappingStack::Prepare()
    {
        for (RuntimeAction& runtime : m_runtime)
        {
            runtime.m_visited = false;
            runtime.m_displacement = { 0, 0 };
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

            for (const CompiledMapping& compiled : entry.m_compiled)
            {
                RuntimeAction& runtime = m_runtime[compiled.m_index];
                if (runtime.m_visited)
                    continue;

                runtime.m_visited = true;
                if (!runtime.m_asset.IsValid())
                    runtime.m_asset = compiled.m_mapping.m_action.GetAssetHandle();

                const auto action = runtime.m_asset.Read();
                if (!action || !action->Validate() || action->m_type != compiled.m_mapping.m_type)
                {
                    Cancel(compiled.m_index);
                    if (action)
                        Logger::LogError("Input action {} has incompatible mapping type or invalid parameters", runtime.m_id);

                    continue;
                }

                const uint32_t generation = runtime.m_asset.GetGeneration();
                const uint32_t incompatibleTypes = runtime.m_expectedTypes & ~(1u << festd::to_underlying(action->m_type));
                if (incompatibleTypes != 0 && runtime.m_reportedTypeGeneration != generation)
                {
                    Logger::LogError("Input action {} is bound with an incompatible value type", runtime.m_id);
                    runtime.m_reportedTypeGeneration = generation;
                }

                const bool changed = runtime.m_owner != entry.m_handle.m_value || runtime.m_tableGeneration != entry.m_generation
                    || runtime.m_actionGeneration != runtime.m_asset.GetGeneration();
                if (changed)
                    Cancel(compiled.m_index);

                runtime.m_owner = entry.m_handle.m_value;
                runtime.m_tableGeneration = entry.m_generation;
                runtime.m_actionGeneration = runtime.m_asset.GetGeneration();
                runtime.m_type = action->m_type;
                runtime.m_processor = action->m_processor;
                runtime.m_discrete = std::any_of(compiled.m_mapping.m_bindings.begin(),
                                                 compiled.m_mapping.m_bindings.end(),
                                                 [](const Binding& binding) {
                                                     return binding.m_kind == BindingKind::kMouseDelta
                                                         || binding.m_kind == BindingKind::kMouseWheel || binding.m_integrate;
                                                 });
            }

            if (entry.m_modal)
                break;
        }

        for (uint32_t i = 0; i < m_runtime.size(); ++i)
        {
            if (!m_runtime[i].m_visited)
                Cancel(i);

            m_results[i] = {};
            m_results[i].m_action = m_runtime[i].m_id;
            m_results[i].m_type = m_runtime[i].m_type;
        }
    }


    void MappingStack::ProcessSample(const Core::Sample& sample)
    {
        Controls consumed;
        for (const TableEntry& entry : m_tables)
        {
            Controls tableConsumed;
            for (const CompiledMapping& compiled : entry.m_compiled)
            {
                RuntimeAction& runtime = m_runtime[compiled.m_index];
                if (runtime.m_owner != entry.m_handle.m_value)
                    continue;

                const MappingValue value = Evaluate(compiled.m_mapping, sample, consumed);
                auto& context = runtime.m_context;
                context.m_active = value.m_actuation != Vector2::kZero;
                context.m_pressed = context.m_active && !context.m_wasActive;
                context.m_deltaSeconds = sample.m_deltaSeconds;
                const bool started = context.m_pressed;
                const bool ended = !context.m_active && context.m_wasActive;
                const bool completed = context.m_completed;
                auto emit = [&](const ActionState state) {
                    m_events.push_back({ runtime.m_id, state, value.m_value });
                };
                if (started)
                    emit(ActionState::kStarted);

                const ActionState state = runtime.m_processor ? runtime.m_processor->Process(context) : ActionState::kTriggered;
                if (state == ActionState::kTriggered && context.m_active && (!completed || !context.m_completed))
                {
                    if (started || (runtime.m_processor && sample.m_deltaSeconds > 0))
                        emit(ActionState::kTriggered);
                }
                else if (ended)
                {
                    emit(runtime.m_processor ? state : ActionState::kCompleted);
                }

                if (runtime.m_discrete)
                    runtime.m_displacement = runtime.m_displacement + value.m_value;

                ResolvedAction& result = m_results[compiled.m_index];
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
                               const uint64_t frame, const festd::span<const Core::Sample> samples)
    {
        FE_Assert(std::isfinite(deltaSeconds) && deltaSeconds >= 0);
        if (m_frame == frame)
            return;

        m_frame = frame;
        m_events.clear();
        Prepare();
        if (samples.empty())
            ProcessSample({ keyboard, mouse, deltaSeconds });
        else
        {
            for (const Core::Sample& sample : samples)
                ProcessSample(sample);
        }

        for (const ActionEvent& event : m_events)
        {
            const uint32_t index = m_indices.at(event.m_action);
            m_results[index].m_state = event.m_state;
            m_results[index].m_hasEvent = true;
        }
    }


    const ResolvedAction* MappingStack::FindAction(const IO::AssetID action) const
    {
        const auto found = m_indices.find(action);
        return found == m_indices.end() ? nullptr : &m_results[found->second];
    }
} // namespace FE::Input
