#include <FeCore/RTTI/Reflection.h>
#include <Graphics/Core/Common/Buffer.h>
#include <Graphics/Core/Common/FrameGraph/FrameGraph.h>
#include <Graphics/Core/Common/Texture.h>
#include <Graphics/Core/FrameGraph/FrameGraphPass.h>

namespace FE::Graphics::Common
{
    namespace
    {
        bool HasShaderStage(const Core::GraphicsPipelineDesc& pipelineDesc, const Core::ShaderStage stage)
        {
            return pipelineDesc.m_shaders[festd::to_underlying(stage)].IsValid();
        }
    } // namespace


    FrameGraph::PassNode::PassNode(std::pmr::memory_resource* allocator)
        : m_textureOwnershipTransferBarriers(allocator)
        , m_bufferOwnershipTransferBarriers(allocator)
        , m_barrierBatcher(allocator)
        , m_accessedTextures(allocator)
        , m_accessedBuffers(allocator)
    {
    }


    FrameGraph::ResourceNode::ResourceNode(std::pmr::memory_resource* allocator)
        : m_accesses(allocator)
    {
    }


    void FrameGraph::CompileAndExecute()
    {
        Compile();
        Execute();
    }


    Core::FrameGraphTextureDescriptorHandle FrameGraph::GetDescriptor(const Core::TextureView texture)
    {
        return Core::FrameGraphTextureDescriptorHandle{ m_descriptorManager->ReserveDescriptor(texture) };
    }


    Core::FrameGraphBufferDescriptorHandle FrameGraph::GetDescriptor(const Core::BufferView buffer)
    {
        return Core::FrameGraphBufferDescriptorHandle{ m_descriptorManager->ReserveDescriptor(buffer) };
    }


    SamplerDescriptor FrameGraph::GetSampler(const Core::SamplerState sampler)
    {
        return SamplerDescriptor{ m_descriptorManager->ReserveDescriptor(sampler) };
    }


    FrameGraph::FrameGraph(Core::Device* device, Core::DescriptorManager* descriptorManager, Core::ResourcePool* resourcePool)
        : m_descriptorManager(descriptorManager)
        , m_resourcePool(resourcePool)
        , m_passes(&m_linearAllocator)
        , m_resources(&m_linearAllocator)
    {
        m_device = device;
    }


    void FrameGraph::AddPassInternal(const PassNodeDesc& desc)
    {
        PassNode& passNode = m_passes.emplace_back(&m_linearAllocator);
        passNode.m_passIndex = m_passes.size() - 1;
        passNode.m_name = desc.m_name;
        passNode.m_functor = desc.m_functor;
        passNode.m_execute = desc.m_execute;
        passNode.m_destroy = desc.m_destroy;
        passNode.m_userPassDescTypeID = desc.m_userPassDescTypeID;
        passNode.m_userPassDescPtr = desc.m_userPassDescPtr;
    }


    void FrameGraph::CommitPassNode(const uint32_t passIndex)
    {
        PreparePassCompileInfo(m_passes[passIndex]);
    }


    Core::ResourcePool* FrameGraph::GetResourcePool()
    {
        return m_resourcePool;
    }


    void FrameGraph::BeginFrame()
    {
        m_descriptorManager->BeginFrame();
    }


    void FrameGraph::ParsePassPushConstants(PassNode& pass, const RTTI::Type& type)
    {
        // Push constants can contain resource descriptor indices.
        // We have to account for the corresponding resources when compiling pass barriers.

        Core::BarrierSyncFlags shaderSyncFlags;
        if (Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kComputePipeline))
        {
            shaderSyncFlags = Core::BarrierSyncFlags::kComputeShading;
        }
        else
        {
            const auto* graphicsPipeline = RTTI::AssertCast<const Core::GraphicsPipeline*>(pass.m_pipeline);
            const Core::GraphicsPipelineDesc& pipelineDesc = graphicsPipeline->GetDesc();

            const bool hasMeshShader = HasShaderStage(pipelineDesc, Core::ShaderStage::kMesh);

            shaderSyncFlags = Core::BarrierSyncFlags::kPixelShading;
            if (hasMeshShader)
            {
                shaderSyncFlags |= Core::BarrierSyncFlags::kMeshShading;

                if (HasShaderStage(pipelineDesc, Core::ShaderStage::kAmplification))
                    shaderSyncFlags |= Core::BarrierSyncFlags::kAmplificationShading;
            }
            else
            {
                FE_Assert(HasShaderStage(pipelineDesc, Core::ShaderStage::kVertex));
                shaderSyncFlags |= Core::BarrierSyncFlags::kVertexShading;
            }
        }

        for (const RTTI::FieldInfo& field : type.m_fields)
        {
            for (uint32_t arrayIndex = 0; arrayIndex < field.m_arraySize; ++arrayIndex)
            {
                if (field.m_type == RTTI::GetTypeID<TextureSRVDescriptor>())
                {
                    const TextureSRVDescriptor descriptor = field.Get<TextureSRVDescriptor>(pass.m_userPassDescPtr, arrayIndex);
                    const Core::ResourceDescriptorInfo resourceInfo = m_descriptorManager->GetResourceInfo(descriptor.m_value);
                    FE_Assert(resourceInfo.m_resource->GetType() == Core::ResourceType::kTexture);
                    m_descriptorManager->CommitResourceDescriptor(descriptor.m_value, Core::DescriptorType::kSRV);

                    TextureAccess access;
                    access.m_syncFlags = shaderSyncFlags;
                    access.m_accessFlags = Core::BarrierAccessFlags::kShaderRead;
                    access.m_layout = Core::BarrierLayout::kShaderRead;
                    access.m_subresource = resourceInfo.m_textureSubresource;
                    RegisterResource(resourceInfo.m_resource, pass, access);
                }
                else if (field.m_type == RTTI::GetTypeID<TextureUAVDescriptor>())
                {
                    const TextureUAVDescriptor descriptor = field.Get<TextureUAVDescriptor>(pass.m_userPassDescPtr, arrayIndex);
                    const Core::ResourceDescriptorInfo resourceInfo = m_descriptorManager->GetResourceInfo(descriptor.m_value);
                    FE_Assert(resourceInfo.m_resource->GetType() == Core::ResourceType::kTexture);
                    m_descriptorManager->CommitResourceDescriptor(descriptor.m_value, Core::DescriptorType::kUAV);

                    TextureAccess access;
                    access.m_syncFlags = shaderSyncFlags;
                    access.m_accessFlags = Core::BarrierAccessFlags::kShaderWrite;
                    access.m_layout = Core::BarrierLayout::kShaderReadWrite;
                    access.m_subresource = resourceInfo.m_textureSubresource;
                    RegisterResource(resourceInfo.m_resource, pass, access);
                }
                else if (field.m_type == RTTI::GetTypeID<BufferSRVDescriptor>())
                {
                    const BufferSRVDescriptor descriptor = field.Get<BufferSRVDescriptor>(pass.m_userPassDescPtr, arrayIndex);
                    const Core::ResourceDescriptorInfo resourceInfo = m_descriptorManager->GetResourceInfo(descriptor.m_value);
                    FE_Assert(resourceInfo.m_resource->GetType() == Core::ResourceType::kBuffer);
                    m_descriptorManager->CommitResourceDescriptor(descriptor.m_value, Core::DescriptorType::kSRV);

                    BufferAccess access;
                    access.m_syncFlags = shaderSyncFlags;
                    access.m_accessFlags = Core::BarrierAccessFlags::kShaderRead;
                    RegisterResource(resourceInfo.m_resource, pass, access);
                }
                else if (field.m_type == RTTI::GetTypeID<BufferUAVDescriptor>())
                {
                    const BufferUAVDescriptor descriptor = field.Get<BufferUAVDescriptor>(pass.m_userPassDescPtr, arrayIndex);
                    const Core::ResourceDescriptorInfo resourceInfo = m_descriptorManager->GetResourceInfo(descriptor.m_value);
                    FE_Assert(resourceInfo.m_resource->GetType() == Core::ResourceType::kBuffer);
                    m_descriptorManager->CommitResourceDescriptor(descriptor.m_value, Core::DescriptorType::kUAV);

                    BufferAccess access;
                    access.m_syncFlags = shaderSyncFlags;
                    access.m_accessFlags = Core::BarrierAccessFlags::kShaderWrite;
                    RegisterResource(resourceInfo.m_resource, pass, access);
                }
                else if (field.m_type == RTTI::GetTypeID<SamplerDescriptor>())
                {
                    const SamplerDescriptor descriptor = field.Get<SamplerDescriptor>(pass.m_userPassDescPtr, arrayIndex);
                    m_descriptorManager->CommitSamplerDescriptor(descriptor.m_value);
                }
            }
        }
    }


    void FrameGraph::PreparePassCompileInfo(PassNode& pass)
    {
        const RTTI::Type* descType = RTTI::TypeRegistry::FindType(pass.m_userPassDescTypeID);
        FE_AssertDebug(descType);

        for (const RTTI::FieldInfo& field : descType->m_fields)
        {
            if (field.m_type == RTTI::GetTypeID<Core::PassGraphicsPipeline>())
            {
                FE_Assert(!Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kGraphicsPipeline));
                const Core::PassGraphicsPipeline pipeline = field.Get<Core::PassGraphicsPipeline>(pass.m_userPassDescPtr);
                pass.m_pipeline = pipeline.m_pipeline;
                pass.m_specifiedStatesMask |= PassStateFlags::kGraphicsPipeline;
            }
            else if (field.m_type == RTTI::GetTypeID<Core::PassComputePipeline>())
            {
                FE_Assert(!Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kComputePipeline));
                const Core::PassComputePipeline pipeline = field.Get<Core::PassComputePipeline>(pass.m_userPassDescPtr);
                pass.m_pipeline = pipeline.m_pipeline;
                pass.m_specifiedStatesMask |= PassStateFlags::kComputePipeline;
            }
        }

        FE_Assert(Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kGraphicsPipeline | PassStateFlags::kComputePipeline),
                  "Either a graphics or a compute pipeline must be specified");

        const bool isGraphicsPipeline = Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kGraphicsPipeline);

        for (const RTTI::FieldInfo& field : descType->m_fields)
        {
            if (field.m_type == RTTI::GetTypeID<Core::PassColorTarget>())
            {
                for (uint32_t arrayIndex = 0; arrayIndex < field.m_arraySize; ++arrayIndex)
                {
                    FE_Assert(isGraphicsPipeline);

                    const Core::PassColorTarget colorTarget =
                        field.Get<Core::PassColorTarget>(pass.m_userPassDescPtr, arrayIndex);

                    const uint32_t colorTargetIndex = pass.m_colorTargetAccessIndices.size();
                    FE_Assert(colorTargetIndex < Core::Limits::Pipeline::kMaxColorAttachments, "Too many color targets");
                    FE_Assert(colorTarget.m_target.m_subresource.m_mipSliceCount == 1);
                    FE_Assert(colorTarget.m_target.m_subresource.m_arraySize == 1);

                    const uint32_t accessIndex = pass.m_accessedTextures.size();

                    TextureAccess access;
                    access.m_syncFlags = Core::BarrierSyncFlags::kRenderTarget;
                    access.m_accessFlags = Core::BarrierAccessFlags::kRenderTarget;
                    access.m_layout = Core::BarrierLayout::kRenderTarget;
                    access.m_subresource = colorTarget.m_target.m_subresource;
                    RegisterResource(colorTarget.m_target.m_resource, pass, access);
                    FE_AssertDebug(pass.m_accessedTextures.size() == accessIndex + 1);

                    pass.m_colorTargetAccessIndices.push_back(accessIndex);
                    pass.m_specifiedStatesMask |= PassStateFlags::kColorTarget;
                }
            }
            else if (field.m_type == RTTI::GetTypeID<Core::PassDepthTarget>())
            {
                FE_Assert(field.m_arraySize == 1);
                FE_Assert(isGraphicsPipeline);

                FE_Assert(!Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kDepthTarget));
                const Core::PassDepthTarget depthTarget = field.Get<Core::PassDepthTarget>(pass.m_userPassDescPtr);
                FE_Assert(depthTarget.m_target.m_subresource.m_mipSliceCount == 1);
                FE_Assert(depthTarget.m_target.m_subresource.m_arraySize == 1);

                const auto* graphicsPipeline = RTTI::AssertCast<const Core::GraphicsPipeline*>(pass.m_pipeline);
                const bool isDepthWriteEnabled = graphicsPipeline->GetDesc().m_depthStencil.m_depthWriteEnabled;

                auto accessFlags = Core::BarrierAccessFlags::kDepthStencilRead;
                if (isDepthWriteEnabled)
                    accessFlags |= Core::BarrierAccessFlags::kDepthStencilWrite;

                const uint32_t accessIndex = pass.m_accessedTextures.size();

                TextureAccess access;
                access.m_syncFlags = Core::BarrierSyncFlags::kDepthStencil;
                access.m_accessFlags = accessFlags;
                access.m_layout = Core::BarrierLayout::kDepthStencilWrite;
                access.m_subresource = depthTarget.m_target.m_subresource;
                RegisterResource(depthTarget.m_target.m_resource, pass, access);
                FE_AssertDebug(pass.m_accessedTextures.size() == accessIndex + 1);

                pass.m_depthTargetAccessIndex = accessIndex;
                pass.m_specifiedStatesMask |= PassStateFlags::kDepthTarget;
            }
            else if (field.m_type == RTTI::GetTypeID<Core::PassViewport>())
            {
                FE_Assert(field.m_arraySize == 1);
                FE_Assert(isGraphicsPipeline);

                FE_Assert(!Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kViewport));
                const Core::PassViewport viewport = field.Get<Core::PassViewport>(pass.m_userPassDescPtr);
                pass.m_viewport = viewport.m_rect;
                pass.m_specifiedStatesMask |= PassStateFlags::kViewport;
            }
            else if (field.m_type == RTTI::GetTypeID<Core::PassScissor>())
            {
                FE_Assert(field.m_arraySize == 1);
                FE_Assert(isGraphicsPipeline);

                FE_Assert(!Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kScissor));
                const Core::PassScissor scissor = field.Get<Core::PassScissor>(pass.m_userPassDescPtr);
                pass.m_scissor = scissor.m_rect;
                pass.m_specifiedStatesMask |= PassStateFlags::kScissor;
            }
            else
            {
                // Other arbitrary types are interpreted as push constants.
                FE_Assert(!Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kPushConstants));
                FE_Assert(field.m_arraySize == 1);

                const std::byte* pushConstantsPtr = static_cast<const std::byte*>(pass.m_userPassDescPtr) + field.m_offset;
                const uint32_t pushConstantsSize = field.m_size;
                FE_Assert(pushConstantsSize <= Core::Limits::Pipeline::kMaxPushConstantsByteSize);

                pass.m_pushConstants = festd::span(pushConstantsPtr, pushConstantsSize);
                pass.m_pushConstantsTypeID = field.m_type;
                pass.m_specifiedStatesMask |= PassStateFlags::kPushConstants;

                const RTTI::Type* pushConstantsType = RTTI::TypeRegistry::FindType(field.m_type);
                FE_Assert(pushConstantsType);
                FE_Assert(Bit::AllSet(pushConstantsType->m_flags, RTTI::TypeFlags::kTrivial | RTTI::TypeFlags::kStandardLayout));
                ParsePassPushConstants(pass, *pushConstantsType);
            }
        }
    }


    void FrameGraph::AccumulateBindFlags(ResourceNode& resource)
    {
        const Core::ResourceType resourceType = resource.m_resource->GetType();
        for (const PassResourceAccess& access : resource.m_accesses)
        {
            const PassNode& pass = m_passes[access.m_passIndex];

            if (resourceType == Core::ResourceType::kBuffer)
            {
                const BufferAccess& bufferAccess = pass.m_accessedBuffers[access.m_accessIndex];
                resource.m_bindFlags |= bufferAccess.m_accessFlags;
            }
            else if (resourceType == Core::ResourceType::kTexture)
            {
                const TextureAccess& textureAccess = pass.m_accessedTextures[access.m_accessIndex];
                resource.m_bindFlags |= textureAccess.m_accessFlags;
            }
            else
            {
                FE_DebugBreak();
            }
        }
    }


    void FrameGraph::CompilePassBarriers(PassNode& pass)
    {
        const uint32_t passIndex = pass.m_passIndex;

        for (const BufferAccess& access : pass.m_accessedBuffers)
        {
            ResourceNode& resourceNode = m_resources[access.m_localResourceIndex];
            FE_AssertDebug(resourceNode.m_resource->GetType() == Core::ResourceType::kBuffer);

            auto* buffer = RTTI::AssertCast<Buffer*>(resourceNode.m_resource.Get());
            if (buffer->GetMemoryStatus() == Core::ResourceMemory::kNotCommitted)
            {
                resourceNode.m_isOwnedByGraph = true;

                Core::BufferCommitParams commitParams;
                commitParams.m_bindFlags = resourceNode.m_bindFlags;
                commitParams.m_memory = Core::ResourceMemory::kDeviceLocal; // TODO: host visible resources
                m_resourcePool->CommitBufferMemory(buffer, commitParams);
            }

            const ResourceInstance* bufferInstance = buffer->GetInstance();
            FE_Assert(Bit::AllSet(bufferInstance->m_bind, resourceNode.m_bindFlags));

            SubresourceState state = buffer->GetState();
            const bool isReadToReadTransition = Core::IsReadAccess(state.m_access) && Core::IsReadAccess(access.m_accessFlags);
            const bool isCompatibleAccess = Bit::AllSet(state.m_access, access.m_accessFlags);
            const bool isCompatibleSync = Bit::AllSet(state.m_sync, access.m_syncFlags);

            if (state.m_queueType != Core::DeviceQueueType::kGraphics)
            {
                // For now, we assume that FrameGraph can only submit commands to the graphics queue.
                // So, the only possible transition is from AsyncCopyQueue (transfer queue).
                FE_Assert(state.m_queueType == Core::DeviceQueueType::kTransfer);

                // As per Vulkan spec, both acquire and release queue ownership transfer barriers must have
                // exactly the same subresource ranges which is kinda lame =P
                const auto transferBarrier = buffer->RetrieveQueueReleaseBarrier(Core::DeviceQueueType::kGraphics);
                FE_Assert(transferBarrier.has_value(), "Cannot transfer buffer ownership without matching release barrier");
                pass.m_bufferOwnershipTransferBarriers.push_back(transferBarrier.value());

                state.m_queueType = Core::DeviceQueueType::kGraphics;
                buffer->SetState(state);
            }

            Core::BufferBarrierDesc barrier;
            barrier.m_buffer = buffer;
            barrier.m_accessBefore = state.m_access;
            barrier.m_syncBefore = state.m_sync;
            barrier.m_accessAfter = access.m_accessFlags;
            barrier.m_syncAfter = access.m_syncFlags;
            barrier.m_queueBefore = Core::DeviceQueueType::kGraphics;
            barrier.m_queueAfter = Core::DeviceQueueType::kGraphics;

            //
            // Read-to-read barriers must be batched.
            // Consider this case:
            // | Pass 1         | Pass 2       |
            // | ComputeShading | PixelShading |
            // ^
            // | We need only one barrier here with sync = ComputeShading | PixelShading, so that both passes' work can overlap.
            if (isReadToReadTransition)
            {
                // Nothing to do here: the resource is already in the correct state.
                if (isCompatibleSync && isCompatibleAccess)
                    continue;

                if (resourceNode.m_previousBarrierPassIndex != kInvalidIndex)
                {
                    // Append flags to the previous barrier if there is one.
                    // Otherwise, we will have to create a new barrier.
                    PassNode& previousBarrierPass = m_passes[resourceNode.m_previousBarrierPassIndex];
                    FE_Verify(previousBarrierPass.m_barrierBatcher.AddBarrier(barrier));

                    state.m_access |= access.m_accessFlags;
                    state.m_sync |= access.m_syncFlags;
                    buffer->SetState(state);
                    continue;
                }
            }

            pass.m_barrierBatcher.AddBarrier(barrier);
            resourceNode.m_previousBarrierPassIndex = passIndex;

            state.m_access = access.m_accessFlags;
            state.m_sync = access.m_syncFlags;
            state.m_queueType = Core::DeviceQueueType::kGraphics;
            buffer->SetState(state);
        }

        for (const TextureAccess& access : pass.m_accessedTextures)
        {
            // We process each subresource separately and batch the resulting barriers later.
            FE_AssertDebug(access.m_subresource.m_mipSliceCount == 1 && access.m_subresource.m_arraySize == 1);

            ResourceNode& resourceNode = m_resources[access.m_localResourceIndex];
            FE_AssertDebug(resourceNode.m_resource->GetType() == Core::ResourceType::kTexture);

            auto* texture = RTTI::AssertCast<Texture*>(resourceNode.m_resource.Get());
            if (texture->GetMemoryStatus() == Core::ResourceMemory::kNotCommitted)
            {
                resourceNode.m_isOwnedByGraph = true;

                Core::TextureCommitParams commitParams;
                commitParams.m_bindFlags = resourceNode.m_bindFlags;
                commitParams.m_memory = Core::ResourceMemory::kDeviceLocal; // TODO: host visible resources
                m_resourcePool->CommitTextureMemory(texture, commitParams);
            }

            const ResourceInstance* textureInstance = texture->GetInstance();
            FE_Assert(Bit::AllSet(textureInstance->m_bind, resourceNode.m_bindFlags));

            SubresourceState state = texture->GetState(access.m_subresource);
            const bool isReadToReadTransition = Core::IsReadAccess(state.m_access) && Core::IsReadAccess(access.m_accessFlags);
            const bool isCompatibleAccess = Bit::AllSet(state.m_access, access.m_accessFlags);
            const bool isCompatibleSync = Bit::AllSet(state.m_sync, access.m_syncFlags);
            const bool isCompatibleLayout = state.m_layout == access.m_layout;

            if (state.m_queueType != Core::DeviceQueueType::kGraphics)
            {
                FE_Assert(state.m_queueType == Core::DeviceQueueType::kTransfer);

                const auto transferBarrier =
                    texture->RetrieveQueueReleaseBarrier(Core::DeviceQueueType::kGraphics, access.m_subresource);
                FE_Assert(transferBarrier.has_value(), "Cannot transfer texture ownership without matching release barrier");
                pass.m_textureOwnershipTransferBarriers.push_back(transferBarrier.value());

                state.m_queueType = Core::DeviceQueueType::kGraphics;
                texture->SetQueueOwnership(transferBarrier->m_subresource, Core::DeviceQueueType::kGraphics);
            }

            Core::TextureBarrierDesc barrier;
            barrier.m_texture = texture;
            barrier.m_subresource = access.m_subresource;
            barrier.m_layoutBefore = state.m_layout;
            barrier.m_layoutAfter = access.m_layout;
            barrier.m_accessBefore = state.m_access;
            barrier.m_accessAfter = access.m_accessFlags;
            barrier.m_syncBefore = state.m_sync;
            barrier.m_syncAfter = access.m_syncFlags;
            barrier.m_queueBefore = Core::DeviceQueueType::kGraphics;
            barrier.m_queueAfter = Core::DeviceQueueType::kGraphics;

            if (isReadToReadTransition && isCompatibleLayout)
            {
                if (isCompatibleSync && isCompatibleAccess)
                    continue;

                if (resourceNode.m_previousBarrierPassIndex != kInvalidIndex)
                {
                    PassNode& previousBarrierPass = m_passes[resourceNode.m_previousBarrierPassIndex];
                    FE_Verify(previousBarrierPass.m_barrierBatcher.AddBarrier(barrier));

                    state.m_access |= access.m_accessFlags;
                    state.m_sync |= access.m_syncFlags;
                    state.m_layout = access.m_layout;
                    texture->SetState(access.m_subresource, state);
                    continue;
                }
            }

            pass.m_barrierBatcher.AddBarrier(barrier);
            resourceNode.m_previousBarrierPassIndex = passIndex;

            state.m_access = access.m_accessFlags;
            state.m_sync = access.m_syncFlags;
            state.m_layout = access.m_layout;
            state.m_queueType = Core::DeviceQueueType::kGraphics;
            texture->SetState(access.m_subresource, state);
        }
    }


    void FrameGraph::Compile()
    {
        for (ResourceNode& resource : m_resources)
            AccumulateBindFlags(resource);

        for (PassNode& pass : m_passes)
            CompilePassBarriers(pass);
    }


    void FrameGraph::Execute()
    {
        PrepareExecuteInternal();
    }


    uint32_t FrameGraph::RegisterResource(Core::Resource* resource, PassNode& pass, const TextureAccess& access)
    {
        const uint32_t resourceID = resource->GetResourceID();
        const auto [it, inserted] = m_resourceIndexMap.insert({ resourceID, m_resources.size() });
        if (inserted)
        {
            ResourceNode& newResourceNode = m_resources.push_back();
            newResourceNode.m_resource = resource;
        }

        const uint32_t localResourceIndex = it->second;
        ResourceNode& resourceNode = m_resources[localResourceIndex];
        FE_Assert(resourceNode.m_resource == resource);

        const Core::TextureSubresourceIterator subresourceIterator(access.m_subresource);
        for (const auto [mipIndex, arrayIndex] : subresourceIterator)
        {
            const uint32_t passAccessIndex = pass.m_accessedTextures.size();
            resourceNode.m_accesses.push_back({ pass.m_passIndex, passAccessIndex });

            TextureAccess& subresourceAccess = pass.m_accessedTextures.emplace_back(access);
            subresourceAccess.m_localResourceIndex = localResourceIndex;
            subresourceAccess.m_subresource.m_mostDetailedMipSlice = mipIndex;
            subresourceAccess.m_subresource.m_firstArraySlice = arrayIndex;
            subresourceAccess.m_subresource.m_mipSliceCount = 1;
            subresourceAccess.m_subresource.m_arraySize = 1;
        }

        return localResourceIndex;
    }


    uint32_t FrameGraph::RegisterResource(Core::Resource* resource, PassNode& pass, const BufferAccess& access)
    {
        const uint32_t resourceID = resource->GetResourceID();
        const auto [it, inserted] = m_resourceIndexMap.insert({ resourceID, m_resources.size() });
        if (inserted)
        {
            ResourceNode& newResourceNode = m_resources.push_back();
            newResourceNode.m_resource = resource;
        }

        const uint32_t localResourceIndex = it->second;
        ResourceNode& resourceNode = m_resources[localResourceIndex];
        FE_Assert(resourceNode.m_resource == resource);

        const uint32_t passAccessIndex = pass.m_accessedBuffers.size();
        resourceNode.m_accesses.push_back({ pass.m_passIndex, passAccessIndex });

        BufferAccess& bufferAccess = pass.m_accessedBuffers.emplace_back(access);
        bufferAccess.m_localResourceIndex = localResourceIndex;

        return localResourceIndex;
    }
} // namespace FE::Graphics::Common
