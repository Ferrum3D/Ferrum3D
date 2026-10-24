#include <Core/Base/PlatformInclude.h>
#include <Framework/Application/Core/PlatformEvent.h>
#include <Framework/ImGui/ImGuiPlatform.h>
#include <Framework/Input/Desktop/Backend.h>
#include <gtest/gtest.h>
#include <imgui.h>

using namespace FE;
using namespace FE::Framework;
using Input::Core::Key;

namespace
{
    struct FakePlatformWindow final : Core::PlatformWindow
    {
        explicit FakePlatformWindow(Core::PlatformApplication* application)
            : m_application(application)
        {
        }


        Core::NativeWindowHandle GetNativeHandle() const override
        {
            return Core::NativeWindowHandle{ 1 };
        }


        Core::PlatformApplication* GetApplication() const override
        {
            return m_application;
        }


        void Show(Core::PlatformWindowShowMode) override {}
        void SetFocus() override {}
        void SetParent(Core::PlatformWindow*) override {}
        void UpdateStyle(Core::PlatformWindowStyleFlags) override {}
        void SetTitle(festd::string_view) override {}
        void SetPosition(Vector2Int) override {}
        void SetSize(Vector2Int) override {}


        bool IsFocused() const override
        {
            return true;
        }


        Core::PlatformWindowShowMode GetShowMode() const override
        {
            return Core::PlatformWindowShowMode::kShow;
        }


        uint32_t GetMonitorIndex() const override
        {
            return 0;
        }


        Vector2Int GetPosition() const override
        {
            return Vector2Int(0, 0);
        }


        Vector2Int GetClientSize() const override
        {
            return Vector2Int(800, 600);
        }


        RectInt GetClientRect() const override
        {
            return RectInt::FromPosAndSize(Vector2Int(0, 0), GetClientSize());
        }

    protected:
        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }

    private:
        Core::PlatformApplication* m_application;
    };


    //! Delivers scripted events, one batch per frame, in place of the Windows message loop.
    struct FakePlatformApplication final : Core::PlatformApplication
    {
        festd::vector<Core::PlatformEvent> m_events;

        Core::AlertResponse ShowAlert(Core::AlertKind, festd::string_view, festd::string_view, Core::AlertButtons) override
        {
            return Core::AlertResponse::kNone;
        }


        Core::PlatformWindow* CreateWindow(const Core::PlatformWindowDesc&) override
        {
            return nullptr;
        }


        void PollEvents() override {}


        const Input::Core::Backend& GetInputBackend() const override
        {
            return m_inputBackend;
        }


        void SetCursor(Core::PlatformCursor) override {}
        void SetInputCapture(bool, bool) override {}


        bool IsCloseRequested() const override
        {
            return false;
        }


        festd::span<const Core::PlatformEvent> GetEvents() const override
        {
            return m_events;
        }


        festd::span<const Core::PlatformMonitorInfo> GetMonitors() const override
        {
            return {};
        }


        festd::span<Core::PlatformWindow* const> GetWindows() const override
        {
            return {};
        }


        Core::PlatformWindow* GetMainWindow() const override
        {
            return nullptr;
        }

    protected:
        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }

    private:
        Input::Desktop::Backend m_inputBackend;
    };


    struct ImGuiInputTest : testing::Test
    {
        void SetUp() override
        {
            m_application = Memory::DefaultNew<FakePlatformApplication>();
            m_window = Memory::DefaultNew<FakePlatformWindow>(m_application.Get());
            m_platform = festd::make_unique<ImGuiPlatform>(m_application.Get(), m_window.Get());
            ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
        }


        void TearDown() override
        {
            m_platform.reset();
        }


        //! Run one UI frame over the given platform events.
        void Frame(std::initializer_list<Core::PlatformEvent> events)
        {
            m_application->m_events.assign(events.begin(), events.end());
            m_platform->NewFrame();
            m_platform->EndFrame();
            m_application->m_events.clear();
        }


        Rc<FakePlatformApplication> m_application;
        Rc<FakePlatformWindow> m_window;
        festd::unique_ptr<ImGuiPlatform> m_platform;
    };
} // namespace


TEST_F(ImGuiInputTest, ModifierReleasedWithBothSidesReportedDoesNotStick)
{
    // Windows reports a release of either side as releases of both sides.
    Frame({ Core::PlatformEvent::KeyboardKeyDown(Key::kLeftCtrl) });
    Frame({});
    EXPECT_TRUE(ImGui::GetIO().KeyCtrl);

    Frame({ Core::PlatformEvent::KeyboardKeyUp(Key::kLeftCtrl), Core::PlatformEvent::KeyboardKeyUp(Key::kRightCtrl) });
    Frame({});
    EXPECT_FALSE(ImGui::GetIO().KeyCtrl);

    Frame({ Core::PlatformEvent::KeyboardKeyDown(Key::kLeftShift) });
    Frame({});
    EXPECT_TRUE(ImGui::GetIO().KeyShift);

    Frame({ Core::PlatformEvent::KeyboardKeyUp(Key::kLeftShift), Core::PlatformEvent::KeyboardKeyUp(Key::kRightShift) });
    Frame({});
    EXPECT_FALSE(ImGui::GetIO().KeyShift);
}


TEST_F(ImGuiInputTest, ModifierStaysHeldWhileTheOtherSideIsDown)
{
    Frame({ Core::PlatformEvent::KeyboardKeyDown(Key::kLeftShift) });
    Frame({ Core::PlatformEvent::KeyboardKeyDown(Key::kRightShift) });
    Frame({ Core::PlatformEvent::KeyboardKeyUp(Key::kLeftShift) });
    Frame({});
    EXPECT_TRUE(ImGui::GetIO().KeyShift);

    Frame({ Core::PlatformEvent::KeyboardKeyUp(Key::kRightShift) });
    Frame({});
    EXPECT_FALSE(ImGui::GetIO().KeyShift);
}


TEST_F(ImGuiInputTest, FocusLossForgetsModifiersReleasedElsewhere)
{
    Frame({ Core::PlatformEvent::KeyboardKeyDown(Key::kLeftCtrl) });
    Frame({ Core::PlatformEvent::WindowFocusChanged(m_window.Get(), false) });
    Frame({ Core::PlatformEvent::WindowFocusChanged(m_window.Get(), true) });

    // The left release went to another application; a later right tap must release the modifier.
    Frame({ Core::PlatformEvent::KeyboardKeyDown(Key::kRightCtrl) });
    Frame({ Core::PlatformEvent::KeyboardKeyUp(Key::kRightCtrl) });
    Frame({});
    EXPECT_FALSE(ImGui::GetIO().KeyCtrl);
}


TEST_F(ImGuiInputTest, HorizontalWheelReachesImGui)
{
    // Windows reports scrolling right as positive; ImGui scrolls right for negative values.
    m_application->m_events.push_back(Core::PlatformEvent::MouseWheel(0.0f, 1.0f));
    m_platform->NewFrame();
    EXPECT_EQ(ImGui::GetIO().MouseWheelH, -1.0f);
    EXPECT_EQ(ImGui::GetIO().MouseWheel, 0.0f);
    m_platform->EndFrame();
}


TEST(PlatformApplicationEvents, MessagesSentBetweenPollsAreDelivered)
{
    Core::PlatformApplication::Init();
    const auto shutdown = festd::defer([] {
        Core::PlatformApplication::Shutdown();
    });

    Core::PlatformApplication& application = Core::PlatformApplication::Get();
    Core::PlatformWindowDesc desc;
    desc.m_rect = RectInt::FromPosAndSize(Vector2Int(0, 0), Vector2Int(64, 64));
    desc.m_title = "PlatformApplicationEvents";
    const Rc<Core::PlatformWindow> window = application.CreateWindow(desc);
    const auto hwnd = reinterpret_cast<HWND>(window->GetNativeHandle().m_value);
    application.PollEvents();

    // Focus changes from SetFocus or window activation are sent synchronously, outside the message loop.
    SendMessageW(hwnd, WM_KILLFOCUS, 0, 0);
    const auto countUnfocus = [&] {
        uint32_t count = 0;
        for (const Core::PlatformEvent& event : application.GetEvents())
        {
            if (event.m_type == Core::PlatformEventType::kWindowUnfocus)
                ++count;
        }

        return count;
    };

    EXPECT_EQ(countUnfocus(), 0u);

    application.PollEvents();
    EXPECT_EQ(countUnfocus(), 1u);

    application.PollEvents();
    EXPECT_EQ(countUnfocus(), 0u);
}
