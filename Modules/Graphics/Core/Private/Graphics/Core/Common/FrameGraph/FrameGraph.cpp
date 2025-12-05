#include <FeCore/RTTI/Reflection.h>
#include <Graphics/Core/Common/FrameGraph/FrameGraph.h>
#include <Graphics/Core/FrameGraph/FrameGraphPass.h>

namespace FE::Graphics::Common
{
    FrameGraph::PassNode::PassNode(std::pmr::memory_resource* allocator)
        : m_textureBarriers(allocator)
        , m_bufferBarriers(allocator)
        , m_accessedTextures(allocator)
        , m_accessedBuffers(allocator)
    {
        m_colorTargetLocalIndices.fill(kInvalidIndex);
    }


    void FrameGraph::CompileAndExecute()
    {
        Compile();
        Execute();
    }


    FrameGraph::ResourceNode::ResourceNode(std::pmr::memory_resource* allocator)
        : m_accesses(allocator)
    {
    }


    FrameGraph::FrameGraph(Core::DescriptorManager* descriptorManager)
        : m_descriptorManager(descriptorManager)
        , m_passes(&m_linearAllocator)
        , m_resources(&m_linearAllocator)
    {
    }


    FrameGraph::PassNodeBase& FrameGraph::AddPassInternal()
    {
        PassNode& passNode = m_passes.emplace_back(&m_linearAllocator);
        passNode.m_passIndex = m_passes.size() - 1;
        return passNode;
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

            constexpr uint32_t meshStageIndex = festd::to_underlying(Core::ShaderStage::kMesh);
            constexpr uint32_t amplificationStageIndex = festd::to_underlying(Core::ShaderStage::kAmplification);
            constexpr uint32_t vertexStageIndex = festd::to_underlying(Core::ShaderStage::kVertex);

            const bool hasMeshShader = pipelineDesc.m_shaders[meshStageIndex].IsValid();

            shaderSyncFlags = Core::BarrierSyncFlags::kPixelShading;
            if (hasMeshShader)
            {
                shaderSyncFlags |= Core::BarrierSyncFlags::kMeshShading;

                if (pipelineDesc.m_shaders[amplificationStageIndex].IsValid())
                    shaderSyncFlags |= Core::BarrierSyncFlags::kAmplificationShading;
            }
            else
            {
                FE_Assert(pipelineDesc.m_shaders[vertexStageIndex].IsValid());
                shaderSyncFlags |= Core::BarrierSyncFlags::kVertexShading;
            }
        }

        for (const RTTI::FieldInfo& field : type.m_fields)
        {
            if (field.m_type == RTTI::GetTypeID<TextureSRVDescriptor>())
            {
                const TextureSRVDescriptor descriptor = field.Get<TextureSRVDescriptor>(pass.m_userPassDescPtr);
                const Core::ResourceDescriptorInfo resourceInfo = m_descriptorManager->GetResourceInfo(descriptor.m_value);
                FE_Assert(resourceInfo.m_resource->GetType() == Core::ResourceType::kTexture);
                m_descriptorManager->CommitResourceDescriptor(descriptor.m_value, Core::DescriptorType::kSRV);

                const uint32_t accessIndex = pass.m_accessedTextures.size();
                TextureAccess& access = pass.m_accessedTextures.emplace_back();
                access.m_localResourceIndex = RegisterResource(resourceInfo.m_resource, pass.m_passIndex, accessIndex);
                access.m_syncFlags = shaderSyncFlags;
                access.m_accessFlags = Core::BarrierAccessFlags::kShaderRead;
                access.m_layout = Core::BarrierLayout::kShaderRead;
                access.m_subresource = resourceInfo.m_textureSubresource;
            }
            else if (field.m_type == RTTI::GetTypeID<TextureUAVDescriptor>())
            {
                const TextureUAVDescriptor descriptor = field.Get<TextureUAVDescriptor>(pass.m_userPassDescPtr);
                const Core::ResourceDescriptorInfo resourceInfo = m_descriptorManager->GetResourceInfo(descriptor.m_value);
                FE_Assert(resourceInfo.m_resource->GetType() == Core::ResourceType::kTexture);
                m_descriptorManager->CommitResourceDescriptor(descriptor.m_value, Core::DescriptorType::kUAV);

                const uint32_t accessIndex = pass.m_accessedTextures.size();
                TextureAccess& access = pass.m_accessedTextures.emplace_back();
                access.m_localResourceIndex = RegisterResource(resourceInfo.m_resource, pass.m_passIndex, accessIndex);
                access.m_syncFlags = shaderSyncFlags;
                access.m_accessFlags = Core::BarrierAccessFlags::kShaderWrite;
                access.m_layout = Core::BarrierLayout::kShaderReadWrite;
                access.m_subresource = resourceInfo.m_textureSubresource;
            }
            else if (field.m_type == RTTI::GetTypeID<BufferSRVDescriptor>())
            {
                const BufferSRVDescriptor descriptor = field.Get<BufferSRVDescriptor>(pass.m_userPassDescPtr);
                const Core::ResourceDescriptorInfo resourceInfo = m_descriptorManager->GetResourceInfo(descriptor.m_value);
                FE_Assert(resourceInfo.m_resource->GetType() == Core::ResourceType::kBuffer);
                m_descriptorManager->CommitResourceDescriptor(descriptor.m_value, Core::DescriptorType::kSRV);

                const uint32_t accessIndex = pass.m_accessedBuffers.size();
                BufferAccess& access = pass.m_accessedBuffers.emplace_back();
                access.m_localResourceIndex = RegisterResource(resourceInfo.m_resource, pass.m_passIndex, accessIndex);
                access.m_syncFlags = shaderSyncFlags;
                access.m_accessFlags = Core::BarrierAccessFlags::kShaderRead;
            }
            else if (field.m_type == RTTI::GetTypeID<BufferUAVDescriptor>())
            {
                const BufferUAVDescriptor descriptor = field.Get<BufferUAVDescriptor>(pass.m_userPassDescPtr);
                const Core::ResourceDescriptorInfo resourceInfo = m_descriptorManager->GetResourceInfo(descriptor.m_value);
                FE_Assert(resourceInfo.m_resource->GetType() == Core::ResourceType::kBuffer);
                m_descriptorManager->CommitResourceDescriptor(descriptor.m_value, Core::DescriptorType::kUAV);

                const uint32_t accessIndex = pass.m_accessedBuffers.size();
                BufferAccess& access = pass.m_accessedBuffers.emplace_back();
                access.m_localResourceIndex = RegisterResource(resourceInfo.m_resource, pass.m_passIndex, accessIndex);
                access.m_syncFlags = shaderSyncFlags;
                access.m_accessFlags = Core::BarrierAccessFlags::kShaderWrite;
            }
            else if (field.m_type == RTTI::GetTypeID<SamplerDescriptor>())
            {
                const SamplerDescriptor descriptor = field.Get<SamplerDescriptor>(pass.m_userPassDescPtr);
                m_descriptorManager->CommitSamplerDescriptor(descriptor.m_value);
            }
        }
    }


    void FrameGraph::CompilePass(PassNode& pass)
    {
        const RTTI::Type* descType = RTTI::TypeRegistry::FindType(pass.m_userPassDescTypeID);
        FE_Assert(descType);

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
                FE_Assert(isGraphicsPipeline);

                const uint32_t accessIndex = pass.m_accessedTextures.size();
                const Core::PassColorTarget colorTarget = field.Get<Core::PassColorTarget>(pass.m_userPassDescPtr);
                const uint32_t localResourceIndex = RegisterResource(colorTarget.m_target, pass.m_passIndex, accessIndex);

                uint32_t colorTargetIndex;
                if (colorTarget.m_explicitIndex != kInvalidIndex)
                {
                    colorTargetIndex = colorTarget.m_explicitIndex;
                    FE_Assert(colorTarget.m_explicitIndex < Core::Limits::Pipeline::kMaxColorAttachments);
                    FE_Assert(pass.m_colorTargetLocalIndices[colorTarget.m_explicitIndex] == kInvalidIndex);
                }
                else
                {
                    colorTargetIndex = festd::find_index(pass.m_colorTargetLocalIndices, kInvalidIndex);
                    FE_Assert(colorTargetIndex != kInvalidIndex, "Too many color targets");
                }

                pass.m_colorTargetLocalIndices[colorTargetIndex] = localResourceIndex;
                pass.m_specifiedStatesMask |= PassStateFlags::kColorTarget;

                TextureAccess& access = pass.m_accessedTextures.emplace_back();
                access.m_localResourceIndex = localResourceIndex;
                access.m_syncFlags = Core::BarrierSyncFlags::kRenderTarget;
                access.m_accessFlags = Core::BarrierAccessFlags::kRenderTarget;
                access.m_layout = Core::BarrierLayout::kRenderTarget;
                access.m_subresource = Core::TextureSubresource::CreateWhole(colorTarget.m_target->GetDesc());
            }
            else if (field.m_type == RTTI::GetTypeID<Core::PassDepthTarget>())
            {
                FE_Assert(isGraphicsPipeline);

                FE_Assert(!Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kDepthTarget));
                const uint32_t accessIndex = pass.m_accessedBuffers.size();
                const Core::PassDepthTarget depthTarget = field.Get<Core::PassDepthTarget>(pass.m_userPassDescPtr);
                pass.m_depthTargetLocalIndex = RegisterResource(depthTarget.m_target, pass.m_passIndex, accessIndex);
                pass.m_specifiedStatesMask |= PassStateFlags::kDepthTarget;

                const auto* graphicsPipeline = RTTI::AssertCast<const Core::GraphicsPipeline*>(pass.m_pipeline);
                const bool isDepthWriteEnabled = graphicsPipeline->GetDesc().m_depthStencil.m_depthWriteEnabled;

                auto accessFlags = Core::BarrierAccessFlags::kDepthStencilRead;
                if (isDepthWriteEnabled)
                    accessFlags |= Core::BarrierAccessFlags::kDepthStencilWrite;

                TextureAccess& access = pass.m_accessedTextures.emplace_back();
                access.m_localResourceIndex = pass.m_depthTargetLocalIndex;
                access.m_syncFlags = Core::BarrierSyncFlags::kDepthStencil;
                access.m_accessFlags = accessFlags;
                access.m_layout = Core::BarrierLayout::kDepthStencilWrite;
                access.m_subresource = Core::TextureSubresource::CreateWhole(depthTarget.m_target->GetDesc());
            }
            else if (field.m_type == RTTI::GetTypeID<Core::PassViewport>())
            {
                FE_Assert(isGraphicsPipeline);

                FE_Assert(!Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kViewport));
                const Core::PassViewport viewport = field.Get<Core::PassViewport>(pass.m_userPassDescPtr);
                pass.m_viewport = viewport.m_rect;
                pass.m_specifiedStatesMask |= PassStateFlags::kViewport;
            }
            else if (field.m_type == RTTI::GetTypeID<Core::PassScissor>())
            {
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


    void FrameGraph::Compile()
    {
        for (PassNode& pass : m_passes)
        {
            CompilePass(pass);
        }
    }


    void FrameGraph::Execute() {}


    uint32_t FrameGraph::RegisterResource(Core::Resource* resource, const uint32_t passIndex, const uint32_t accessIndex)
    {
        const uint32_t resourceID = resource->GetResourceID();
        const auto [it, inserted] = m_resourceIndexMap.insert({ resourceID, m_resources.size() });
        if (inserted)
        {
            ResourceNode& newResourceNode = m_resources.push_back();
            newResourceNode.m_resource = resource;
        }

        ResourceNode& resourceNode = m_resources[it->second];
        FE_Assert(resourceNode.m_resource == resource);
        resourceNode.m_accesses.push_back({ passIndex, accessIndex });
        return it->second;
    }
} // namespace FE::Graphics::Common
