#include <Graphics/RHI/Window.h>

namespace FE::Graphics::RHI
{
    void Window::PollEvents()
    {
        glfwPollEvents();
    }


    bool Window::CloseRequested()
    {
        return glfwWindowShouldClose(m_window);
    }


    void* Window::GetNativeHandle()
    {
        return static_cast<void*>(m_handle);
    }


    ResultCode Window::Init(const WindowDesc& desc)
    {
        m_title = desc.m_title;
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_window =
            glfwCreateWindow(static_cast<int>(desc.m_width), static_cast<int>(desc.m_height), m_title.Data(), nullptr, nullptr);

        if (m_window == nullptr)
            return ResultCode::kUnknownError;

        m_handle = glfwGetWin32Window(m_window);
        return ResultCode::kSuccess;
    }


    Viewport Window::CreateViewport()
    {
        int width, height;
        glfwGetWindowSize(m_window, &width, &height);
        return { 0, static_cast<float>(width), 0, static_cast<float>(height) };
    }


    Scissor Window::CreateScissor()
    {
        return Scissor(CreateViewport());
    }
} // namespace FE::Graphics::RHI
