#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Visibility)

    FE_CONSTEXPR uint32_t kContractVersion = 3;
    FE_CONSTEXPR uint32_t kPrimitiveBits = 6;
    FE_CONSTEXPR uint32_t kMeshletBits = 22;
    FE_CONSTEXPR uint32_t kLodBits = 4;
    FE_CONSTEXPR uint32_t kLodShift = kPrimitiveBits + kMeshletBits;
    FE_CONSTEXPR uint32_t kMaxMeshletCount = 1u << kMeshletBits;
    FE_CONSTEXPR uint32_t kMaxPrimitiveCount = 1u << kPrimitiveBits;
    FE_CONSTEXPR uint32_t kMaxLodCount = 1u << kLodBits;
    FE_CONSTEXPR uint32_t kMaxInstanceRow = 0x0ffffffeu;
    FE_CONSTEXPR uint32_t kObjectTypeShift = 28;
    FE_CONSTEXPR uint32_t kObjectIndexMask = 0x0fffffffu;

    struct VisibilityID final
    {
        uint32_t m_instance;
        uint32_t m_primitive;
    };


    // The primitive word stores the LOD, meshlet and meshlet-local triangle from the most significant bits down.
    FE_INLINE uint32_t EncodePrimitive(const uint32_t lod, const uint32_t meshlet, const uint32_t primitive)
    {
        return (lod << kLodShift) | (meshlet << kPrimitiveBits) | primitive;
    }


    FE_INLINE uint32_t DecodeLod(const uint32_t encodedPrimitive)
    {
        return encodedPrimitive >> kLodShift;
    }


    FE_INLINE uint32_t DecodeMeshlet(const uint32_t encodedPrimitive)
    {
        return (encodedPrimitive >> kPrimitiveBits) & (kMaxMeshletCount - 1);
    }


    FE_INLINE uint32_t DecodePrimitive(const uint32_t encodedPrimitive)
    {
        return encodedPrimitive & (kMaxPrimitiveCount - 1);
    }


#if FE_HOST
    static_assert(Core::Limits::Mesh::kMaxMeshletPrimitiveCount == kMaxPrimitiveCount);
    static_assert(kLodShift + kLodBits == 32);
    static_assert(sizeof(VisibilityID) == 8);

    inline VisibilityID Encode(const uint32_t instanceRow,
                               const uint32_t meshlet,
                               const uint32_t primitive,
                               const uint32_t objectType = 0,
                               const uint32_t lod = 0)
    {
        FE_Assert(objectType < 16);
        FE_Assert(instanceRow <= kMaxInstanceRow);
        FE_Assert(meshlet < kMaxMeshletCount);
        FE_Assert(primitive < kMaxPrimitiveCount);
        FE_Assert(lod < kMaxLodCount);
        return { (objectType << kObjectTypeShift) | (instanceRow + 1), EncodePrimitive(lod, meshlet, primitive) };
    }
#endif

FE_HOST_END_NAMESPACE
