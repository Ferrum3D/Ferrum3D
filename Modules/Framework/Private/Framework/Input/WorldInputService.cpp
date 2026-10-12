#include <Core/IO/AssetManager.h>
#include <Core/Logging/Logger.h>
#include <Framework/Application/Core/PlatformApplication.h>
#include <Framework/Entities/EntityWorld.h>
#include <Framework/Entities/WorldTimeService.h>
#include <Framework/Input/WorldInputService.h>
#include <atomic>

namespace FE::Input
{
    bool InputConfiguration::Validate() const
    {
        for (const ContextDefinition& context : m_contexts)
        {
            if (!context.m_table.GetAssetID().IsValid())
                return false;
        }

        return true;
    }


    bool WorldInputService::Configure(const Rtti::Any& settings)
    {
        if (!settings.HasValue())
        {
            m_configuration = {};
            return true;
        }

        const auto* configuration = settings.TryGet<InputConfiguration>();
        if (!configuration || !configuration->Validate())
            return false;

        m_configuration = *configuration;
        return true;
    }


    Rtti::Any WorldInputService::CaptureConfiguration() const
    {
        Rtti::Any result;
        result.Emplace<InputConfiguration>().m_contexts = GetContexts();
        return result;
    }


    void WorldInputService::Init(Framework::EntityWorld& world)
    {
        FE_Assert(world.FindService<Framework::WorldTimeService>() != nullptr, "Input requires WorldTimeService before it");
        static std::atomic<uint64_t> nextGeneration = 0;
        m_inputFrame.m_generation = ++nextGeneration;
        m_deltaSeconds = 0;
        m_lastSampleSequence = 0;
        for (const ContextDefinition& context : m_configuration.m_contexts)
            (void)Activate(context.m_table, context.m_priority, context.m_modal);
    }


    void WorldInputService::Shutdown(Framework::EntityWorld&)
    {
        m_stack.Clear();
        m_contexts.clear();
        m_backend.reset();
        m_inputFrame = {};
    }


    const Core::Backend& WorldInputService::GetBackend() const
    {
        return m_backend ? *m_backend : Framework::Core::PlatformApplication::Get().GetInputBackend();
    }


    void WorldInputService::Update(Framework::EntityWorld& world)
    {
        if (m_backend)
            m_backend->Update();

        m_deltaSeconds = world.FindService<Framework::WorldTimeService>()->GetDeltaSeconds();
        ++m_inputFrame.m_number;
        const Core::Backend& backend = GetBackend();
        const uint64_t sequence = backend.GetSampleSequence();
        if (sequence != 0 && sequence == m_lastSampleSequence)
        {
            Core::KeyboardState keyboard = backend.GetKeyboardState();
            Core::MouseState mouse = backend.GetMouseState();
            keyboard.m_keyPressMap.reset();
            keyboard.m_keyUpMap.reset();
            mouse.m_buttonPressMap.reset();
            mouse.m_buttonUpMap.reset();
            mouse.m_delta = { 0, 0 };
            mouse.m_wheelDelta = 0;
            m_stack.Resolve(keyboard, mouse, m_deltaSeconds, m_inputFrame.m_number);
        }
        else
        {
            m_stack.Resolve(backend.GetKeyboardState(),
                            backend.GetMouseState(),
                            m_deltaSeconds,
                            m_inputFrame.m_number,
                            backend.GetSamples());
        }

        m_lastSampleSequence = sequence;
        const auto actions = m_stack.GetActions();
        const auto events = m_stack.GetEvents();
        m_inputFrame.m_actions.assign(actions.begin(), actions.end());
        m_inputFrame.m_events.assign(events.begin(), events.end());
    }


    void WorldInputService::SetBackend(festd::unique_ptr<Core::Backend> backend)
    {
        FE_Assert(backend != nullptr);
        m_backend = std::move(backend);
        m_lastSampleSequence = 0;
    }


    ContextHandle WorldInputService::Activate(const IO::Link<MappingTable>& table, const int32_t priority, const bool modal)
    {
        FE_Assert(table.GetAssetID() != IO::AssetID::kNull);
        Context context;
        context.m_definition = { table, priority, modal };
        context.m_residency = IO::AssetManager::LoadAsset(table);
        context.m_handle = m_stack.Activate(table.GetAssetHandle(), priority, modal);
        const ContextHandle handle = context.m_handle;
        m_contexts.push_back(std::move(context));
        return handle;
    }


    void WorldInputService::Deactivate(const ContextHandle context)
    {
        m_stack.Deactivate(context);
        m_contexts.erase(std::remove_if(m_contexts.begin(),
                                        m_contexts.end(),
                                        [context](const Context& entry) {
                                            return entry.m_handle == context;
                                        }),
                         m_contexts.end());
    }


    festd::vector<ContextDefinition> WorldInputService::GetContexts() const
    {
        festd::vector<ContextDefinition> result;
        for (const Context& context : m_contexts)
            result.push_back(context.m_definition);

        return result;
    }


    uint32_t WorldInputService::BindInternal(const IO::Link<Action>& action, const ActionType type)
    {
        const auto asset = action.GetAssetHandle().Read();
        if (asset && asset->m_type != type)
            Logger::LogError("Input action {} is bound with the wrong value type", action.GetAssetID());

        return m_stack.Bind(action.GetAssetID(), type);
    }
} // namespace FE::Input
