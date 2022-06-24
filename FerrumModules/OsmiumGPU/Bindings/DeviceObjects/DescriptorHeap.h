#pragma once
#include <FeCore/Containers/IByteBuffer.h>

namespace FE::Osmium
{
    struct DescriptorHeapDescBinding
    {
        IByteBuffer* Sizes;
        UInt32 MaxSets;
    };
}
