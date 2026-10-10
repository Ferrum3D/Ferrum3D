#pragma once
#include <Framework/Entities/EntityAssetRecords.h>
#include <festd/span.h>

namespace FE::Framework
{
    // Copy serialized fields into an initialized value, remapping internal entity references when requested.
    bool CopyComponent(const Rtti::Type& type, const void* source, void* destination,
                       festd::span<const EntityUuidBinding> bindings = {});
} // namespace FE::Framework
