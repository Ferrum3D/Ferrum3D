#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Visibility)

    FE_CONSTEXPR uint32_t kContractVersion = 2;
    FE_CONSTEXPR uint32_t kPrimitiveBits = 6;
    FE_CONSTEXPR uint32_t kMaxMeshletCount = 1u << 26;
    FE_CONSTEXPR uint32_t kMaxPrimitiveCount = 1u << kPrimitiveBits;
    FE_CONSTEXPR uint32_t kMaxInstanceRow = 0x0ffffffeu;
    FE_CONSTEXPR uint32_t kObjectTypeShift = 28;
    FE_CONSTEXPR uint32_t kObjectIndexMask = 0x0fffffffu;

    struct VisibilityID final
    {
        uint32_t m_instance;
        uint32_t m_primitive;
    };


#if FE_HOST
    static_assert(Core::Limits::Mesh::kMaxMeshletPrimitiveCount == kMaxPrimitiveCount);
    static_assert(sizeof(VisibilityID) == 8);

    inline VisibilityID Encode(const uint32_t instanceRow,
                               const uint32_t meshlet,
                               const uint32_t primitive,
                               const uint32_t objectType = 0)
    {
        FE_Assert(objectType < 16);
        FE_Assert(instanceRow <= kMaxInstanceRow);
        FE_Assert(meshlet < kMaxMeshletCount);
        FE_Assert(primitive < kMaxPrimitiveCount);
        return { (objectType << kObjectTypeShift) | (instanceRow + 1), (meshlet << kPrimitiveBits) | primitive };
    }
#endif

FE_HOST_END_NAMESPACE
