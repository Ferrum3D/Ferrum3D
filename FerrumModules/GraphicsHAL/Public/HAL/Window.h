#pragma once
#include <FeCore/Base/PlatformInclude.h>
#include <HAL/Common/BaseTypes.h>
#include <HAL/IWindow.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace FE::Graphics::HAL
{
    class Window : public IWindow
    {
        GLFWwindow* m_Window{};
        HWND m_Handle{};
        String m_Title{};

    public:
        FE_RTTI_Class(Window, "CB4F9E4C-6BF5-4C43-B471-0EF42990B409");

        ResultCode Init(const WindowDesc& desc) override;

        void PollEvents() override;
        bool CloseRequested() override;
        void* GetNativeHandle() override;
        Viewport CreateViewport() override;
        Scissor CreateScissor() override;
    };
} // namespace FE::Graphics::HAL
