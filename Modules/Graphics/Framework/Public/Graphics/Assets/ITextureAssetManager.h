#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE::Graphics
{
    struct ITextureAssetManager : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(ITextureAssetManager, "DE919340-61E9-467E-96CB-CAF037A20A3A");
    };
} // namespace FE::Graphics
