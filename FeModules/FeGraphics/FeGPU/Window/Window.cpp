#include <FeGPU/Window/Window.h>

namespace FE::GPU
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

    Window::Window(const WindowDesc& desc)
        : m_Title(desc.Title)
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_Window = glfwCreateWindow(desc.Width, desc.Height, m_Title.Data(), nullptr, nullptr);
        m_Handle = glfwGetWin32Window(m_Window);
    }

    Viewport Window::CreateViewport()
    {
        int width, height;
        glfwGetWindowSize(m_Window, &width, &height);
        return Viewport(0, static_cast<float>(width), 0, static_cast<float>(height));
    }

    Scissor Window::CreateScissor()
    {
        return Scissor(CreateViewport());
    }
}
