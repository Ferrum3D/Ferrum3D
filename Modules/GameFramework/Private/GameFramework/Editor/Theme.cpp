#include <GameFramework/Editor/Theme.h>
#include <cstdint>
#include <imgui.h>

namespace FE::Editor
{
    namespace
    {
        constexpr ImVec4 Rgb(uint32_t hex, float alpha = 1.0f)
        {
            return ImVec4(static_cast<float>((hex >> 16) & 0xff) / 255.0f,
                          static_cast<float>((hex >> 8) & 0xff) / 255.0f,
                          static_cast<float>(hex & 0xff) / 255.0f,
                          alpha);
        }


        constexpr ImVec4 WithAlpha(ImVec4 color, float alpha)
        {
            color.w = alpha;
            return color;
        }


        constexpr ImVec4 kCrust = Rgb(0x1c1917);
        constexpr ImVec4 kMantle = Rgb(0x231f1c);
        constexpr ImVec4 kBase = Rgb(0x292522);
        constexpr ImVec4 kSurface0 = Rgb(0x34302c);
        constexpr ImVec4 kSurface1 = Rgb(0x403a36);
        constexpr ImVec4 kSurface2 = Rgb(0x58504a);
        constexpr ImVec4 kOverlay0 = Rgb(0x716355);
        constexpr ImVec4 kOverlay1 = Rgb(0x867462);
        constexpr ImVec4 kText = Rgb(0xece1d7);

        constexpr ImVec4 kOrange = Rgb(0xf4ad68);
        constexpr ImVec4 kYellow = Rgb(0xf7d07c);
        constexpr ImVec4 kCyan = Rgb(0x9ed2d5);
        constexpr ImVec4 kBlue = Rgb(0xb8bfec);
        constexpr ImVec4 kBlueDeep = Rgb(0x273142);

        void ApplyImGui(float scale)
        {
            ImGuiStyle& style = ImGui::GetStyle();
            style = ImGuiStyle{};
            ImGui::StyleColorsDark(&style);
            style.FontSizeBase = 16.0f;

            style.WindowPadding = ImVec2(10.0f, 10.0f);
            style.FramePadding = ImVec2(8.0f, 5.0f);
            style.ItemSpacing = ImVec2(8.0f, 6.0f);
            style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
            style.WindowRounding = 5.0f;
            style.ChildRounding = 4.0f;
            style.FrameRounding = 4.0f;
            style.PopupRounding = 5.0f;
            style.ScrollbarRounding = 5.0f;
            style.GrabRounding = 4.0f;
            style.TabRounding = 4.0f;
            style.WindowBorderSize = 1.0f;
            style.ChildBorderSize = 1.0f;
            style.PopupBorderSize = 1.0f;
            style.FrameBorderSize = 0.0f;
            style.TabBorderSize = 0.0f;
            style.TabBarBorderSize = 1.0f;
            style.ScaleAllSizes(scale);
            style.FontScaleDpi = scale;

            ImVec4* colors = style.Colors;
            colors[ImGuiCol_Text] = kText;
            colors[ImGuiCol_TextDisabled] = kOverlay0;
            colors[ImGuiCol_WindowBg] = kBase;
            colors[ImGuiCol_ChildBg] = kBase;
            colors[ImGuiCol_PopupBg] = kSurface0;
            colors[ImGuiCol_Border] = kSurface2;
            colors[ImGuiCol_BorderShadow] = WithAlpha(kCrust, 0.0f);
            colors[ImGuiCol_FrameBg] = kSurface0;
            colors[ImGuiCol_FrameBgHovered] = kSurface1;
            colors[ImGuiCol_FrameBgActive] = kSurface2;
            colors[ImGuiCol_TitleBg] = kMantle;
            colors[ImGuiCol_TitleBgActive] = kSurface0;
            colors[ImGuiCol_TitleBgCollapsed] = kCrust;
            colors[ImGuiCol_MenuBarBg] = kMantle;
            colors[ImGuiCol_ScrollbarBg] = kMantle;
            colors[ImGuiCol_ScrollbarGrab] = kSurface2;
            colors[ImGuiCol_ScrollbarGrabHovered] = kOverlay0;
            colors[ImGuiCol_ScrollbarGrabActive] = kOverlay1;
            colors[ImGuiCol_CheckMark] = kBlue;
            colors[ImGuiCol_CheckboxSelectedBg] = kSurface1;
            colors[ImGuiCol_SliderGrab] = kOverlay1;
            colors[ImGuiCol_SliderGrabActive] = kBlue;
            colors[ImGuiCol_Button] = kSurface0;
            colors[ImGuiCol_ButtonHovered] = kSurface1;
            colors[ImGuiCol_ButtonActive] = kSurface2;
            colors[ImGuiCol_Header] = kSurface1;
            colors[ImGuiCol_HeaderHovered] = kSurface2;
            colors[ImGuiCol_HeaderActive] = kSurface2;
            colors[ImGuiCol_Separator] = kSurface2;
            colors[ImGuiCol_SeparatorHovered] = kOverlay0;
            colors[ImGuiCol_SeparatorActive] = kBlue;
            colors[ImGuiCol_ResizeGrip] = kSurface2;
            colors[ImGuiCol_ResizeGripHovered] = kOverlay0;
            colors[ImGuiCol_ResizeGripActive] = kBlue;
            colors[ImGuiCol_InputTextCursor] = kText;
            colors[ImGuiCol_TabHovered] = kSurface1;
            colors[ImGuiCol_Tab] = kMantle;
            colors[ImGuiCol_TabSelected] = kSurface0;
            colors[ImGuiCol_TabSelectedOverline] = kBlue;
            colors[ImGuiCol_TabDimmed] = kCrust;
            colors[ImGuiCol_TabDimmedSelected] = kMantle;
            colors[ImGuiCol_TabDimmedSelectedOverline] = kSurface2;
            colors[ImGuiCol_DockingPreview] = WithAlpha(kBlue, 0.35f);
            colors[ImGuiCol_DockingEmptyBg] = kCrust;
            colors[ImGuiCol_PlotLines] = kCyan;
            colors[ImGuiCol_PlotLinesHovered] = kBlue;
            colors[ImGuiCol_PlotHistogram] = kYellow;
            colors[ImGuiCol_PlotHistogramHovered] = kOrange;
            colors[ImGuiCol_TableHeaderBg] = kMantle;
            colors[ImGuiCol_TableBorderStrong] = kSurface2;
            colors[ImGuiCol_TableBorderLight] = kSurface1;
            colors[ImGuiCol_TableRowBg] = WithAlpha(kBase, 0.0f);
            colors[ImGuiCol_TableRowBgAlt] = WithAlpha(kSurface0, 0.4f);
            colors[ImGuiCol_TextLink] = kBlue;
            colors[ImGuiCol_TextSelectedBg] = WithAlpha(kSurface1, 0.9f);
            colors[ImGuiCol_TreeLines] = kSurface2;
            colors[ImGuiCol_DragDropTarget] = kBlue;
            colors[ImGuiCol_DragDropTargetBg] = WithAlpha(kBlueDeep, 0.7f);
            colors[ImGuiCol_UnsavedMarker] = kOrange;
            colors[ImGuiCol_NavCursor] = kBlue;
            colors[ImGuiCol_NavWindowingHighlight] = WithAlpha(kText, 0.7f);
            colors[ImGuiCol_NavWindowingDimBg] = WithAlpha(kCrust, 0.5f);
            colors[ImGuiCol_ModalWindowDimBg] = WithAlpha(kCrust, 0.6f);
        }

    } // namespace


    void ApplyTheme(const float dpiScale)
    {
        ApplyImGui(dpiScale);
    }
} // namespace FE::Editor
