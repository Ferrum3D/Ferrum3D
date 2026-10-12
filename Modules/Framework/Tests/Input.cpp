#include <AssetBuilder/AssetFile.h>
#include <AssetBuilder/AssetPipeline.h>
#include <Core/IO/Artifact.h>
#include <Core/IO/AssetManager.h>
#include <Core/IO/MemoryStream.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Core/Serialization/JsonSerialization.h>
#include <Core/Strings/Format.h>
#include <Core/Threading/Thread.h>
#include <Core/Time/BaseTime.h>
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
    template<class Configuration>
    concept AcceptsInputConfiguration = requires(const Configuration& configuration) {
        Framework::WorldServiceDescriptor::Create<WorldInputService>(configuration);
    };

    static_assert(AcceptsInputConfiguration<InputConfiguration>);
    static_assert(!AcceptsInputConfiguration<Action>);

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


    TEST(InputBackend, QueuedWindowsMessagesPreserveTheirHoldInterval)
    {
        Framework::Windows::PlatformApplication app;
        Rc<Framework::Core::PlatformWindow> window = app.CreateWindow({});
        const HWND handle = reinterpret_cast<HWND>(window->GetNativeHandle().m_value);
        app.PollEvents();
        ASSERT_TRUE(PostMessageW(handle, WM_KEYDOWN, 'W', 0));
        Threading::Sleep(300);
        ASSERT_TRUE(PostMessageW(handle, WM_KEYUP, 'W', 0));
        app.PollEvents();
        double down = -1, up = -1;
        for (const auto& event : app.GetEvents())
        {
            if (event.m_type == Framework::Core::PlatformEventType::kKeyboardKeyDown)
                down = event.m_timestampSeconds;
            if (event.m_type == Framework::Core::PlatformEventType::kKeyboardKeyUp)
                up = event.m_timestampSeconds;
        }
        ASSERT_GE(down, 0);
        ASSERT_GE(up, 0);
        EXPECT_GE(up - down, 0.25);
        EXPECT_LT(up - down, 2.0);
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
            binding.m_source = ButtonBinding{ Control::Key(key) };
            return binding;
        }


        ActionMapping Map(const IO::Link<Action>& action, festd::vector<Binding> bindings)
        {
            ActionMapping mapping;
            mapping.m_action = action;
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
        table.m_mappings = { Map(holdLink, { Button(Core::Key::kW) }), Map(pressLink, { Button(Core::Key::kW) }) };
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
        table.m_mappings = { Map(action, { Button(Core::Key::kW) }) };
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
        chord.m_source = ChordBinding{ { Control::Key(Core::Key::kQ), Control::Key(Core::Key::kE) } };
        MappingTable table;
        table.m_mappings = { Map(action, { chord, Button(Core::Key::kF1) }) };
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
        mouse.m_source = MouseDeltaBinding{};
        mouse.m_conditions = { Control::MouseButton(1) };
        mouse.m_sensitivity = 0.01f;
        MappingTable table;
        table.m_mappings = { Map(action, { mouse }) };
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
        low.m_mappings = { Map(action, { Button(Core::Key::kW) }) };
        high.m_mappings = { Map(action, { Button(Core::Key::kQ) }) };
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
        EXPECT_EQ(stack.FindAction(action.GetAssetID()), nullptr);
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
        low.m_mappings = { Map(action, { Binding::WASD() }) };
        low.m_mappings[0].m_clampMagnitude = true;
        high.m_mappings = { Map(other, { Button(Core::Key::kW) }) };
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


    TEST_F(InputAssets, WorldOwnsResidencyContextsTypedReferencesAndIndependentFrames)
    {
        Action move;
        move.m_type = ActionType::kVector2;
        const auto action = Build("move.asset", move);
        MappingTable table;
        table.m_mappings = { Map(action, { Binding::WASD() }) };
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
        const ActionReference<Vector2> firstHandle(action);
        const ActionReference<Vector2> secondHandle(action);
        const ActionReference<float> wrongType(action);
        firstInput.Update(first);
        secondInput.Update(second);
        EXPECT_EQ(firstInput.GetFrame().Read(firstHandle), (Vector2{ 0, 1 }));
        EXPECT_EQ(secondInput.GetFrame().Read(secondHandle), (Vector2{ 1, 0 }));
        EXPECT_EQ(secondInput.GetFrame().Read(firstHandle), (Vector2{ 1, 0 }));
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
        mouse.m_source = MouseDeltaBinding{};
        mouse.m_conditions = { Control::MouseButton(1) };
        MappingTable table;
        table.m_mappings = { Map(button, { Button(Core::Key::kW) }), Map(lookLink, { mouse }) };
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
        const ActionReference<Vector2> lookHandle(lookLink);
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


    TEST_F(InputAssets, RepeatedWorldUpdatesAccountForHeldTimeOnlyOnce)
    {
        Action rate;
        rate.m_type = ActionType::kVector2;
        const auto rateLink = Build("rate.asset", rate);
        Action hold;
        auto* processor = Memory::DefaultNew<HoldDurationActionProcessor>();
        processor->m_holdTime = 0.75f;
        hold.m_processor = processor;
        const auto holdLink = Build("hold.asset", hold);
        Binding binding = Binding::WASD();
        binding.m_integrate = true;
        MappingTable table;
        table.m_mappings = { Map(rateLink, { binding }), Map(holdLink, { Button(Core::Key::kW) }) };
        const auto link = Build("table.asset", table);
        (void)Load(link);
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
        backend->m_keyboard.m_keyDownMap.set(87);
        backend->m_samples = { { backend->m_keyboard, {}, 0.5f } };
        backend->m_sequence = 1;
        input.Update(world);
        EXPECT_FLOAT_EQ(input.GetFrame().Read(ActionReference<Vector2>(rateLink)).y, 0.5f);
        Threading::Sleep(20);
        time.Update(world);
        input.Update(world);
        EXPECT_EQ(input.GetFrame().Read(ActionReference<Vector2>(rateLink)), Vector2::kZero);
        EXPECT_TRUE(input.GetFrame().GetEvents().empty());
        backend->m_sequence = 2;
        backend->m_samples.front().m_deltaSeconds = 0.25f;
        input.Update(world);
        EXPECT_FLOAT_EQ(input.GetFrame().Read(ActionReference<Vector2>(rateLink)).y, 0.25f);
        uint32_t triggered = 0;
        for (const ActionEvent& event : input.GetFrame().GetEvents())
        {
            if (event.m_action == holdLink.GetAssetID() && event.m_state == ActionState::kTriggered)
                ++triggered;
        }
        EXPECT_EQ(triggered, 1);
    }


    TEST_F(InputAssets, SerializedServiceConfigurationSurvivesWorldSnapshotRestore)
    {
        const auto action = Build("action.asset", Action{});
        MappingTable table;
        table.m_mappings = { Map(action, { Button(Core::Key::kW) }) };
        const auto link = Build("table.asset", table);
        Framework::EntityWorldAsset definition;
        definition.m_services.push_back(Rtti::GetTypeID<Framework::WorldTimeService>());
        InputConfiguration configuration;
        configuration.m_contexts.push_back({ link, 42, true });
        definition.m_services.push_back(Framework::WorldServiceDescriptor::Create<WorldInputService>(configuration));
        Framework::EntityWorldSnapshotAsset snapshot;
        {
            Framework::EntityWorldInstance instance;
            ASSERT_TRUE(instance.Load(definition));
            auto* input = instance.GetWorld().FindService<WorldInputService>();
            ASSERT_NE(input, nullptr);
            auto* backend = Memory::DefaultNew<FixedBackend>();
            backend->m_keyboard.m_keyDownMap.set(87);
            input->SetBackend(festd::unique_ptr<Core::Backend>(backend));
            const ActionReference<bool> handle(action);
            for (uint32_t i = 0; i < 5000 && !link.GetAssetHandle().Read(); ++i)
            {
                IO::AssetManager::Tick();
                Threading::Sleep(1);
            }

            ASSERT_TRUE(link.GetAssetHandle().Read());
            input->Update(instance.GetWorld());
            EXPECT_TRUE(input->GetFrame().Read(handle));
            ASSERT_TRUE(instance.Capture(snapshot));
            const auto* capturedConfiguration = snapshot.m_services[1].m_configuration.TryGet<InputConfiguration>();
            ASSERT_NE(capturedConfiguration, nullptr);
            ASSERT_EQ(capturedConfiguration->m_contexts.size(), 1);
            EXPECT_EQ(capturedConfiguration->m_contexts[0].m_priority, 42);
            EXPECT_TRUE(capturedConfiguration->m_contexts[0].m_modal);
        }

        Framework::EntityWorldInstance restored;
        ASSERT_TRUE(restored.Restore(snapshot));
        const auto* input = restored.GetWorld().FindService<WorldInputService>();
        ASSERT_NE(input, nullptr);
        ASSERT_EQ(input->GetContexts().size(), 1);
        EXPECT_EQ(input->GetContexts()[0].m_table.GetAssetID(), link.GetAssetID());
    }


    TEST_F(InputAssets, RejectsIncompatibleSerializedServiceConfiguration)
    {
        Framework::EntityWorldAsset definition;
        definition.m_services.push_back(Rtti::GetTypeID<Framework::WorldTimeService>());
        Framework::WorldServiceDescriptor descriptor(Rtti::GetTypeID<WorldInputService>());
        descriptor.m_configuration.Emplace<Action>();
        definition.m_services.push_back(std::move(descriptor));
        Framework::EntityWorldInstance instance;
        EXPECT_FALSE(instance.Load(definition));
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
        table.m_mappings = { Map(action, { rate }) };
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


    TEST_F(InputAssets, NormalizedIntegratedAxesAreInvariantToSampleSubdivision)
    {
        using Framework::Core::PlatformEvent;
        Action action;
        action.m_type = ActionType::kVector2;
        const auto link = Build("action.asset", action);
        Binding binding = Binding::WASD();
        binding.m_sensitivity = 2;
        binding.m_integrate = true;
        MappingTable table;
        table.m_mappings = { Map(link, { binding }) };
        table.m_mappings.front().m_clampMagnitude = true;
        MappingStack first, second;
        const auto handle = Load(Build("table.asset", table));
        (void)first.Activate(handle);
        (void)second.Activate(handle);
        PlatformEvent down[] = { PlatformEvent::KeyboardKeyDown(Core::Key::kW), PlatformEvent::KeyboardKeyDown(Core::Key::kD) };
        for (auto& event : down)
            event.m_timestampSeconds = 99;
        Desktop::Backend whole, split;
        whole.Update(down, 1, 100);
        split.Update(down, 0.5f, 99.5);
        Resolve(first, whole, 1);
        Resolve(second, split, 1);
        const Vector2 partial = second.FindAction(link.GetAssetID())->m_value;
        const PlatformEvent unrelated[] = { PlatformEvent::KeyboardCharInput('x'), PlatformEvent::MouseMove(10, 10) };
        split.Update(unrelated, 0.5f, 100);
        Resolve(second, split, 2);
        const Vector2 subdivided = partial + second.FindAction(link.GetAssetID())->m_value;
        EXPECT_NEAR(Math::Length(first.FindAction(link.GetAssetID())->m_value), 2, 1e-5f);
        EXPECT_TRUE(Math::CmpEqual(subdivided, first.FindAction(link.GetAssetID())->m_value));
    }


    TEST_F(InputAssets, InactiveConditionsAndUnselectedAlternativesDoNotConsume)
    {
        using Framework::Core::PlatformEvent;
        const auto high = Build("high.asset", Action{});
        const auto low = Build("low.asset", Action{});
        const auto lowF1 = Build("lowF1.asset", Action{});
        Binding conditioned = Button(Core::Key::kK);
        conditioned.m_conditions = { Control::Key(Core::Key::kLeftCtrl) };
        MappingTable highTable, lowTable;
        highTable.m_mappings = { Map(high, { conditioned, Button(Core::Key::kF1) }) };
        highTable.m_mappings.front().m_consume = true;
        highTable.m_mappings.front().m_operation = MappingOperation::kStrongest;
        lowTable.m_mappings = { Map(low, { Button(Core::Key::kLeftCtrl) }), Map(lowF1, { Button(Core::Key::kF1) }) };
        MappingStack stack;
        (void)stack.Activate(Load(Build("lowTable.asset", lowTable)));
        (void)stack.Activate(Load(Build("highTable.asset", highTable)), 10);
        Desktop::Backend backend;
        const PlatformEvent ctrl[] = { PlatformEvent::KeyboardKeyDown(Core::Key::kLeftCtrl) };
        backend.Update(ctrl);
        Resolve(stack, backend, 1);
        EXPECT_TRUE(stack.FindAction(low.GetAssetID())->m_active);
        EXPECT_FALSE(stack.FindAction(high.GetAssetID())->m_active);
        const PlatformEvent f1[] = { PlatformEvent::KeyboardKeyDown(Core::Key::kF1) };
        backend.Update(f1);
        Resolve(stack, backend, 2);
        EXPECT_TRUE(stack.FindAction(low.GetAssetID())->m_active);
        EXPECT_TRUE(stack.FindAction(high.GetAssetID())->m_active);
        const PlatformEvent k[] = { PlatformEvent::KeyboardKeyDown(Core::Key::kK) };
        backend.Update(k);
        Resolve(stack, backend, 3);
        EXPECT_FALSE(stack.FindAction(low.GetAssetID())->m_active);
        EXPECT_TRUE(stack.FindAction(lowF1.GetAssetID())->m_active);
    }


    TEST_F(InputAssets, OpposingAndZeroDurationRateControlsStillConsume)
    {
        using Framework::Core::PlatformEvent;
        Action axis;
        axis.m_type = ActionType::kVector2;
        const auto high = Build("axis.asset", axis);
        const auto low = Build("button.asset", Action{});
        Binding integrated = Binding::WASD();
        integrated.m_integrate = true;
        MappingTable highTable, lowTable;
        highTable.m_mappings = { Map(high, { integrated }) };
        highTable.m_mappings.front().m_consume = true;
        lowTable.m_mappings = { Map(low, { Button(Core::Key::kW) }) };
        MappingStack stack;
        (void)stack.Activate(Load(Build("lowTable.asset", lowTable)));
        (void)stack.Activate(Load(Build("highTable.asset", highTable)), 10);
        Desktop::Backend backend;
        const PlatformEvent opposed[] = { PlatformEvent::KeyboardKeyDown(Core::Key::kW),
                                          PlatformEvent::KeyboardKeyDown(Core::Key::kS) };
        backend.Update(opposed);
        Resolve(stack, backend, 1);
        EXPECT_FALSE(stack.FindAction(low.GetAssetID())->m_active);
        const PlatformEvent release[] = { PlatformEvent::KeyboardKeyUp(Core::Key::kS) };
        backend.Update(release);
        Resolve(stack, backend, 2, 0);
        EXPECT_FALSE(stack.FindAction(low.GetAssetID())->m_active);
        EXPECT_EQ(stack.GetFrame().Read(ActionReference<Vector2>(high)), Vector2{});
    }


    TEST_F(InputAssets, AssetBuilderRejectsInvalidMappingConfiguration)
    {
        const IO::Path path = m_directory / "invalid.asset";
        ASSERT_TRUE(AssetBuilder::ImportAsset({ {}, path, Rtti::GetTypeID<MappingTable>() }));
        AssetBuilder::AssetFile file;
        ASSERT_TRUE(AssetBuilder::LoadAssetFile(path, file));
        const auto action = Build("action.asset", Action{});
        MappingTable table;
        table.m_mappings = { Map(action, { Button(Core::Key::kW) }) };
        table.m_mappings.push_back(table.m_mappings.front());
        file.m_artifacts.front().m_buildSettings.Emplace<MappingTable>(table);
        ASSERT_TRUE(AssetBuilder::SaveAssetFile(path, file));
        ASSERT_TRUE(AssetBuilder::ImportAsset({ {}, path, Rtti::GetTypeID<MappingTable>() }));
        EXPECT_FALSE(AssetBuilder::BuildAsset({ path, {}, m_directory }));
    }


    TEST_F(InputAssets, DISABLED_ResolverAndConsumerMeasurement)
    {
        using Framework::Core::PlatformEvent;
        for (const uint32_t actionCount : { 8u, 64u })
        {
            MappingTable table;
            festd::vector<ActionReference<Vector2>> references;
            for (uint32_t i = 0; i < actionCount; ++i)
            {
                Action action;
                action.m_type = ActionType::kVector2;
                const auto link = Build(Fmt::Format("action-{}-{}.asset", actionCount, i).c_str(), action);
                table.m_mappings.push_back(Map(link, { Binding::WASD() }));
                references.push_back(link);
            }
            const auto handle = Load(Build(Fmt::Format("table-{}.asset", actionCount).c_str(), table));
            for (const uint32_t contextCount : { 1u, 4u })
            {
                for (const uint32_t eventMode : { 0u, 1u, 2u })
                {
                    MappingStack stack;
                    for (uint32_t i = 0; i < contextCount; ++i)
                        (void)stack.Activate(handle, static_cast<int32_t>(i));
                    Desktop::Backend backend;
                    const PlatformEvent down[] = { PlatformEvent::KeyboardKeyDown(Core::Key::kW) };
                    backend.Update(down);
                    Resolve(stack, backend, 1);
                    festd::vector<PlatformEvent> events;
                    for (uint32_t i = 0; i < (eventMode == 0 ? 0u : 32u); ++i)
                        events.push_back(eventMode == 1 ? PlatformEvent::KeyboardCharInput('x') : PlatformEvent::MouseMove(i, i));
                    for (uint32_t i = 0; i < 10; ++i)
                    {
                        backend.Update(events, 0.016f, i + 1);
                        Resolve(stack, backend, i + 2);
                    }
                    const ResolvedAction* published = stack.GetActions().data();
                    const uint64_t start = Platform::GetTicks();
                    const uint64_t allocationStart = Memory::GetThreadAllocationCount();
                    for (uint32_t i = 0; i < 1000; ++i)
                    {
                        backend.Update(events, 0.016f, i + 20);
                        Resolve(stack, backend, i + 20);
                    }
                    const double resolverMicros = (Platform::GetTicks() - start) * Platform::GetSecondsPerTick() * 1000;
                    const uint64_t resolverAllocations = Memory::GetThreadAllocationCount() - allocationStart;
                    EXPECT_EQ(stack.GetActions().data(), published);
                    const uint64_t readStart = Platform::GetTicks();
                    const uint64_t readAllocationStart = Memory::GetThreadAllocationCount();
                    volatile float sink = 0;
                    for (uint32_t i = 0; i < 200000; ++i)
                        sink = sink + stack.GetFrame().Read(references[i % references.size()]).y;
                    const double readNanos = (Platform::GetTicks() - readStart) * Platform::GetSecondsPerTick() * 1e9 / 200000;
                    const uint64_t readAllocations = Memory::GetThreadAllocationCount() - readAllocationStart;
                    EXPECT_EQ(resolverAllocations, 0);
                    EXPECT_EQ(readAllocations, 0);
                    IO::PrintLn(
                        "Input measurement: actions={} contexts={} eventMode={} samples={} resolverUs={} readNs={} resolverAllocations={} readAllocations={} sink={}",
                        actionCount,
                        contextCount,
                        eventMode,
                        backend.GetSamples().size(),
                        resolverMicros,
                        readNanos,
                        resolverAllocations,
                        readAllocations,
                        static_cast<float>(sink));
                }
            }
        }
    }


    TEST(InputValidation, RejectsDuplicateRowsInvalidControlsTypesAndParameters)
    {
        ActionMapping mapping = Map(IO::Link<Action>(Uuid::Random()), { Binding::WASD() });
        MappingTable table;
        table.m_mappings = { mapping };
        EXPECT_TRUE(table.Validate());
        table.m_mappings.push_back(mapping);
        EXPECT_FALSE(table.Validate());
        table.m_mappings.pop_back();
        EXPECT_FALSE(table.m_mappings[0].m_bindings[0].Supports(ActionType::kBoolean));
        std::get<Axis2DBinding>(table.m_mappings[0].m_bindings[0].m_source).m_left = {};
        EXPECT_FALSE(table.Validate());
        Action hold;
        auto processor = Rc<HoldDurationActionProcessor>(Memory::DefaultNew<HoldDurationActionProcessor>());
        processor->m_holdTime = -1;
        hold.m_processor = processor;
        EXPECT_FALSE(hold.Validate());
        processor->m_holdTime = std::bit_cast<float>(0x7f800000u);
        EXPECT_FALSE(hold.Validate());
        processor->m_holdTime = std::bit_cast<float>(0x7fc00001u);
        EXPECT_FALSE(hold.Validate());
        Binding finite = Binding::WASD();
        finite.m_sensitivity = std::bit_cast<float>(0x7f800000u);
        EXPECT_FALSE(finite.Validate());
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
