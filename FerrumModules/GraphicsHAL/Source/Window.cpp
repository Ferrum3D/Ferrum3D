#include <HAL/Window.h>

namespace FE::Graphics::HAL
{
    void Window::PollEvents()
    {
        glfwPollEvents();
    }

    bool Window::CloseRequested()
    {
        return glfwWindowShouldClose(m_Window);
    }

    void* Window::GetNativeHandle()
    {
        return static_cast<void*>(m_Handle);
    }

    ResultCode Window::Init(const WindowDesc& desc)
    {
        m_Title = desc.Title;
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_Window =
            glfwCreateWindow(static_cast<int>(desc.Width), static_cast<int>(desc.Height), m_Title.Data(), nullptr, nullptr);

        if (m_Window == nullptr)
            return ResultCode::UnknownError;

        m_Handle = glfwGetWin32Window(m_Window);
        return ResultCode::Success;
    }

    Viewport Window::CreateViewport()
    {
        int width, height;
        glfwGetWindowSize(m_Window, &width, &height);
        return { 0, static_cast<float>(width), 0, static_cast<float>(height) };
    }

    Scissor Window::CreateScissor()
    {
        return Scissor(CreateViewport());
    }
} // namespace FE::Graphics::HAL
