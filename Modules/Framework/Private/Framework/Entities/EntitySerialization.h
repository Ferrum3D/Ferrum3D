#pragma once
#include <Framework/Entities/EntityAssetRecords.h>
#include <festd/span.h>

namespace FE::Framework
{
    //! @brief Point entity references to the entities bound to their UUIDs; references to other entities are kept.
    //!
    //! References are found anywhere in the value, including containers and dynamic values; runtime-only fields are skipped.
    void RemapEntityReferences(const Rtti::Type& type, void* value, festd::span<const EntityUuidBinding> bindings);


    //! @brief Copy serialized fields between initialized component values, then remap internal entity references.
    //!
    //! The destination keeps its runtime-only fields. Pass the same value as source and destination to only remap.
    bool CopyComponent(const Rtti::Type& type, const void* source, void* destination,
                       festd::span<const EntityUuidBinding> bindings = {});
} // namespace FE::Framework
