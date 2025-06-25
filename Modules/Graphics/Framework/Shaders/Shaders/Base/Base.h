#pragma once

#if defined(__cplusplus)
#    define FE_HOST 1
#else
#    define FE_DEVICE 1
#endif

#if FE_HOST
#    include <FeCore/Base/Base.h>
#    include <FeCore/Math/Matrix4x4.h>
#    include <Graphics/Core/FrameGraph/FrameGraph.h>

#    define FE_HOST_BEGIN_NAMESPACE(name)                                                                                        \
        namespace name                                                                                                           \
        {
#    define FE_HOST_END_NAMESPACE }

#    define FE_CONST const

namespace FE
{
    using float2 = Vector2;
    using float3 = PackedVector3F;
    using float4 = PackedVector4F;

    using fehalf = float;
    using fehalf2 = float2;
    using fehalf3 = float3;
    using fehalf4 = float4;

    using int2 = Vector2Int;
    using int3 = PackedVector3Int;

    using uint2 = Vector2UInt;
    using uint3 = PackedVector3UInt;

    using float4x4 = Matrix4x4;
} // namespace FE

#endif

#if FE_DEVICE
#    include <Shaders/Base/Base.hlsli>
#endif
