#pragma once
#include <Core/RTTI/Reflection.h>

namespace FE::Editor
{
    bool DrawReflectedFields(const Rtti::Type& type, void* value, bool readOnly = false, const void* runtimeValue = nullptr);
    festd::ascii_view GetDisplayName(const Rtti::Type& type);
    festd::ascii_view GetIcon(const Rtti::Type* type);
} // namespace FE::Editor
