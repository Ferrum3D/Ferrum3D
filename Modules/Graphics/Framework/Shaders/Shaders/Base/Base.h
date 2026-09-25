#pragma once

#if defined(__cplusplus)
#    define FE_HOST 1
#else
#    define FE_DEVICE 1
#endif

#if FE_HOST
#    include <Core/Base/Base.h>
#    include <Core/Math/Matrix4x4.h>
#    include <Graphics/Core/FrameGraph/FrameGraph.h>
#    include <Graphics/Core/FrameGraph/FrameGraphPass.h>

#    define FE_HOST_BEGIN_NAMESPACE(name)                                                                                        \
        namespace name                                                                                                           \
        {
#    define FE_HOST_END_NAMESPACE }

#    define FE_CONST const
#    define FE_CONSTEXPR inline constexpr

#    define FE_INIT(...) = __VA_ARGS__

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


    struct BufferPointer final
    {
        uint2 m_deviceAddress;

        BufferPointer() = default;

        explicit BufferPointer(const uint64_t deviceAddress)
            : m_deviceAddress(static_cast<uint32_t>(deviceAddress), static_cast<uint32_t>(deviceAddress >> 32))
        {
        }

        [[nodiscard]] uint64_t GetDeviceAddress() const
        {
            return static_cast<uint64_t>(m_deviceAddress.x) | (static_cast<uint64_t>(m_deviceAddress.y) << 32);
        }
    };

    static_assert(sizeof(BufferPointer) == sizeof(uint64_t));
    static_assert(alignof(BufferPointer) == alignof(uint32_t));
} // namespace FE

#endif

#if FE_DEVICE
#    include <Shaders/Base/Base.hlsli>
#endif
