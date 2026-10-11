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
#include <Framework/Input/Desktop/Backend.h>
#include <Framework/Input/MappingStack.h>
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


    TEST_F(InputAssets, ContinuousActionStopsAfterKeyRelease)
    {
        Action move;
        move.m_type = ActionType::kVector2;
        move.m_processor = Memory::DefaultNew<ContinuousActionProcessor>();
        const auto moveLink = Build("move.asset", move);
        MappingTable table;
        ActionMapping mapping;
        mapping.m_action = moveLink;
        mapping.m_keyboard = { { Core::Key::kW, { 0, 1 } } };
        table.m_mappings.push_back(mapping);
        MappingStack stack;
        stack.PushTable(Load(Build("table.asset", table)), MappingStackFlags::kEnabled);
        Desktop::Backend backend;
        using Framework::Core::PlatformEvent;
        const PlatformEvent press[] = { PlatformEvent::KeyboardKeyDown(Core::Key::kW) };
        const PlatformEvent release[] = { PlatformEvent::KeyboardKeyUp(Core::Key::kW) };
        backend.Update(press);
        (void)stack.Resolve(backend.GetKeyboardState(), backend.GetMouseState(), 0.1f, 1);
        EXPECT_TRUE(stack.FindAction(moveLink.GetAssetID())->m_active);
        backend.Update(release);
        (void)stack.Resolve(backend.GetKeyboardState(), backend.GetMouseState(), 0.1f, 2);
        const auto* result = stack.FindAction(moveLink.GetAssetID());
        EXPECT_FALSE(result->m_active);
        EXPECT_EQ(result->m_value, Vector2::kZero);
        EXPECT_EQ(result->m_state, ActionState::kFinished);
        EXPECT_TRUE(result->m_hasEvent);
        backend.Update({});
        (void)stack.Resolve(backend.GetKeyboardState(), backend.GetMouseState(), 0.1f, 3);
        EXPECT_FALSE(stack.FindAction(moveLink.GetAssetID())->m_active);
        EXPECT_FALSE(stack.FindAction(moveLink.GetAssetID())->m_hasEvent);
    }


    TEST_F(InputAssets, PriorityConsumptionNormalizationAndFrameCache)
    {
        Action move;
        move.m_type = ActionType::kVector2;
        move.m_processor = Memory::DefaultNew<ContinuousActionProcessor>();
        const auto moveLink = Build("move.asset", move);
        Action alternate;
        alternate.m_flags = ActionFlags::kConsume;
        const auto alternateLink = Build("alternate.asset", alternate);
        MappingTable low;
        ActionMapping mapping;
        mapping.m_action = moveLink;
        mapping.m_keyboard = { { Core::Key::kW, { 0, 1 } }, { Core::Key::kD, { 1, 0 } } };
        mapping.m_flags = MappingFlags::kNormalizeDirection;
        low.m_mappings.push_back(mapping);
        const auto lowHandle = Load(Build("low.asset", low));
        MappingTable high;
        mapping.m_action = alternateLink;
        mapping.m_keyboard = { { Core::Key::kW, { 1, 0 } } };
        high.m_mappings.push_back(mapping);
        const auto highHandle = Load(Build("high.asset", high));
        MappingStack stack;
        stack.PushTable(lowHandle, MappingStackFlags::kEnabled);
        stack.PushTable(highHandle);
        Core::KeyboardState keyboard;
        Core::MouseState mouse;
        keyboard.m_keyDownMap.set(87).set(68);
        keyboard.m_keyPressMap = keyboard.m_keyDownMap;
        (void)stack.Resolve(keyboard, mouse, 0.1f, 1);
        const auto* result = stack.FindAction(moveLink.GetAssetID());
        ASSERT_NE(result, nullptr);
        EXPECT_NEAR(result->m_value.x, 0.7071067f, 0.00001f);
        EXPECT_EQ(result->m_state, ActionState::kStarted);
        (void)stack.Resolve(keyboard, mouse, 0.1f, 1);
        EXPECT_EQ(stack.FindAction(moveLink.GetAssetID())->m_state, ActionState::kStarted);
        stack.Enable(highHandle);
        (void)stack.Resolve(keyboard, mouse, 0.1f, 2);
        EXPECT_FLOAT_EQ(stack.FindAction(moveLink.GetAssetID())->m_value.x, 1);
        EXPECT_FLOAT_EQ(stack.FindAction(moveLink.GetAssetID())->m_value.y, 0);
        EXPECT_TRUE(stack.FindAction(alternateLink.GetAssetID())->m_hasEvent);
        stack.Disable(highHandle);
        (void)stack.Resolve(keyboard, mouse, 0.1f, 3);
        EXPECT_NEAR(stack.FindAction(moveLink.GetAssetID())->m_value.y, 0.7071067f, 0.00001f);
    }


    TEST_F(InputAssets, HoldStateIsIndependentAndFinishesOnce)
    {
        Action action;
        auto* hold = Memory::DefaultNew<HoldDurationActionProcessor>();
        hold->m_holdTime = 0.25f;
        action.m_processor = hold;
        const auto actionLink = Build("hold.asset", action);
        MappingTable table;
        ActionMapping mapping;
        mapping.m_action = actionLink;
        mapping.m_keyboard = { { Core::Key::kE, { 1, 0 } } };
        table.m_mappings.push_back(mapping);
        const auto handle = Load(Build("table.asset", table));
        MappingStack first, second;
        first.PushTable(handle, MappingStackFlags::kEnabled);
        second.PushTable(handle, MappingStackFlags::kEnabled);
        Core::KeyboardState keyboard;
        Core::MouseState mouse;
        keyboard.m_keyDownMap.set(69);
        (void)first.Resolve(keyboard, mouse, 0.1f, 1);
        EXPECT_EQ(first.FindAction(actionLink.GetAssetID())->m_state, ActionState::kStarted);
        (void)first.Resolve(keyboard, mouse, 0.1f, 2);
        EXPECT_EQ(first.FindAction(actionLink.GetAssetID())->m_state, ActionState::kInProgress);
        (void)second.Resolve(keyboard, mouse, 0.1f, 2);
        EXPECT_EQ(second.FindAction(actionLink.GetAssetID())->m_state, ActionState::kStarted);
        (void)first.Resolve(keyboard, mouse, 0.1f, 3);
        EXPECT_EQ(first.FindAction(actionLink.GetAssetID())->m_state, ActionState::kFinished);
        EXPECT_TRUE(first.FindAction(actionLink.GetAssetID())->m_hasEvent);
        (void)first.Resolve(keyboard, mouse, 0.1f, 4);
        EXPECT_FALSE(first.FindAction(actionLink.GetAssetID())->m_hasEvent);
        keyboard.m_keyDownMap.reset();
        (void)second.Resolve(keyboard, mouse, 0.1f, 3);
        EXPECT_EQ(second.FindAction(actionLink.GetAssetID())->m_state, ActionState::kCanceled);
        EXPECT_TRUE(second.FindAction(actionLink.GetAssetID())->m_hasEvent);
        (void)second.Resolve(keyboard, mouse, 0.1f, 4);
        EXPECT_FALSE(second.FindAction(actionLink.GetAssetID())->m_hasEvent);
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

    TEST_F(InputAssets, DefaultPressChordMouseGateAndPriorityOverride)
    {
        Action action;
        const auto actionLink = Build("press.asset", action);
        MappingTable low, high;
        ActionMapping mapping;
        mapping.m_action = actionLink;
        mapping.m_keyboard = { { Core::Key::kQ, { 1, 0 } }, { Core::Key::kE, { 1, 0 } } };
        mapping.m_operation = MappingOperation::kAll;
        low.m_mappings.push_back(mapping);
        mapping.m_keyboard = { { Core::Key::kW, { 1, 0 } } };
        high.m_mappings.push_back(mapping);
        const auto lowHandle = Load(Build("low.asset", low));
        const auto highHandle = Load(Build("high.asset", high));
        MappingStack stack;
        stack.PushTable(lowHandle, MappingStackFlags::kEnabled);
        Core::KeyboardState keyboard;
        Core::MouseState mouse;
        keyboard.m_keyDownMap.set(81);
        keyboard.m_keyPressMap.set(81);
        (void)stack.Resolve(keyboard, mouse, 0.1f, 1);
        EXPECT_FALSE(stack.FindAction(actionLink.GetAssetID())->m_hasEvent);
        keyboard.m_keyDownMap.set(69);
        keyboard.m_keyPressMap.reset().set(69);
        (void)stack.Resolve(keyboard, mouse, 0.1f, 2);
        EXPECT_TRUE(stack.FindAction(actionLink.GetAssetID())->m_hasEvent);
        EXPECT_EQ(stack.FindAction(actionLink.GetAssetID())->m_state, ActionState::kFinished);
        keyboard.m_keyPressMap.reset();
        (void)stack.Resolve(keyboard, mouse, 0.1f, 3);
        EXPECT_FALSE(stack.FindAction(actionLink.GetAssetID())->m_hasEvent);
        stack.PushTable(highHandle, MappingStackFlags::kEnabled);
        (void)stack.Resolve(keyboard, mouse, 0.1f, 4);
        EXPECT_FALSE(stack.FindAction(actionLink.GetAssetID())->m_active);
        stack.RemoveTable(highHandle);
        (void)stack.Resolve(keyboard, mouse, 0.1f, 5);
        EXPECT_TRUE(stack.FindAction(actionLink.GetAssetID())->m_active);

        Action look;
        look.m_type = ActionType::kVector2;
        look.m_processor = Memory::DefaultNew<ContinuousActionProcessor>();
        const auto lookLink = Build("look.asset", look);
        MappingTable mouseTable;
        mapping.m_action = lookLink;
        mapping.m_keyboard.clear();
        mapping.m_operation = MappingOperation::kSum;
        mapping.m_mouse = { { MouseSource::kDeltaX, 0, { 1, 0 } }, { MouseSource::kDeltaY, 0, { 0, 1 } } };
        mapping.m_mouseButtonGate = 1;
        mouseTable.m_mappings.push_back(mapping);
        stack.PushTable(Load(Build("mouse.asset", mouseTable)), MappingStackFlags::kEnabled);
        mouse.m_delta = { 8, -3 };
        (void)stack.Resolve(keyboard, mouse, 0.1f, 6);
        EXPECT_FALSE(stack.FindAction(lookLink.GetAssetID())->m_active);
        mouse.m_buttonDownMap.set(1);
        (void)stack.Resolve(keyboard, mouse, 0.1f, 7);
        EXPECT_FLOAT_EQ(stack.FindAction(lookLink.GetAssetID())->m_value.x, 8);
        EXPECT_FLOAT_EQ(stack.FindAction(lookLink.GetAssetID())->m_value.y, -3);
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
