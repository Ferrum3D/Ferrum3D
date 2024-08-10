#pragma once
#include <FeCore/Containers/ByteBuffer.h>

namespace FE::Osmium
{
    struct DescriptorHeapDescBinding
    {
        ByteBuffer Sizes;
        uint32_t MaxSets;
    };
}
