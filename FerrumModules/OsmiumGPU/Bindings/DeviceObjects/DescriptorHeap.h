#pragma once
#include <FeCore/Containers/IByteBuffer.h>

namespace FE::GPU
{
    struct DescriptorHeapDescBinding
    {
        IByteBuffer* Sizes;
        UInt32 MaxSets;
    };
}
