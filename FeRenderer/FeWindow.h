#pragma once
#include "FeRenderAPI.h"
#include "FeRenderInternal.h"
#include "IFeWindow.h"
#include <DiligentCore/Platforms/Win32/interface/Win32NativeWindow.h>
#include <GLFW/glfw3.h>
#include <cstdint>

namespace FE
{
    class FeWindow : public IFeWindow
    {
        uint32_t m_Width;
        uint32_t m_Height;
        GLFWwindow* m_Window{};

    public:
        FeWindow(uint32_t width, uint32_t height);
        virtual void Init() override;
        virtual void PollEvents() override;
        virtual bool ShouldClose() override;
        virtual void Resize(uint32_t width, uint32_t height) override;
        virtual void Close() override;

        Diligent::Win32NativeWindow GetNativeWindow() const;
    };
} // namespace FE
