#pragma once
#include <Core/RTTI/Traversal.h>
#include <GameFramework/Editor/AssetBrowser.h>
#include <festd/string.h>

namespace FE::Editor
{
    //! A change made by an inspector control, applied to a copy of the drawn value.
    struct InspectorEdit final
    {
        enum class Kind : uint8_t
        {
            kNone,
            //! Replace a trivially copyable value with m_value.
            kAssign,
            kSetString,
            kSetHandle,
            //! Insert a default element at m_index of the sequence at m_path.
            kInsertElement,
            kEraseElement,
            //! Switch the variant at m_path to alternative m_index.
            kSetAlternative,
            //! Replace the dynamic value at m_path with a default instance of m_dynamicType, or clear it.
            kSetDynamicType,
        };

        //! Controls exist only for trivially copyable types up to this size.
        static constexpr uint32_t kMaxSize = 64;

        Kind m_kind = Kind::kNone;
        //! Location of the edited value relative to the drawn value.
        Rtti::ValuePath m_path;
        uint32_t m_index = 0;
        uint32_t m_size = 0;
        Uuid m_handle = Uuid::kNull;
        const Rtti::Type* m_dynamicType = nullptr;
        festd::fixed_string m_text;
        alignas(16) std::byte m_value[kMaxSize];

        //! @brief Apply to a value of the drawn type; fails when it no longer has the edited structure.
        bool Apply(const Rtti::Type& type, void* value) const;
    };


    struct InspectorContext final
    {
        //! Candidates for asset references, filtered by the referenced asset type.
        festd::span<const AssetEntry> m_assets;
        //! Open every nested struct, container and base.
        bool m_expandAll = false;
    };


    //! @brief Draw a reflected value without writing to it; returns true when a control requested an edit.
    //!
    //! Structs show their bases and fields, containers their elements, and variants and dynamic values their active type.
    //! Transient fields of the drawn struct are read from runtimeValue when provided and are always read-only.
    bool DrawReflectedValue(const Rtti::Type& type, const void* value, InspectorEdit& edit, const InspectorContext& context,
                            bool readOnly = false, const void* runtimeValue = nullptr);

    festd::ascii_view GetDisplayName(const Rtti::Type& type);
    festd::ascii_view GetIcon(const Rtti::Type* type);
} // namespace FE::Editor
