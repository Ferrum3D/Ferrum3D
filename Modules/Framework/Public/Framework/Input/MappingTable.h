#pragma once
#include <Framework/Input/Action.h>
#include <Framework/Input/Core/Keys.h>

namespace FE::Input
{
    struct KeyboardMapping final
    {
        FE_RTTI_Reflect("720B53E8-835A-4274-976D-7A132E85FF32");
        FE_RTTI_Serialize();

        Core::Key m_key = Core::Key::kInvalid;
        Vector2 m_value{ 1, 0 };
    };


    enum class MouseSource : uint32_t
    {
        kButton,
        kDeltaX,
        kDeltaY,
        kWheel,
    };


    struct MouseMapping final
    {
        FE_RTTI_Reflect("42F20FE1-974A-4A10-B914-186B5FFDD6B3");
        FE_RTTI_Serialize();

        MouseSource m_source = MouseSource::kButton;
        uint32_t m_button = 0;
        Vector2 m_value{ 1, 0 };
    };


    enum class MappingFlags : uint32_t
    {
        kNone = 0,
        kNormalizeDirection = 1 << 0,
    };

    FE_ENUM_OPERATORS(MappingFlags);


    enum class MappingOperation : uint32_t
    {
        kSum,
        kAny,
        kAll,
    };


    struct ActionMapping final
    {
        FE_RTTI_Reflect("16158613-2631-4F6F-A38E-43FBFADE7D40");
        FE_RTTI_Serialize();

        IO::Link<Action> m_action;
        festd::vector<KeyboardMapping> m_keyboard;
        festd::vector<MouseMapping> m_mouse;
        MappingFlags m_flags = MappingFlags::kNone;
        MappingOperation m_operation = MappingOperation::kSum;
        //! Optional mouse button gate, e.g. 1 for RMB. -1 means no gate.
        int32_t m_mouseButtonGate = -1;
    };


    struct MappingTable final
    {
        FE_RTTI_Reflect("C27E593F-6199-41FB-AFFD-8E73F51ECCFE");
        FE_RTTI_Serialize();

        festd::vector<ActionMapping> m_mappings;
    };
} // namespace FE::Input
