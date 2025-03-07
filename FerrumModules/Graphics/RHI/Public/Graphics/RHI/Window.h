﻿#pragma once
#include <FeCore/Base/PlatformInclude.h>
#include <Graphics/RHI/Common/BaseTypes.h>
#include <Graphics/RHI/IWindow.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace FE::Graphics::RHI
{
    struct Window : public IWindow
    {
        FE_RTTI_Class(Window, "CB4F9E4C-6BF5-4C43-B471-0EF42990B409");

        ResultCode Init(const WindowDesc& desc) override;

        void PollEvents() override;
        bool CloseRequested() override;
        void* GetNativeHandle() override;
        Viewport CreateViewport() override;
        Scissor CreateScissor() override;

    private:
        GLFWwindow* m_window = nullptr;
        HWND m_handle = nullptr;
        String m_title;
    };
} // namespace FE::Graphics::RHI
