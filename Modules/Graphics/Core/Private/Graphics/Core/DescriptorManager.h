#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/FrameGraph/Base.h>
#include <Graphics/Core/Sampler.h>
#include <Graphics/Core/Texture.h>
#include <festd/bit_vector.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Core
{
    struct Resource;
    struct Texture;
    struct Buffer;

    struct DescriptorManager : public Memory::RefCountedObjectBase
    {
        [[nodiscard]] uint32_t ReserveDescriptor(Texture* texture, TextureSubresource subresource);
        [[nodiscard]] uint32_t ReserveDescriptor(Buffer* buffer, BufferSubresource subresource);
        [[nodiscard]] uint32_t ReserveDescriptor(SamplerState samplerState);

        void CommitResourceDescriptor(uint32_t descriptorIndex, DescriptorType type);
        void CommitSamplerDescriptor(uint32_t descriptorIndex);

        virtual void BeginFrame() = 0;
        virtual FenceSyncPoint CloseFrame() = 0;

    protected:
        static constexpr uint32_t kMaxDescriptorSets = 8;
        static constexpr uint32_t kSamplerDescriptorCount = 512;
        static constexpr uint32_t kResourceDescriptorCount = 64 * 1024;

        DescriptorManager();

        void Clear();

        struct TextureKey final
        {
            uint32_t m_resourceID = kInvalidIndex;
            TextureSubresource m_subresource = TextureSubresource::kInvalid;

            FE_DECLARE_POD_HASH(TextureKey);
        };

        struct BufferKey final
        {
            uint32_t m_resourceID = kInvalidIndex;
            BufferSubresource m_subresource = BufferSubresource::kInvalid;

            FE_DECLARE_POD_HASH(BufferKey);
        };

        struct ResourceDescriptorInfo final
        {
            Resource* m_resource = nullptr;
            DescriptorType m_descriptorType = DescriptorType::kInvalid;

            union
            {
                TextureSubresource m_textureSubresource;
                BufferSubresource m_bufferSubresource;
            };

            ResourceDescriptorInfo(Texture* texture, TextureSubresource subresource);
            ResourceDescriptorInfo(Buffer* buffer, BufferSubresource subresource);
        };

        festd::segmented_unordered_dense_map<TextureKey, uint32_t, TextureKey::Hash, TextureKey::Eq> m_textureDescriptorMap;
        festd::segmented_unordered_dense_map<BufferKey, uint32_t, BufferKey::Hash, BufferKey::Eq> m_bufferDescriptorMap;
        festd::segmented_unordered_dense_map<SamplerState, uint32_t> m_samplerDescriptorMap;

        SegmentedVector<ResourceDescriptorInfo> m_resourceDescriptors;
        SegmentedVector<SamplerState> m_samplerDescriptors;

        festd::bit_vector m_committedResourceDescriptors;
        festd::bit_vector m_committedSamplerDescriptors;
    };
} // namespace FE::Graphics::Core
