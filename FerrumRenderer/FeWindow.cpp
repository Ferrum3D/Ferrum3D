#include "FeWindow.h"
#include <FerrumCore/FeLog.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace Ferrum
{
	FeWindow::FeWindow(uint32_t width, uint32_t height) {
		m_Width = width;
		m_Height = height;
	}

	void FeWindow::Init() {
		FE_ASSERT_MSG(glfwInit(), "GLFW initialization failed");
		glfwWindowHint(GLFW_RESIZABLE, 0);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_Window = glfwCreateWindow(m_Width, m_Height, "Test", nullptr, nullptr);
	}

	void FeWindow::PollEvents() {
		glfwPollEvents();
	}

	bool FeWindow::ShouldClose() {
		return glfwWindowShouldClose(m_Window);
	}

	void FeWindow::Resize(uint32_t width, uint32_t height) {
		glfwSetWindowSize(m_Window, width, height);
	}

	void FeWindow::Close() {
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	Diligent::Win32NativeWindow FeWindow::GetNativeWindow() const {
		auto window = glfwGetWin32Window(m_Window);
		return Diligent::Win32NativeWindow(window);
	}
}
