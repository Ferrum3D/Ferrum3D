#pragma once
#include <Core/IO/Assets.h>
#include <Core/RTTI/Traversal.h>

namespace FE::IO
{
    struct AssetReferenceInfo final
    {
        AssetID m_assetId = AssetID::kNull;
        Rtti::TypeID m_assetType = Rtti::TypeID::kNull;
        DependencyKind m_kind = DependencyKind::kHard;
    };


    using AssetReferenceCallback = void (*)(void* userData, const AssetReferenceInfo& reference);


    //! @brief Report every valid asset reference in a value, including those in containers and dynamic values.
    //!
    //! References are values whose type has an AssetReference attribute. Fields that are not serialized are skipped.
    void VisitAssetReferences(const Rtti::Type& type, const void* value, void* userData, AssetReferenceCallback callback);


    template<class TCallback>
    void VisitAssetReferences(const Rtti::Type& type, const void* value, TCallback&& callback)
    {
        using Function = std::remove_reference_t<TCallback>;
        VisitAssetReferences(type, value, &callback, [](void* userData, const AssetReferenceInfo& reference) {
            (*static_cast<Function*>(userData))(reference);
        });
    }
} // namespace FE::IO
