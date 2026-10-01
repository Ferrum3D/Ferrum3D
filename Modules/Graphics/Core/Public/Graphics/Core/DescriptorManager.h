#pragma once
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Fence.h>
#include <Graphics/Core/Sampler.h>
#include <Graphics/Core/Texture.h>
#include <festd/bit_vector.h>
#include <festd/unordered_map.h>
#include <festd/vector.h>

namespace FE::Graphics::Core
{
    struct ResourceDescriptorInfo final
    {
        Rc<Resource> m_resource;
        DescriptorType m_descriptorType = DescriptorType::kInvalid;
        TextureSubresource m_textureSubresource = TextureSubresource::kInvalid;
        BufferSlice m_bufferSlice = BufferSlice::kInvalid;

        ResourceDescriptorInfo() = default;
        ResourceDescriptorInfo(TextureView texture, DescriptorType type);
        ResourceDescriptorInfo(BufferView buffer, DescriptorType type);
    };


    struct DescriptorManager : public Memory::RefCountedObjectBase
    {
        FE_RTTI("7238722E-6241-4EB2-B140-C0545346DD57");

        static constexpr uint32_t kPersistentDescriptorCount = 32 * 1024;
        static constexpr uint32_t kTransientDescriptorCount = 32 * 1024;
        static constexpr uint32_t kResourceDescriptorCount = kPersistentDescriptorCount + kTransientDescriptorCount;
        static constexpr uint32_t kSamplerDescriptorCount = 512;

        //! Persistent descriptors are exclusively owned. Mutations happen between frames.
        //! Updates preserve the index; replaced resources and freed indices retire after submitted uses complete.
        //! Do not explicitly replace or decommit physical storage while any descriptor version can be in use.
        [[nodiscard]] TextureSRVDescriptor CreateSRV(TextureView texture);
        [[nodiscard]] TextureUAVDescriptor CreateUAV(TextureView texture);
        [[nodiscard]] BufferSRVDescriptor CreateSRV(BufferView buffer);
        [[nodiscard]] BufferUAVDescriptor CreateUAV(BufferView buffer);
        void Update(TextureSRVDescriptor descriptor, TextureView texture);
        void Update(TextureUAVDescriptor descriptor, TextureView texture);
        void Update(BufferSRVDescriptor descriptor, BufferView buffer);
        void Update(BufferUAVDescriptor descriptor, BufferView buffer);
        void FreePersistentDescriptor(uint32_t descriptorIndex);
        [[nodiscard]] const ResourceDescriptorInfo& GetPersistentResourceInfo(uint32_t descriptorIndex) const;

        //! Samplers are interned for the lifetime of the manager.
        [[nodiscard]] SamplerDescriptor GetSampler(SamplerState sampler);

        //! All frames use one ordered queue timeline, including work recorded but not submitted yet.
        void BeginFrame(const FenceSyncPoint& completion);
        //! Called after graph resources have physical storage, before recording their uses.
        void PrepareDescriptors(festd::span<const ResourceDescriptorInfo> transientDescriptors);
        void EndFrame();

    protected:
        DescriptorManager();

        virtual void BeginFrameInternal(const FenceSyncPoint& completion) = 0;
        virtual void PrepareDescriptorsInternal(festd::span<const ResourceDescriptorInfo> transientDescriptors) = 0;
        virtual void InvalidatePersistentDescriptor(uint32_t descriptorIndex) = 0;

        festd::vector<ResourceDescriptorInfo> m_persistentDescriptors;
        festd::vector<SamplerState> m_samplers;

    private:
        struct RetiredDescriptor final
        {
            Rc<Resource> m_resource;
            FenceSyncPoint m_completion;
            uint32_t m_indexToRelease = kInvalidIndex;
        };

        uint32_t AllocatePersistentDescriptor(ResourceDescriptorInfo info);
        void UpdatePersistentDescriptor(uint32_t descriptorIndex, ResourceDescriptorInfo info);
        void RetireDescriptor(uint32_t descriptorIndex, bool releaseIndex);
        void CollectRetiredDescriptors();

        festd::bit_vector m_freePersistentDescriptors;
        festd::vector<RetiredDescriptor> m_retiredDescriptors;
        festd::unordered_dense_map<SamplerState, uint32_t> m_samplerMap;
        FenceSyncPoint m_lastFrameCompletion;
        bool m_frameActive = false;
        bool m_descriptorsPrepared = false;
    };
} // namespace FE::Graphics::Core
