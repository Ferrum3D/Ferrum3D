#pragma once
#include <Core/RTTI/Reflection.h>
#include <cstring>

namespace FE::Editor
{
    //! A field changed by an inspector control, applied to a copy of the drawn value.
    struct FieldEdit final
    {
        //! Controls exist only for trivially copyable types up to this size.
        static constexpr uint32_t kMaxSize = 64;

        uint32_t m_offset = 0;
        uint32_t m_size = 0;
        alignas(16) std::byte m_value[kMaxSize];

        void Apply(void* destination) const
        {
            memcpy(static_cast<std::byte*>(destination) + m_offset, m_value, m_size);
        }
    };


    //! @brief Draw reflected fields without writing to the value; returns true when a control changed a field.
    //!
    //! Transient fields are read from runtimeValue when provided and are always read-only.
    bool DrawReflectedFields(const Rtti::Type& type, const void* value, FieldEdit& edit, bool readOnly = false,
                             const void* runtimeValue = nullptr);

    festd::ascii_view GetDisplayName(const Rtti::Type& type);
    festd::ascii_view GetIcon(const Rtti::Type* type);
} // namespace FE::Editor
