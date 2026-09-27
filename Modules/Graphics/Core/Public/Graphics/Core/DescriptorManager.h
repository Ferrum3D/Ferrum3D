#pragma once
#include <Core/Containers/SegmentedVector.h>
#include <Graphics/Core/Base.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Sampler.h>
#include <Graphics/Core/Texture.h>
#include <festd/bit_vector.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Core
{
    struct Resource;
    struct Texture;
    struct Buffer;


    enum class DescriptorLifetime : uint32_t
    {
        //! Retired automatically by CloseFrame() and recycled after the frame fence.
        kTransient,

        //! Keeps a stable index until explicitly freed.
        kPersistent,
    };


    struct ResourceDescriptorInfo final
    {
        Resource* m_resource = nullptr;
        DescriptorType m_descriptorType = DescriptorType::kInvalid;

        union
        {
            TextureSubresource m_textureSubresource;
            BufferSlice m_bufferSlice;
        };

        ResourceDescriptorInfo();
        explicit ResourceDescriptorInfo(TextureView texture);
        explicit ResourceDescriptorInfo(BufferView buffer);
    };


    struct DescriptorManager : public Memory::RefCountedObjectBase
    {
        FE_RTTI("7238722E-6241-4EB2-B140-C0545346DD57");

        [[nodiscard]] uint32_t ReserveDescriptor(TextureView texture,
                                                 DescriptorLifetime lifetime = DescriptorLifetime::kTransient);
        [[nodiscard]] uint32_t ReserveDescriptor(BufferView buffer, DescriptorLifetime lifetime = DescriptorLifetime::kTransient);
        [[nodiscard]] uint32_t ReserveDescriptor(SamplerState samplerState,
                                                 DescriptorLifetime lifetime = DescriptorLifetime::kTransient);

        //! Rebinds an existing persistent descriptor without changing its shader-visible index.
        void UpdateDescriptor(uint32_t descriptorIndex, TextureView texture);
        void UpdateDescriptor(uint32_t descriptorIndex, BufferView buffer);
        void FreeResourceDescriptor(uint32_t descriptorIndex);
        void FreeSamplerDescriptor(uint32_t descriptorIndex);

        void CommitResourceDescriptor(uint32_t descriptorIndex, DescriptorType type);
        void CommitSamplerDescriptor(uint32_t descriptorIndex);

        ResourceDescriptorInfo GetResourceInfo(uint32_t descriptorIndex);
        virtual uint64_t GetDeviceAddress(uint32_t descriptorIndex) = 0;

        virtual void BeginFrame() = 0;
        virtual FenceSyncPoint CloseFrame() = 0;

    protected:
        static constexpr uint32_t kMaxDescriptorSets = 8;
        static constexpr uint32_t kSamplerDescriptorCount = 512;
        static constexpr uint32_t kResourceDescriptorCount = 64 * 1024;

        DescriptorManager();

        void ClearTransientDescriptors();
        void ReleaseResourceDescriptorIndex(uint32_t descriptorIndex);
        void ReleaseSamplerDescriptorIndex(uint32_t descriptorIndex);
        virtual void RetireResourceDescriptor(uint32_t descriptorIndex, Resource* resource, bool releaseIndex) = 0;
        virtual void RetireTransientResourceDescriptor(uint32_t descriptorIndex, Resource* resource) = 0;
        virtual void RetireTransientSamplerDescriptor(uint32_t descriptorIndex) = 0;
        virtual void RetireSamplerDescriptor(uint32_t descriptorIndex) = 0;

        struct TextureKey final
        {
            uint32_t m_resourceID = kInvalidIndex;
            TextureSubresource m_subresource = TextureSubresource::kInvalid;
            DescriptorLifetime m_lifetime = DescriptorLifetime::kTransient;

            FE_DECLARE_POD_HASH(TextureKey);
        };

        struct BufferKey final
        {
            uint32_t m_resourceID = kInvalidIndex;
            BufferSlice m_subresource = BufferSlice::kInvalid;
            DescriptorLifetime m_lifetime = DescriptorLifetime::kTransient;

            FE_DECLARE_POD_HASH(BufferKey);
        };

        struct SamplerKey final
        {
            SamplerState m_state = SamplerState::kPointWrap;
            DescriptorLifetime m_lifetime = DescriptorLifetime::kTransient;

            FE_DECLARE_POD_HASH(SamplerKey);
        };

        festd::segmented_unordered_dense_map<TextureKey, uint32_t, TextureKey::Hash, TextureKey::Eq> m_textureDescriptorMap;
        festd::segmented_unordered_dense_map<BufferKey, uint32_t, BufferKey::Hash, BufferKey::Eq> m_bufferDescriptorMap;
        festd::segmented_unordered_dense_map<SamplerKey, uint32_t, SamplerKey::Hash, SamplerKey::Eq> m_samplerDescriptorMap;

        SegmentedVector<ResourceDescriptorInfo> m_resourceDescriptors;
        SegmentedVector<SamplerState> m_samplerDescriptors;

        festd::bit_vector m_freeResourceDescriptors;
        festd::bit_vector m_transientResourceDescriptors;
        festd::bit_vector m_persistentResourceDescriptors;
        festd::bit_vector m_initializedResourceDescriptors;
        festd::bit_vector m_resourceDescriptorsToUpdate;

        festd::bit_vector m_freeSamplerDescriptors;
        festd::bit_vector m_transientSamplerDescriptors;
        festd::bit_vector m_persistentSamplerDescriptors;
        festd::bit_vector m_initializedSamplerDescriptors;
        festd::bit_vector m_samplerDescriptorsToUpdate;
    };
} // namespace FE::Graphics::Core
