#pragma once
#include <Core/Serialization/Serialization.h>

namespace FE::Serialization
{
    //! @brief Visit authored asset references without building a payload; reports serializer errors.
    ResultCode VisitAssetReferences(const Rtti::Type& type, const void* value, void* userData,
                                    SerializationContext::AssetReferenceCallback callback, Format format = Format::kPackedBinary);
} // namespace FE::Serialization
