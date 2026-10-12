#include <AssetBuilder/AssetFile.h>
#include <AssetBuilder/AssetPipeline.h>
#include <Core/IO/Artifact.h>
#include <Core/IO/AssetManager.h>
#include <Core/IO/MemoryStream.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Core/Serialization/JsonSerialization.h>
#include <Core/Strings/Format.h>
#include <Core/Threading/Thread.h>
#include <Framework/Application/Platform/Windows/PlatformApplication.h>
#include <Framework/Entities/EntityWorldInstance.h>
#include <Framework/Entities/WorldTimeService.h>
#include <Framework/Input/Desktop/Backend.h>
#include <Framework/Input/MappingStack.h>
#include <Framework/Input/WorldInputService.h>
#include <Framework/Reflection.gen.h>
#include <InputTestTypes.h>
#include <Reflection.gen.h>
#include <gtest/gtest.h>

namespace FE::Input::Tests
{

    namespace
    {
        struct InputAssets : testing::Test
        {
            IO::Path m_directory;
            festd::vector<IO::AssetRequest> m_requests;

            void SetUp() override
            {
                CallLinkerAnchor_Framework();
                m_directory = IO::Path(FE_ENTITY_ASSET_TEST_OUTPUT) / Fmt::Format("Input-{}", Uuid::Random());
                IO::ArtifactStore::SetCatalogSource(m_directory);
                IO::AssetManager::Init();
            }


            void TearDown() override
            {
                m_requests.clear();
                IO::AssetManager::Shutdown();
            }


            template<class T>
            IO::Link<T> Build(const char* name, const T& value)
            {
                const IO::Path path = m_directory / name;
                EXPECT_TRUE(AssetBuilder::ImportAsset({ {}, path, Rtti::GetTypeID<T>() }));
                AssetBuilder::AssetFile file;
                EXPECT_TRUE(AssetBuilder::LoadAssetFile(path, file));
                auto& artifact = file.m_artifacts.front();
                artifact.m_buildSettings.Emplace<T>(value);
                const auto id = artifact.m_assetId;
                EXPECT_TRUE(AssetBuilder::SaveAssetFile(path, file));
                EXPECT_TRUE(AssetBuilder::ImportAsset({ {}, path, Rtti::GetTypeID<T>() }));
                EXPECT_TRUE(AssetBuilder::BuildAsset({ path, {}, m_directory }));
                return IO::Link<T>(id);
            }


            IO::AssetHandle<MappingTable> Load(const IO::Link<MappingTable>& table)
            {
                m_requests.push_back(IO::AssetManager::LoadAsset(table));
                for (uint32_t i = 0; i < 5000 && !m_requests.back().IsCompleted(); ++i)
                {
                    IO::AssetManager::Tick();
                    Threading::Sleep(1);
                }

                EXPECT_EQ(m_requests.back().GetResult(), IO::AssetLoadResult::kSucceeded);
                return table.GetAssetHandle();
            }
        };
    } // namespace


    TEST(InputBackend, EdgesRepeatMotionAndFocusLoss)
    {
        Desktop::Backend backend;
        using Framework::Core::PlatformEvent;
        const PlatformEvent events[] = { PlatformEvent::KeyboardKeyDown(Core::Key::kW),
                                         PlatformEvent::KeyboardKeyDown(Core::Key::kW),
                                         PlatformEvent::MouseDown(1),
                                         PlatformEvent::MouseMove(10, 10),
                                         PlatformEvent::MouseMove(15, 7),
                                         PlatformEvent::MouseWheel(1),
                                         PlatformEvent::MouseWheel(2) };
        backend.Update(events);
        EXPECT_EQ(backend.EnumerateDevices().size(), 2);
        EXPECT_TRUE(backend.GetKeyboardState().m_keyPressMap.test(87));
        EXPECT_TRUE(backend.GetKeyboardState().m_keyDownMap.test(87));
        EXPECT_FLOAT_EQ(backend.GetMouseState().m_delta.x, 5);
        EXPECT_FLOAT_EQ(backend.GetMouseState().m_delta.y, -3);
        EXPECT_FLOAT_EQ(backend.GetMouseState().m_wheelDelta, 3);
        backend.Update({});
        EXPECT_FALSE(backend.GetKeyboardState().m_keyPressMap.test(87));
        EXPECT_TRUE(backend.GetKeyboardState().m_keyDownMap.test(87));
        EXPECT_FLOAT_EQ(backend.GetMouseState().m_delta.x, 0);
        const PlatformEvent unfocus[] = { PlatformEvent::WindowFocusChanged(nullptr, false) };
        backend.Update(unfocus);
        EXPECT_FALSE(backend.GetKeyboardState().m_keyDownMap.any());
        EXPECT_TRUE(backend.GetKeyboardState().m_keyUpMap.test(87));
        EXPECT_TRUE(backend.GetMouseState().m_buttonUpMap.test(1));
    }


    TEST(InputBackend, WindowsPollingDoesNotReplayReleasedKeysOrMouseMotion)
    {
        Framework::Windows::PlatformApplication app;
        Rc<Framework::Core::PlatformWindow> window = app.CreateWindow({});
        const HWND handle = reinterpret_cast<HWND>(window->GetNativeHandle().m_value);
        Desktop::Backend backend;
        app.PollEvents();
        backend.Update(app.GetEvents());

        EXPECT_TRUE(PostMessageW(handle, WM_KEYDOWN, 'W', 0));
        EXPECT_TRUE(PostMessageW(handle, WM_MOUSEMOVE, 0, MAKELPARAM(10, 10)));
        EXPECT_TRUE(PostMessageW(handle, WM_MOUSEMOVE, 0, MAKELPARAM(15, 7)));
        app.PollEvents();
        backend.Update(app.GetEvents());
        EXPECT_TRUE(backend.GetKeyboardState().m_keyDownMap.test(87));
        EXPECT_TRUE(backend.GetKeyboardState().m_keyPressMap.test(87));
        EXPECT_FLOAT_EQ(backend.GetMouseState().m_delta.x, 5);
        EXPECT_FLOAT_EQ(backend.GetMouseState().m_delta.y, -3);

        EXPECT_TRUE(PostMessageW(handle, WM_KEYUP, 'W', 0));
        app.PollEvents();
        backend.Update(app.GetEvents());
        EXPECT_FALSE(backend.GetKeyboardState().m_keyDownMap.test(87));
        EXPECT_FALSE(backend.GetKeyboardState().m_keyPressMap.test(87));
        EXPECT_TRUE(backend.GetKeyboardState().m_keyUpMap.test(87));
        EXPECT_EQ(backend.GetMouseState().m_delta, Vector2::kZero);

        app.PollEvents();
        EXPECT_TRUE(app.GetEvents().empty());
        backend.Update(app.GetEvents());
        EXPECT_FALSE(backend.GetKeyboardState().m_keyDownMap.test(87));
        EXPECT_FALSE(backend.GetKeyboardState().m_keyPressMap.test(87));
        EXPECT_FALSE(backend.GetKeyboardState().m_keyUpMap.test(87));
        EXPECT_EQ(backend.GetMouseState().m_delta, Vector2::kZero);
    }


    namespace
    {
        Binding Button(const Core::Key key)
        {
            Binding binding;
            binding.m_controls = { Control::Key(key) };
            return binding;
        }


        ActionMapping Map(const IO::Link<Action>& action, const ActionType type, festd::vector<Binding> bindings)
        {
            ActionMapping mapping;
            mapping.m_action = action;
            mapping.m_type = type;
            mapping.m_bindings = std::move(bindings);
            return mapping;
        }


        bool HasEvent(const MappingStack& stack, const IO::Link<Action>& action, const ActionState state)
        {
            for (const ActionEvent& event : stack.GetEvents())
            {
                if (event.m_action == action.GetAssetID() && event.m_state == state)
                    return true;
            }

            return false;
        }


        void Resolve(MappingStack& stack, Desktop::Backend& backend, const uint64_t frame, const float seconds = 0)
        {
            stack.Resolve(backend.GetKeyboardState(), backend.GetMouseState(), seconds, frame, backend.GetSamples());
        }
    } // namespace


    TEST_F(InputAssets, ShortTapCancelsHoldAndPreservesPressEvents)
    {
        using Framework::Core::PlatformEvent;
        Action hold;
        auto processor = Rc<HoldDurationActionProcessor>(Memory::DefaultNew<HoldDurationActionProcessor>());
        processor->m_holdTime = 0.25f;
        hold.m_processor = processor;
        const auto holdLink = Build("hold.asset", hold);
        const auto pressLink = Build("press.asset", Action{});
        MappingTable table;
        table.m_mappings = { Map(holdLink, ActionType::kBoolean, { Button(Core::Key::kW) }),
                             Map(pressLink, ActionType::kBoolean, { Button(Core::Key::kW) }) };
        MappingStack stack;
        (void)stack.Activate(Load(Build("table.asset", table)));
        Desktop::Backend backend;
        const PlatformEvent tap[] = { PlatformEvent::KeyboardKeyDown(Core::Key::kW),
                                      PlatformEvent::KeyboardKeyUp(Core::Key::kW) };
        backend.Update(tap, 0.5f);
        Resolve(stack, backend, 1, 0.5f);
        EXPECT_TRUE(HasEvent(stack, holdLink, ActionState::kStarted));
        EXPECT_TRUE(HasEvent(stack, holdLink, ActionState::kCanceled));
        EXPECT_FALSE(HasEvent(stack, holdLink, ActionState::kTriggered));
        EXPECT_TRUE(HasEvent(stack, pressLink, ActionState::kTriggered));
        EXPECT_FALSE(stack.FindAction(pressLink.GetAssetID())->m_active);
        backend.Update({});
        Resolve(stack, backend, 2);
        EXPECT_TRUE(stack.GetEvents().empty());
    }


    TEST_F(InputAssets, HoldUsesTimestampIntervalsAndIndependentWorldState)
    {
        using Framework::Core::PlatformEvent;
        Action hold;
        auto processor = Rc<HoldDurationActionProcessor>(Memory::DefaultNew<HoldDurationActionProcessor>());
        processor->m_holdTime = 0.25f;
        hold.m_processor = processor;
        const auto action = Build("hold.asset", hold);
        MappingTable table;
        table.m_mappings = { Map(action, ActionType::kBoolean, { Button(Core::Key::kW) }) };
        const auto handle = Load(Build("table.asset", table));
        MappingStack first, second;
        (void)first.Activate(handle);
        (void)second.Activate(handle);
        Desktop::Backend backend;
        PlatformEvent tap[] = { PlatformEvent::KeyboardKeyDown(Core::Key::kW), PlatformEvent::KeyboardKeyUp(Core::Key::kW) };
        tap[0].m_timestampSeconds = 10.3;
        tap[1].m_timestampSeconds = 10.4;
        backend.Update(tap, 0.5f, 10.5);
        Resolve(first, backend, 1, 0.5f);
        EXPECT_FALSE(HasEvent(first, action, ActionState::kTriggered));
        tap[0].m_timestampSeconds = 10.1;
        backend.Update({ tap, 1 }, 0.5f, 10.5);
        Resolve(first, backend, 2, 0.5f);
        EXPECT_TRUE(HasEvent(first, action, ActionState::kTriggered));
        backend.Update({}, 0.1f, 10.6);
        Resolve(first, backend, 3, 0.1f);
        Resolve(second, backend, 1, 0.1f);
        EXPECT_FALSE(HasEvent(first, action, ActionState::kTriggered));
        EXPECT_FALSE(HasEvent(second, action, ActionState::kTriggered));
    }


    TEST_F(InputAssets, ChordRequiresOverlapAndSupportsAlternativeBindings)
    {
        using Framework::Core::PlatformEvent;
        const auto action = Build("press.asset", Action{});
        Binding chord;
        chord.m_kind = BindingKind::kChord;
        chord.m_controls = { Control::Key(Core::Key::kQ), Control::Key(Core::Key::kE) };
        MappingTable table;
        table.m_mappings = { Map(action, ActionType::kBoolean, { chord, Button(Core::Key::kF1) }) };
        MappingStack stack;
        (void)stack.Activate(Load(Build("table.asset", table)));
        Desktop::Backend backend;
        const PlatformEvent separate[] = { PlatformEvent::KeyboardKeyDown(Core::Key::kQ),
                                           PlatformEvent::KeyboardKeyUp(Core::Key::kQ),
                                           PlatformEvent::KeyboardKeyDown(Core::Key::kE) };
        backend.Update(separate);
        Resolve(stack, backend, 1);
        EXPECT_FALSE(HasEvent(stack, action, ActionState::kTriggered));
        const PlatformEvent overlap[] = { PlatformEvent::KeyboardKeyDown(Core::Key::kQ) };
        backend.Update(overlap);
        Resolve(stack, backend, 2);
        EXPECT_TRUE(HasEvent(stack, action, ActionState::kTriggered));
        const PlatformEvent alternate[] = { PlatformEvent::KeyboardKeyUp(Core::Key::kQ),
                                            PlatformEvent::KeyboardKeyUp(Core::Key::kE),
                                            PlatformEvent::KeyboardKeyDown(Core::Key::kF1) };
        backend.Update(alternate);
        Resolve(stack, backend, 3);
        EXPECT_TRUE(HasEvent(stack, action, ActionState::kTriggered));
    }


    TEST_F(InputAssets, MouseGateUsesOrderedMotionBeforeAndAfterButtonTransitions)
    {
        using Framework::Core::PlatformEvent;
        Action look;
        look.m_type = ActionType::kVector2;
        const auto action = Build("look.asset", look);
        Binding mouse;
        mouse.m_kind = BindingKind::kMouseDelta;
        mouse.m_conditions = { Control::MouseButton(1) };
        mouse.m_sensitivity = 0.01f;
        MappingTable table;
        table.m_mappings = { Map(action, ActionType::kVector2, { mouse }) };
        MappingStack stack;
        (void)stack.Activate(Load(Build("table.asset", table)));
        Desktop::Backend backend;
        const PlatformEvent before[] = { PlatformEvent::MouseMove(0, 0),
                                         PlatformEvent::MouseMove(100, 0),
                                         PlatformEvent::MouseDown(1) };
        backend.Update(before);
        Resolve(stack, backend, 1);
        EXPECT_EQ(stack.FindAction(action.GetAssetID())->m_value, Vector2::kZero);
        const PlatformEvent during[] = { PlatformEvent::MouseMove(120, 10),
                                         PlatformEvent::MouseUp(1),
                                         PlatformEvent::MouseMove(200, 50) };
        backend.Update(during);
        Resolve(stack, backend, 2);
        EXPECT_NEAR(stack.FindAction(action.GetAssetID())->m_value.x, 0.2f, 1e-6f);
        EXPECT_NEAR(stack.FindAction(action.GetAssetID())->m_value.y, 0.1f, 1e-6f);
        backend.Update({});
        Resolve(stack, backend, 3);
        EXPECT_FALSE(stack.FindAction(action.GetAssetID())->m_active);
    }


    TEST_F(InputAssets, OwnershipChangesCancelBeforeReplacementStartsAndModalBlocks)
    {
        Action hold;
        hold.m_processor = Memory::DefaultNew<HoldDurationActionProcessor>();
        const auto action = Build("hold.asset", hold);
        MappingTable low, high;
        low.m_mappings = { Map(action, ActionType::kBoolean, { Button(Core::Key::kW) }) };
        high.m_mappings = { Map(action, ActionType::kBoolean, { Button(Core::Key::kQ) }) };
        MappingStack stack;
        (void)stack.Activate(Load(Build("low.asset", low)), 0);
        Core::KeyboardState keyboard;
        Core::MouseState mouse;
        keyboard.m_keyDownMap.set(87);
        stack.Resolve(keyboard, mouse, 0.1f, 1);
        const auto override = stack.Activate(Load(Build("high.asset", high)), 100);
        stack.Resolve(keyboard, mouse, 0.1f, 2);
        EXPECT_TRUE(HasEvent(stack, action, ActionState::kCanceled));
        EXPECT_FALSE(stack.FindAction(action.GetAssetID())->m_active);
        stack.Deactivate(override);
        stack.Resolve(keyboard, mouse, 0.1f, 3);
        EXPECT_TRUE(HasEvent(stack, action, ActionState::kStarted));
        const auto modal = stack.Activate(Load(Build("modal.asset", MappingTable{})), 1000, true);
        stack.Resolve(keyboard, mouse, 0.1f, 4);
        EXPECT_TRUE(HasEvent(stack, action, ActionState::kCanceled));
        EXPECT_FALSE(stack.FindAction(action.GetAssetID())->m_active);
        stack.Deactivate(modal);
        keyboard.m_keyDownMap.set(81);
        (void)stack.Activate(Load(Build("replacement.asset", high)), 100);
        stack.Resolve(keyboard, mouse, 0.1f, 5);
        EXPECT_TRUE(HasEvent(stack, action, ActionState::kStarted));
    }


    TEST_F(InputAssets, ConsumptionAndNormalizationAreBindingPolicies)
    {
        Action move;
        move.m_type = ActionType::kVector2;
        const auto action = Build("move.asset", move);
        const auto other = Build("other.asset", Action{});
        MappingTable low, high;
        low.m_mappings = { Map(action, ActionType::kVector2, { Binding::WASD() }) };
        low.m_mappings[0].m_clampMagnitude = true;
        high.m_mappings = { Map(other, ActionType::kBoolean, { Button(Core::Key::kW) }) };
        high.m_mappings[0].m_consume = true;
        MappingStack stack;
        (void)stack.Activate(Load(Build("low.asset", low)), 0);
        Core::KeyboardState keyboard;
        Core::MouseState mouse;
        keyboard.m_keyDownMap.set(87).set(68);
        stack.Resolve(keyboard, mouse, 0.1f, 1);
        EXPECT_NEAR(stack.FindAction(action.GetAssetID())->m_value.x, 0.7071067f, 1e-5f);
        (void)stack.Activate(Load(Build("high.asset", high)), 100);
        stack.Resolve(keyboard, mouse, 0.1f, 1);
        EXPECT_NEAR(stack.FindAction(action.GetAssetID())->m_value.y, 0.7071067f, 1e-5f);
        stack.Resolve(keyboard, mouse, 0.1f, 2);
        EXPECT_EQ(stack.FindAction(action.GetAssetID())->m_value, (Vector2{ 1, 0 }));
    }


    namespace
    {
        struct FixedBackend final : Core::Backend
        {
            Core::KeyboardState m_keyboard;
            Core::MouseState m_mouse;
            festd::vector<Core::Sample> m_samples;
            uint64_t m_sequence = 0;
            festd::span<const Core::DeviceKind> EnumerateDevices() const override
            {
                return {};
            }
            void Update() override {}
            uint64_t GetSampleSequence() const override
            {
                return m_sequence;
            }
            festd::span<const Core::Sample> GetSamples() const override
            {
                return m_samples;
            }
            const Core::KeyboardState& GetKeyboardState() const override
            {
                return m_keyboard;
            }
            const Core::MouseState& GetMouseState() const override
            {
                return m_mouse;
            }
        };
    } // namespace


    TEST_F(InputAssets, WorldOwnsResidencyContextsTypedHandlesAndIndependentFrames)
    {
        Action move;
        move.m_type = ActionType::kVector2;
        const auto action = Build("move.asset", move);
        MappingTable table;
        table.m_mappings = { Map(action, ActionType::kVector2, { Binding::WASD() }) };
        const auto tableLink = Build("table.asset", table);
        Framework::EntityWorld first, second;
        Framework::WorldTimeService firstTime, secondTime;
        WorldInputService firstInput, secondInput;
        auto* firstBackend = Memory::DefaultNew<FixedBackend>();
        auto* secondBackend = Memory::DefaultNew<FixedBackend>();
        firstBackend->m_keyboard.m_keyDownMap.set(87);
        secondBackend->m_keyboard.m_keyDownMap.set(68);
        firstInput.SetBackend(festd::unique_ptr<Core::Backend>(firstBackend));
        secondInput.SetBackend(festd::unique_ptr<Core::Backend>(secondBackend));
        first.AddService(firstTime);
        first.AddService(firstInput);
        second.AddService(secondTime);
        second.AddService(secondInput);
        auto cleanup = festd::defer([&] {
            first.Clear();
            second.Clear();
            first.RemoveService(firstInput);
            first.RemoveService(firstTime);
            second.RemoveService(secondInput);
            second.RemoveService(secondTime);
        });
        const auto firstContext = firstInput.Activate(tableLink);
        (void)secondInput.Activate(tableLink);
        for (uint32_t i = 0; i < 5000 && !tableLink.GetAssetHandle().Read(); ++i)
        {
            IO::AssetManager::Tick();
            Threading::Sleep(1);
        }

        ASSERT_TRUE(tableLink.GetAssetHandle().Read());
        const auto firstHandle = firstInput.Bind<Vector2>(action);
        const auto secondHandle = secondInput.Bind<Vector2>(action);
        const auto wrongType = firstInput.Bind<float>(action);
        firstInput.Update(first);
        secondInput.Update(second);
        EXPECT_EQ(firstInput.GetFrame().Read(firstHandle), (Vector2{ 0, 1 }));
        EXPECT_EQ(secondInput.GetFrame().Read(secondHandle), (Vector2{ 1, 0 }));
        EXPECT_EQ(secondInput.GetFrame().Read(firstHandle), Vector2::kZero);
        EXPECT_FLOAT_EQ(firstInput.GetFrame().Read(wrongType), 0);
        firstInput.Deactivate(firstContext);
        EXPECT_EQ(firstInput.GetFrame().Read(firstHandle), (Vector2{ 0, 1 }));
        firstInput.Update(first);
        EXPECT_EQ(firstInput.GetFrame().Read(firstHandle), Vector2::kZero);
        EXPECT_EQ(secondInput.GetFrame().Read(secondHandle), (Vector2{ 1, 0 }));
    }


    TEST_F(InputAssets, UpdatingTwiceBetweenPlatformPollsDoesNotReplayTransients)
    {
        using Framework::Core::PlatformEvent;
        const auto button = Build("button.asset", Action{});
        Action look;
        look.m_type = ActionType::kVector2;
        const auto lookLink = Build("look.asset", look);
        Binding mouse;
        mouse.m_kind = BindingKind::kMouseDelta;
        mouse.m_conditions = { Control::MouseButton(1) };
        MappingTable table;
        table.m_mappings = { Map(button, ActionType::kBoolean, { Button(Core::Key::kW) }),
                             Map(lookLink, ActionType::kVector2, { mouse }) };
        const auto link = Build("table.asset", table);
        Framework::EntityWorld world;
        Framework::WorldTimeService time;
        WorldInputService input;
        auto* backend = Memory::DefaultNew<FixedBackend>();
        input.SetBackend(festd::unique_ptr<Core::Backend>(backend));
        world.AddService(time);
        world.AddService(input);
        auto cleanup = festd::defer([&] {
            world.Clear();
            world.RemoveService(input);
            world.RemoveService(time);
        });
        (void)input.Activate(link);
        for (uint32_t i = 0; i < 5000 && !link.GetAssetHandle().Read(); ++i)
        {
            IO::AssetManager::Tick();
            Threading::Sleep(1);
        }

        ASSERT_TRUE(link.GetAssetHandle().Read());
        const auto lookHandle = input.Bind<Vector2>(lookLink);
        Desktop::Backend source;
        const PlatformEvent events[] = { PlatformEvent::KeyboardKeyDown(Core::Key::kW),
                                         PlatformEvent::KeyboardKeyUp(Core::Key::kW),
                                         PlatformEvent::MouseDown(1),
                                         PlatformEvent::MouseMove(0, 0),
                                         PlatformEvent::MouseMove(10, 20) };
        source.Update(events);
        backend->m_keyboard = source.GetKeyboardState();
        backend->m_mouse = source.GetMouseState();
        const auto samples = source.GetSamples();
        backend->m_samples.assign(samples.begin(), samples.end());
        backend->m_sequence = source.GetSampleSequence();
        input.Update(world);
        EXPECT_EQ(input.GetFrame().Read(lookHandle), (Vector2{ 10, 20 }));
        EXPECT_FALSE(input.GetFrame().GetEvents().empty());
        input.Update(world);
        EXPECT_EQ(input.GetFrame().Read(lookHandle), Vector2::kZero);
        EXPECT_TRUE(input.GetFrame().GetEvents().empty());
    }


    TEST_F(InputAssets, SerializedServiceConfigurationSurvivesWorldSnapshotRestore)
    {
        const auto action = Build("action.asset", Action{});
        MappingTable table;
        table.m_mappings = { Map(action, ActionType::kBoolean, { Button(Core::Key::kW) }) };
        const auto link = Build("table.asset", table);
        Framework::EntityWorldAsset definition;
        definition.m_services.push_back(Rtti::GetTypeID<Framework::WorldTimeService>());
        auto& descriptor = definition.m_services.emplace_back(Rtti::GetTypeID<WorldInputService>());
        descriptor.m_configuration.Emplace<InputConfiguration>().m_contexts.push_back({ link, 42, true });
        Framework::EntityWorldSnapshotAsset snapshot;
        {
            Framework::EntityWorldInstance instance;
            ASSERT_TRUE(instance.Load(definition));
            auto* input = instance.GetWorld().FindService<WorldInputService>();
            ASSERT_NE(input, nullptr);
            auto* backend = Memory::DefaultNew<FixedBackend>();
            backend->m_keyboard.m_keyDownMap.set(87);
            input->SetBackend(festd::unique_ptr<Core::Backend>(backend));
            const auto handle = input->Bind<bool>(action);
            for (uint32_t i = 0; i < 5000 && !link.GetAssetHandle().Read(); ++i)
            {
                IO::AssetManager::Tick();
                Threading::Sleep(1);
            }

            ASSERT_TRUE(link.GetAssetHandle().Read());
            input->Update(instance.GetWorld());
            EXPECT_TRUE(input->GetFrame().Read(handle));
            ASSERT_TRUE(instance.Capture(snapshot));
            const auto* configuration = snapshot.m_services[1].m_configuration.TryGet<InputConfiguration>();
            ASSERT_NE(configuration, nullptr);
            ASSERT_EQ(configuration->m_contexts.size(), 1);
            EXPECT_EQ(configuration->m_contexts[0].m_priority, 42);
            EXPECT_TRUE(configuration->m_contexts[0].m_modal);
        }

        Framework::EntityWorldInstance restored;
        ASSERT_TRUE(restored.Restore(snapshot));
        const auto* input = restored.GetWorld().FindService<WorldInputService>();
        ASSERT_NE(input, nullptr);
        ASSERT_EQ(input->GetContexts().size(), 1);
        EXPECT_EQ(input->GetContexts()[0].m_table.GetAssetID(), link.GetAssetID());
    }


    TEST_F(InputAssets, RateIntegrationKeepsInteractionActiveAcrossZeroDurationTransitions)
    {
        using Framework::Core::PlatformEvent;
        Action look;
        look.m_type = ActionType::kVector2;
        look.m_processor = Memory::DefaultNew<ContinuousActionProcessor>();
        const auto action = Build("rate.asset", look);
        Binding rate = Binding::WASD();
        rate.m_integrate = true;
        rate.m_sensitivity = 2;
        MappingTable table;
        table.m_mappings = { Map(action, ActionType::kVector2, { rate }) };
        MappingStack stack;
        (void)stack.Activate(Load(Build("table.asset", table)));
        Desktop::Backend backend;
        PlatformEvent press = PlatformEvent::KeyboardKeyDown(Core::Key::kW);
        press.m_timestampSeconds = 10.1;
        backend.Update({ &press, 1 }, 0.5f, 10.5);
        Resolve(stack, backend, 1, 0.5f);
        EXPECT_NEAR(stack.FindAction(action.GetAssetID())->m_value.y, 0.8f, 1e-5f);
        EXPECT_FALSE(HasEvent(stack, action, ActionState::kCompleted));
        backend.Update({}, 0.1f, 10.6);
        Resolve(stack, backend, 2, 0.1f);
        EXPECT_NEAR(stack.FindAction(action.GetAssetID())->m_value.y, 0.2f, 1e-5f);
        EXPECT_FALSE(HasEvent(stack, action, ActionState::kStarted));
        EXPECT_FALSE(HasEvent(stack, action, ActionState::kCompleted));
    }


    TEST_F(InputAssets, AssetBuilderRejectsInvalidMappingConfiguration)
    {
        const IO::Path path = m_directory / "invalid.asset";
        ASSERT_TRUE(AssetBuilder::ImportAsset({ {}, path, Rtti::GetTypeID<MappingTable>() }));
        AssetBuilder::AssetFile file;
        ASSERT_TRUE(AssetBuilder::LoadAssetFile(path, file));
        const auto action = Build("action.asset", Action{});
        MappingTable table;
        table.m_mappings = { Map(action, ActionType::kBoolean, { Button(Core::Key::kW) }) };
        table.m_mappings.push_back(table.m_mappings.front());
        file.m_artifacts.front().m_buildSettings.Emplace<MappingTable>(table);
        ASSERT_TRUE(AssetBuilder::SaveAssetFile(path, file));
        ASSERT_TRUE(AssetBuilder::ImportAsset({ {}, path, Rtti::GetTypeID<MappingTable>() }));
        EXPECT_FALSE(AssetBuilder::BuildAsset({ path, {}, m_directory }));
    }


    TEST(InputValidation, RejectsDuplicateRowsInvalidControlsTypesAndParameters)
    {
        ActionMapping mapping = Map(IO::Link<Action>(Uuid::Random()), ActionType::kVector2, { Binding::WASD() });
        MappingTable table;
        table.m_mappings = { mapping };
        EXPECT_TRUE(table.Validate());
        table.m_mappings.push_back(mapping);
        EXPECT_FALSE(table.Validate());
        table.m_mappings.pop_back();
        table.m_mappings[0].m_type = ActionType::kBoolean;
        EXPECT_FALSE(table.Validate());
        table.m_mappings[0].m_type = ActionType::kVector2;
        table.m_mappings[0].m_bindings[0].m_controls[0].m_key = Core::Key::kInvalid;
        EXPECT_FALSE(table.Validate());
        Action hold;
        auto processor = Rc<HoldDurationActionProcessor>(Memory::DefaultNew<HoldDurationActionProcessor>());
        processor->m_holdTime = -1;
        hold.m_processor = processor;
        EXPECT_FALSE(hold.Validate());
    }


    TEST(InputSerialization, PolymorphicRcRoundTripsAndNullReplacesValue)
    {
        CallLinkerAnchor_Framework();
        auto* hold = Memory::DefaultNew<HoldDurationActionProcessor>();
        hold->m_holdTime = 2.5f;
        Rc<ActionProcessor> original(hold);
        IO::WriteOnlyMemoryStream stream;
        Serialization::JsonFormat format;
        Serialization::SerializationContext store(&stream, format);
        ASSERT_EQ(store.Store(original), Serialization::ResultCode::kSuccess);
        festd::pmr::vector<std::byte> bytes;
        stream.DumpAll(bytes);
        IO::ReadOnlyMemoryStream read(bytes);
        Serialization::DeserializationContext load(&read, format);
        Rc<ActionProcessor> copy;
        ASSERT_EQ(load.Load(copy), Serialization::ResultCode::kSuccess);
        ASSERT_NE(copy.Get(), original.Get());
        const auto* restored = Rtti::Cast<HoldDurationActionProcessor*>(copy.Get());
        ASSERT_NE(restored, nullptr);
        EXPECT_FLOAT_EQ(restored->m_holdTime, 2.5f);
        EXPECT_EQ(copy->GetRefCount(), 1);

        IO::WriteOnlyMemoryStream nullStream;
        Serialization::TaggedBinaryFormat binary;
        Serialization::SerializationContext nullStore(&nullStream, binary);
        ASSERT_EQ(nullStore.Store(Rc<ActionProcessor>{}), Serialization::ResultCode::kSuccess);
        bytes.clear();
        nullStream.DumpAll(bytes);
        IO::ReadOnlyMemoryStream nullRead(bytes);
        Serialization::DeserializationContext nullLoad(&nullRead, binary);
        ASSERT_EQ(nullLoad.Load(copy), Serialization::ResultCode::kSuccess);
        EXPECT_FALSE(copy);
    }

    TEST(InputSerialization, OffsetBaseAndMalformedDataPreserveOwnershipInEveryFormat)
    {
        CallLinkerAnchor_FrameworkTests();
        for (uint32_t index = 0; index < 3; ++index)
        {
            auto* processor = Memory::DefaultNew<OffsetProcessor>();
            processor->m_value = 987;
            Rc<ActionProcessor> original(processor);
            EXPECT_NE(static_cast<const void*>(original.Get()), static_cast<const void*>(processor));
            EXPECT_EQ(original->RTTI_GetDynamicObject(), processor);
            const ActionProcessor* constBase = original.Get();
            EXPECT_EQ(constBase->RTTI_GetDynamicObject(), processor);
            EXPECT_EQ(&constBase->RTTI_GetDynamicType(), &Rtti::GetType<OffsetProcessor>());
            Serialization::PackedBinaryFormat packed;
            Serialization::TaggedBinaryFormat tagged;
            Serialization::JsonFormat json;
            Serialization::SerializationFormat* format = index == 0 ? static_cast<Serialization::SerializationFormat*>(&packed)
                : index == 1                                        ? static_cast<Serialization::SerializationFormat*>(&tagged)
                                                                    : &json;
            IO::WriteOnlyMemoryStream stream;
            Serialization::SerializationContext store(&stream, *format);
            ASSERT_EQ(store.Store(original), Serialization::ResultCode::kSuccess);
            festd::pmr::vector<std::byte> bytes;
            stream.DumpAll(bytes);
            IO::ReadOnlyMemoryStream read(bytes);
            Serialization::DeserializationContext load(&read, *format);
            Rc<ActionProcessor> copy;
            ASSERT_EQ(load.Load(copy), Serialization::ResultCode::kSuccess);
            const auto* restored = Rtti::Cast<OffsetProcessor*>(copy.Get());
            ASSERT_NE(restored, nullptr);
            EXPECT_EQ(restored->m_value, 987);
            EXPECT_EQ(copy->GetRefCount(), 1);
            ActionProcessor* previous = copy.Get();
            IO::ReadOnlyMemoryStream malformed(bytes.data(), bytes.size() / 2);
            Serialization::DeserializationContext invalid(&malformed, *format);
            EXPECT_NE(invalid.Load(copy), Serialization::ResultCode::kSuccess);
            EXPECT_EQ(copy.Get(), previous);
            EXPECT_EQ(copy->GetRefCount(), 1);
        }
    }
} // namespace FE::Input::Tests
