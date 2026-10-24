#include <Core/IO/AssetReferences.h>

namespace FE::IO
{
    void VisitAssetReferences(const Rtti::Type& type, const void* value, void* userData, const AssetReferenceCallback callback)
    {
        FE_PROFILER_ZONE();
        FE_Assert(value && callback);

        Rtti::VisitConst(type, value, Rtti::VisitFlags::kSkipTransient, [&](const Rtti::VisitNode& node) {
            // Large element arrays, e.g. mesh data, have no references when their element type cannot hold one.
            const Rtti::Type* elementType = Rtti::GetElementType(node.m_type);
            if (elementType && !Rtti::CanContainShape(*elementType, Rtti::TypeShape::kHandle))
                return Rtti::VisitResult::kSkipChildren;

            const auto* handle = node.m_type.TryGetShape<Rtti::HandleShape>();
            const auto* reference = node.m_type.TryGetAttribute<AssetReference>();
            if (handle == nullptr || reference == nullptr)
                return Rtti::VisitResult::kContinue;

            const AssetID assetId = handle->m_get(node.m_value);
            if (assetId.IsValid())
                callback(userData, AssetReferenceInfo{ assetId, reference->m_getAssetType(), reference->m_kind });

            return Rtti::VisitResult::kSkipChildren;
        });
    }
} // namespace FE::IO
