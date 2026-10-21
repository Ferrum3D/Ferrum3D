#pragma once
#include <Core/Serialization/Serialization.h>
#include <Framework/Entities/EntityAssetRecords.h>
#include <festd/span.h>

namespace FE::Framework
{
    //! @brief Copy serialized fields between initialized values through a reusable scratch buffer.
    struct ComponentCopier final
    {
        //! @brief Copy fields into destination, remapping internal entity references and reporting asset references.
        bool Copy(const Rtti::Type& type, const void* source, void* destination,
                  festd::span<const EntityUuidBinding> bindings = {}, void* referenceUserData = nullptr,
                  Serialization::SerializationContext::AssetReferenceCallback referenceCallback = nullptr);

    private:
        festd::pmr::vector<std::byte> m_buffer;
    };


    //! @brief One-off ComponentCopier::Copy; prefer a shared copier when copying many components.
    bool CopyComponent(const Rtti::Type& type, const void* source, void* destination,
                       festd::span<const EntityUuidBinding> bindings = {}, void* referenceUserData = nullptr,
                       Serialization::SerializationContext::AssetReferenceCallback referenceCallback = nullptr);
} // namespace FE::Framework
