#include <Core/RTTI/Reflection.h>
#include <Core/RTTI/Traversal.h>
#include <Graphics/Core/Common/Buffer.h>
#include <Graphics/Core/Common/FrameGraph/FrameGraph.h>

#include <Core/Memory/FiberTempAllocator.h>
#include <Graphics/Core/Common/Texture.h>
#include <Graphics/Core/FrameGraph/FrameGraphPass.h>
#include <Graphics/Core/Vulkan/ComputePipeline.h>

namespace FE::Graphics::Common
{
    namespace
    {
        bool HasShaderStage(const Core::GraphicsPipelineDesc& pipelineDesc, const Core::ShaderStage stage)
        {
            return pipelineDesc.m_shaders[festd::to_underlying(stage)].IsValid();
        }


        //! Pass desc fields hold one pass state or a C array of them.
        struct PassFieldElements final
        {
            const Rtti::Type* m_type = nullptr;
            uint32_t m_count = 1;
            uint32_t m_stride = 0;

            template<class T>
            [[nodiscard]] const T& Get(const Rtti::FieldInfo& field, const void* data, const uint32_t index = 0) const
            {
                FE_AssertDebug(index < m_count);
                return *reinterpret_cast<const T*>(static_cast<const std::byte*>(field.GetAddress(data)) + index * m_stride);
            }
        };


        PassFieldElements GetPassFieldElements(const Rtti::FieldInfo& field)
        {
            if (const auto* array = field.m_type->TryGetShape<Rtti::FixedArrayShape>())
                return { array->m_element, array->m_count, array->m_stride };

            return { field.m_type, 1, field.m_size };
        }


        //! Calls the visitor with the access a value of the node's type declares, if any.
        template<class TDescriptorAccess, class TVisitor>
        void VisitDescriptorNode(const Rtti::VisitNode& node, TVisitor&& visitor)
        {
            using Kind = typename TDescriptorAccess::Kind;

            const Rtti::TypeID id = node.m_type.m_id;
            if (id == Rtti::GetTypeID<TextureSRVDescriptor>())
                visitor(TDescriptorAccess{ Kind::kTextureSRV, static_cast<const TextureSRVDescriptor*>(node.m_value)->m_value });
            else if (id == Rtti::GetTypeID<TextureUAVDescriptor>())
                visitor(TDescriptorAccess{ Kind::kTextureUAV, static_cast<const TextureUAVDescriptor*>(node.m_value)->m_value });
            else if (id == Rtti::GetTypeID<BufferSRVDescriptor>())
                visitor(TDescriptorAccess{ Kind::kBufferSRV, static_cast<const BufferSRVDescriptor*>(node.m_value)->m_value });
            else if (id == Rtti::GetTypeID<BufferUAVDescriptor>())
                visitor(TDescriptorAccess{ Kind::kBufferUAV, static_cast<const BufferUAVDescriptor*>(node.m_value)->m_value });
        }


        bool AreOverlapping(const Core::BufferSlice lhs, const Core::BufferSlice rhs)
        {
            return lhs.m_offset < rhs.m_offset + rhs.m_size && rhs.m_offset < lhs.m_offset + lhs.m_size;
        }
    } // namespace


    FrameGraph::PassNode::PassNode(std::pmr::memory_resource* allocator)
        : m_textureOwnershipTransferBarriers(allocator)
        , m_bufferOwnershipTransferBarriers(allocator)
        , m_texturePostOwnershipBarriers(allocator)
        , m_ownershipTransferWaits(allocator)
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
        FE_Assert(m_scopeDepth == 0, "Frame graph scopes must end before execution");
        Compile();
        Execute();

        m_blackboard.Reset();
        for (Core::FrameGraphBlackboard* blackboard : m_blackboards)
            blackboard->Reset();

        m_blackboards.clear();
        m_passes.clear();
        m_scopeEvents.clear();
        m_resources.clear();
        m_resourceIndexMap.clear();
        m_basePassDescs.clear();
        m_descriptorViews.clear();
        m_textureViews.clear();
        m_bufferViews.clear();
        m_constantsAccesses.clear();
        m_uploadCopies.clear();

        // These keep their storage after clear(), but the pages are about to be recycled.
        m_passes.reset_lose_memory();
        m_scopeEvents.reset_lose_memory();
        m_resources.reset_lose_memory();
        m_linearAllocator.Clear();
    }


    AnyConstantsPtr FrameGraph::CreateConstantsInternal(const Rtti::Type& type, const void* data)
    {
        FE_PROFILER_ZONE();
        FE_AssertDebug(type.m_alignment <= ConstantAllocator::kAlignment);

        const ConstantAllocator::Allocation allocation = m_constantAllocator.Allocate(type.m_size);
        memcpy(allocation.m_hostPointer, data, type.m_size);

        // Nested constants exist before the constants that reference them, so their lists are already complete.
        Memory::FiberTempAllocator temp;
        festd::pmr::vector<DescriptorAccess> accesses{ &temp };
        Rtti::VisitConst(type, data, Rtti::VisitFlags::kNone, [&](const Rtti::VisitNode& node) {
            if (node.m_type.m_id == Rtti::GetTypeID<AnyConstantsPtr>())
            {
                const auto* nested = static_cast<const AnyConstantsPtr*>(node.m_value);
                const auto entry = m_constantsAccesses.find(nested->m_ptr.GetDeviceAddress());
                if (entry != m_constantsAccesses.end())
                    accesses.insert(accesses.end(), entry->second.begin(), entry->second.end());
            }
            else
            {
                VisitDescriptorNode<DescriptorAccess>(node, [&](const DescriptorAccess access) {
                    if (access.m_descriptor != kInvalidIndex)
                        accesses.push_back(access);
                });
            }

            return Rtti::VisitResult::kContinue;
        });

        if (!accesses.empty())
        {
            auto* stored = static_cast<DescriptorAccess*>(
                m_linearAllocator.allocate(accesses.size() * sizeof(DescriptorAccess), alignof(DescriptorAccess)));
            festd::copy(accesses, stored);
            m_constantsAccesses.emplace(allocation.m_deviceAddress, festd::span<const DescriptorAccess>(stored, accesses.size()));
        }

        AnyConstantsPtr result;
        result.m_ptr = BufferPointer{ allocation.m_deviceAddress };
        return result;
    }


    void FrameGraph::BeginScope(const festd::string_view name)
    {
        EnsureUploadPasses();

        if (Build::IsDebug())
        {
            for (const int32_t codepoint : name)
                FE_Assert(ASCII::IsValid(codepoint));
        }

        m_scopeEvents.push_back({ Env::Name(name), m_passes.size(), true });
        ++m_scopeDepth;
    }


    void FrameGraph::EndScope()
    {
        FE_Assert(m_scopeDepth != 0, "No frame graph scope to end");
        m_scopeEvents.push_back({ {}, m_passes.size(), false });
        --m_scopeDepth;
    }


    TextureSRVDescriptor FrameGraph::GetSRV(const Core::TextureView texture)
    {
        return { ReserveDescriptor(texture, Core::DescriptorType::kSRV) };
    }


    TextureUAVDescriptor FrameGraph::GetUAV(const Core::TextureView texture)
    {
        return { ReserveDescriptor(texture, Core::DescriptorType::kUAV) };
    }


    BufferSRVDescriptor FrameGraph::GetSRV(const Core::BufferView buffer)
    {
        return { ReserveDescriptor(buffer, Core::DescriptorType::kSRV) };
    }


    BufferUAVDescriptor FrameGraph::GetUAV(const Core::BufferView buffer)
    {
        return { ReserveDescriptor(buffer, Core::DescriptorType::kUAV) };
    }


    uint32_t FrameGraph::ReserveDescriptor(const Core::TextureView texture, const Core::DescriptorType type)
    {
        FE_Assert(texture.IsValid());
        const TextureViewKey key{ texture.m_resource->GetResourceID(), texture.m_subresource, type };
        const auto it = m_textureViews.find(key);
        if (it != m_textureViews.end())
            return it->second;

        FE_Assert(m_descriptorViews.size() < Core::DescriptorManager::kTransientDescriptorCount,
                  "Frame descriptor arena exhausted");
        const uint32_t index = Core::DescriptorManager::kPersistentDescriptorCount + m_descriptorViews.size();
        m_descriptorViews.emplace_back(texture, type);
        m_textureViews.emplace(key, index);
        return index;
    }


    uint32_t FrameGraph::ReserveDescriptor(const Core::BufferView buffer, const Core::DescriptorType type)
    {
        FE_Assert(buffer.IsValid());
        const BufferViewKey key{ buffer.m_resource->GetResourceID(), buffer.m_slice, type };
        const auto it = m_bufferViews.find(key);
        if (it != m_bufferViews.end())
            return it->second;

        FE_Assert(m_descriptorViews.size() < Core::DescriptorManager::kTransientDescriptorCount,
                  "Frame descriptor arena exhausted");
        const uint32_t index = Core::DescriptorManager::kPersistentDescriptorCount + m_descriptorViews.size();
        m_descriptorViews.emplace_back(buffer, type);
        m_bufferViews.emplace(key, index);
        return index;
    }


    const Core::ResourceDescriptorInfo& FrameGraph::GetResourceInfo(const uint32_t index, const Core::DescriptorType type) const
    {
        if (index < Core::DescriptorManager::kPersistentDescriptorCount)
        {
            const auto& info = m_descriptorManager->GetPersistentResourceInfo(index);
            FE_Assert(info.m_descriptorType == type);
            return info;
        }

        const uint32_t localIndex = index - Core::DescriptorManager::kPersistentDescriptorCount;
        FE_Assert(localIndex < m_descriptorViews.size(), "Descriptor does not belong to this frame");
        const auto& info = m_descriptorViews[localIndex];
        FE_Assert(info.m_descriptorType == type);
        return info;
    }


    SamplerDescriptor FrameGraph::GetSampler(const Core::SamplerState sampler)
    {
        return m_descriptorManager->GetSampler(sampler);
    }


    FrameGraph::FrameGraph(Core::Device* device, Core::DescriptorManager* descriptorManager, Core::ResourcePool* resourcePool)
        : m_constantAllocator(resourcePool)
        , m_passes(&m_linearAllocator)
        , m_scopeEvents(&m_linearAllocator)
        , m_resources(&m_linearAllocator)
    {
        m_device = device;
        m_descriptorManager = descriptorManager;
        m_resourcePool = resourcePool;
    }


    void FrameGraph::AddCopyPass(const Core::BufferView& destination, const Core::BufferView& source)
    {
        const auto execute = [source, destination](Core::FrameGraphContext& context) {
            context.Copy(destination, source);
        };

        auto* passDesc = AllocatePassData<InternalPassDesc::CopyBufferToBuffer>();
        passDesc->m_destination = { destination, Core::BarrierSyncFlags::kCopy, Core::BarrierAccessFlags::kCopyDest };
        passDesc->m_source = { source, Core::BarrierSyncFlags::kCopy, Core::BarrierAccessFlags::kCopySource };
        AddPass(Fmt::FixedFormat("Copy '{}' to '{}'", source.GetName(), destination.GetName()), passDesc, execute);
    }


    void FrameGraph::AddUploadCopy(const Core::BufferView& destination, const Core::BufferView& source)
    {
        EnsureUploadPasses();
        if (!CanBatchUploadCopy(destination))
        {
            AddCopyPass(destination, source);
            return;
        }

        m_uploadCopies.push_back({ destination, source });

        PassNode& uploadPass = m_passes[kUploadPassIndex];
        BufferAccess write;
        write.m_syncFlags = Core::BarrierSyncFlags::kCopy;
        write.m_accessFlags = Core::BarrierAccessFlags::kCopyDest;
        RegisterResource(destination.m_resource, uploadPass, write);

        BufferAccess read;
        read.m_syncFlags = Core::BarrierSyncFlags::kCopy;
        read.m_accessFlags = Core::BarrierAccessFlags::kCopySource;
        RegisterResource(source.m_resource, uploadPass, read);
    }


    void FrameGraph::EnsureUploadPasses()
    {
        if (!m_passes.empty())
            return;

        static const Env::Name kUploadPassName{ "Uploads" };
        static const Env::Name kUploadReadPassName{ "UploadReads" };

        PassNode& uploadPass = m_passes.emplace_back(&m_linearAllocator);
        uploadPass.m_passIndex = kUploadPassIndex;
        uploadPass.m_name = kUploadPassName;
        uploadPass.m_functor = this;
        uploadPass.m_execute = &ExecuteUploadCopies;
        uploadPass.m_destroy = [](void*, void*) {};

        PassNode& readPass = m_passes.emplace_back(&m_linearAllocator);
        readPass.m_passIndex = kUploadReadPassIndex;
        readPass.m_name = kUploadReadPassName;
    }


    bool FrameGraph::CanBatchUploadCopy(const Core::BufferView& destination) const
    {
        // A batched copy runs before every pass, which keeps the order of accesses only if the earlier ones are uploads.
        const auto resource = m_resourceIndexMap.find(destination.m_resource->GetResourceID());
        if (resource == m_resourceIndexMap.end())
            return true;

        for (const PassResourceAccess& access : m_resources[resource->second].m_accesses)
        {
            if (access.m_passIndex != kUploadPassIndex)
                return false;
        }

        // The copies of a batch are not ordered against each other.
        for (const UploadCopy& upload : m_uploadCopies)
        {
            if (upload.m_destination.m_resource != destination.m_resource)
                continue;

            if (AreOverlapping(upload.m_destination.m_slice, destination.m_slice))
                return false;
        }

        return true;
    }


    void FrameGraph::AddUploadReadAccesses()
    {
        if (m_uploadCopies.empty())
            return;

        PassNode& uploadPass = m_passes[kUploadPassIndex];
        PassNode& readPass = m_passes[kUploadReadPassIndex];
        for (const BufferAccess& upload : uploadPass.m_accessedBuffers)
        {
            if (!Bit::AllSet(upload.m_accessFlags, Core::BarrierAccessFlags::kCopyDest))
                continue;

            // The reads up to the first write share one transition, after which their passes need no barriers.
            ResourceNode& resourceNode = m_resources[upload.m_localResourceIndex];
            BufferAccess reads;
            reads.m_localResourceIndex = upload.m_localResourceIndex;
            for (const PassResourceAccess& access : resourceNode.m_accesses)
            {
                if (access.m_passIndex == kUploadPassIndex)
                    continue;

                const BufferAccess& later = m_passes[access.m_passIndex].m_accessedBuffers[access.m_accessIndex];
                if (!Core::IsReadAccess(later.m_accessFlags))
                    break;

                reads.m_syncFlags |= later.m_syncFlags;
                reads.m_accessFlags |= later.m_accessFlags;
            }

            if (reads.m_accessFlags == Core::BarrierAccessFlags::kNone)
                continue;

            resourceNode.m_accesses.push_back({ kUploadReadPassIndex, readPass.m_accessedBuffers.size() });
            readPass.m_accessedBuffers.push_back(reads);
        }
    }


    void FrameGraph::ExecuteUploadCopies(void* functor, Core::FrameGraphContext& context)
    {
        const auto* graph = static_cast<const FrameGraph*>(functor);
        for (const UploadCopy& upload : graph->m_uploadCopies)
            context.Copy(upload.m_destination, upload.m_source);
    }


    void FrameGraph::AddPassInternal(const PassNodeDesc& desc)
    {
        FE_PROFILER_ZONE();

        FE_Assert((desc.m_functor == nullptr && desc.m_execute == nullptr && desc.m_destroy == nullptr)
                  || (desc.m_functor != nullptr && desc.m_execute != nullptr && desc.m_destroy != nullptr));
        FE_Assert(desc.m_userPassDescTypeID != Rtti::TypeID::kNull, "PassDesc must be registered with RTTI");

        EnsureUploadPasses();
        PassNode& passNode = m_passes.emplace_back(&m_linearAllocator);
        passNode.m_passIndex = m_passes.size() - 1;
        passNode.m_name = desc.m_name;
        passNode.m_functor = desc.m_functor;
        passNode.m_execute = desc.m_execute;
        passNode.m_destroy = desc.m_destroy;
        passNode.m_userPassDescTypeID = desc.m_userPassDescTypeID;
        passNode.m_userPassDescPtr = desc.m_userPassDescPtr;
        passNode.m_basePassDescToken = desc.m_basePassDescToken;
        if (desc.m_computePipeline != nullptr)
            PrepareComputePass(passNode, desc);
        else
            PreparePassCompileInfo(passNode);
    }


    void FrameGraph::ParsePassPushConstants(PassNode& pass, const Rtti::Type& type, const void* data)
    {
        FE_PROFILER_ZONE();

        // Push constants can contain resource descriptor indices.
        // We have to account for the corresponding resources when compiling pass barriers.

        Core::BarrierSyncFlags shaderSyncFlags;
        if (Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kComputePipeline))
        {
            shaderSyncFlags = Core::BarrierSyncFlags::kComputeShading;
        }
        else
        {
            FE_Assert(Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kGraphicsPipeline),
                      "Push constants can only be used in passes with graphics or compute pipelines");

            const auto* graphicsPipeline = Rtti::AssertCast<const Core::GraphicsPipeline*>(pass.m_pipeline.Get());
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

        // Descriptor fields and arrays of them name the resources a pass accesses.
        Rtti::VisitConst(type, data, Rtti::VisitFlags::kNone, [&](const Rtti::VisitNode& node) {
            if (node.m_type.m_id == Rtti::GetTypeID<AnyConstantsPtr>())
            {
                RegisterConstantsAccesses(pass, *static_cast<const AnyConstantsPtr*>(node.m_value), shaderSyncFlags);
            }
            else
            {
                VisitDescriptorNode<DescriptorAccess>(node, [&](const DescriptorAccess access) {
                    RegisterDescriptorAccess(pass, access, shaderSyncFlags);
                });
            }

            return Rtti::VisitResult::kContinue;
        });
    }


    void FrameGraph::RegisterDescriptorAccess(PassNode& pass, const DescriptorAccess access,
                                              const Core::BarrierSyncFlags shaderSyncFlags)
    {
        if (access.m_descriptor == kInvalidIndex)
            return;

        const auto registerTexture =
            [&](const Core::DescriptorType type, const Core::BarrierAccessFlags flags, const Core::BarrierLayout layout) {
                const Core::ResourceDescriptorInfo& info = GetResourceInfo(access.m_descriptor, type);
                FE_Assert(info.m_resource->GetType() == Core::ResourceType::kTexture);

                TextureAccess textureAccess;
                textureAccess.m_syncFlags = shaderSyncFlags;
                textureAccess.m_accessFlags = flags;
                textureAccess.m_layout = layout;
                textureAccess.m_subresource = info.m_textureSubresource;
                RegisterResource(info.m_resource.Get(), pass, textureAccess);
            };

        const auto registerBuffer = [&](const Core::DescriptorType type, const Core::BarrierAccessFlags flags) {
            const Core::ResourceDescriptorInfo& info = GetResourceInfo(access.m_descriptor, type);
            FE_Assert(info.m_resource->GetType() == Core::ResourceType::kBuffer);

            BufferAccess bufferAccess;
            bufferAccess.m_syncFlags = shaderSyncFlags;
            bufferAccess.m_accessFlags = flags;
            RegisterResource(info.m_resource.Get(), pass, bufferAccess);
        };

        constexpr auto kReadWrite = Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite;
        switch (access.m_kind)
        {
        case DescriptorAccess::Kind::kTextureSRV:
            registerTexture(Core::DescriptorType::kSRV, Core::BarrierAccessFlags::kShaderRead, Core::BarrierLayout::kShaderRead);
            break;

        case DescriptorAccess::Kind::kTextureUAV:
            registerTexture(Core::DescriptorType::kUAV, kReadWrite, Core::BarrierLayout::kShaderReadWrite);
            break;

        case DescriptorAccess::Kind::kBufferSRV:
            registerBuffer(Core::DescriptorType::kSRV, Core::BarrierAccessFlags::kShaderRead);
            break;

        case DescriptorAccess::Kind::kBufferUAV:
            registerBuffer(Core::DescriptorType::kUAV, kReadWrite);
            break;

        case DescriptorAccess::Kind::kNone:
        default:
            FE_DebugBreak();
            break;
        }
    }


    void FrameGraph::RegisterConstantsAccesses(PassNode& pass, const AnyConstantsPtr constants,
                                               const Core::BarrierSyncFlags shaderSyncFlags)
    {
        if (!constants.IsValid())
            return;

        const uint64_t address = constants.m_ptr.GetDeviceAddress();
        FE_AssertDebug(m_constantAllocator.IsCurrentFrameAddress(address), "Constants do not belong to this frame");

        const auto entry = m_constantsAccesses.find(address);
        if (entry == m_constantsAccesses.end())
            return;

        for (const DescriptorAccess access : entry->second)
            RegisterDescriptorAccess(pass, access, shaderSyncFlags);
    }


    void FrameGraph::SetPassPushConstants(PassNode& pass, const Rtti::Type& type, const void* data)
    {
        FE_Assert(!Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kPushConstants));
        FE_Assert(type.m_size <= Core::Limits::Pipeline::kMaxPushConstantsByteSize);
        FE_Assert(Bit::AllSet(type.m_flags, Rtti::TypeFlags::kStandardLayout));

        pass.m_pushConstants = festd::span(static_cast<const std::byte*>(data), type.m_size);
        pass.m_specifiedStatesMask |= PassStateFlags::kPushConstants;
        ParsePassPushConstants(pass, type, data);
    }


    void FrameGraph::SetPassIndirectArgs(PassNode& pass, const Core::BufferView arguments)
    {
        FE_Assert(!Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kIndirectArgs));
        FE_Assert(arguments.IsValid());

        BufferAccess access;
        access.m_syncFlags = Core::BarrierSyncFlags::kExecuteIndirect;
        access.m_accessFlags = Core::BarrierAccessFlags::kIndirectArgument;
        RegisterResource(arguments.m_resource, pass, access);

        pass.m_indirectArgs = arguments;
        pass.m_specifiedStatesMask |= PassStateFlags::kIndirectArgs;
    }


    void FrameGraph::PrepareComputePass(PassNode& pass, const PassNodeDesc& desc)
    {
        FE_PROFILER_ZONE_TEXT("%s", pass.m_name.c_str());

        pass.m_pipeline = const_cast<Core::ComputePipeline*>(desc.m_computePipeline);
        pass.m_specifiedStatesMask |= PassStateFlags::kComputePipeline;
        if (desc.m_indirectArgs.IsValid())
            SetPassIndirectArgs(pass, desc.m_indirectArgs);

        SetPassPushConstants(pass, *desc.m_constantsType, desc.m_constants);
    }


    void FrameGraph::PreparePassCompileInfo(PassNode& pass)
    {
        FE_PROFILER_ZONE_TEXT("%s", pass.m_name.c_str());

        const Rtti::Type* descType = Rtti::TypeRegistry::FindType(pass.m_userPassDescTypeID);
        FE_AssertDebug(descType);

        struct PassFieldBinding final
        {
            const Rtti::FieldInfo* m_field = nullptr;
            const void* m_data = nullptr;
        };

        Memory::FiberTempAllocator temp;
        festd::pmr::vector<PassFieldBinding> fields{ &temp };
        if (pass.m_basePassDescToken.IsValid())
        {
            FE_Assert(pass.m_basePassDescToken.m_value < m_basePassDescs.size());
            const BasePassDesc& base = m_basePassDescs[pass.m_basePassDescToken.m_value];
            const Rtti::Type* baseType = Rtti::TypeRegistry::FindType(base.m_typeID);
            FE_Assert(baseType);

            fields.reserve(Math::Max(baseType->m_fields.size(), descType->m_fields.size()));
            for (const Rtti::FieldInfo& baseField : baseType->m_fields)
            {
                bool isOverridden = false;
                const Rtti::TypeID baseFieldType = GetPassFieldElements(baseField).m_type->m_id;
                for (const Rtti::FieldInfo& field : descType->m_fields)
                {
                    const Rtti::TypeID fieldType = GetPassFieldElements(field).m_type->m_id;
                    if (baseFieldType == fieldType)
                    {
                        const bool isResourceAccess = fieldType == Rtti::GetTypeID<Core::PassBufferAccess>()
                            || fieldType == Rtti::GetTypeID<Core::PassTextureAccess>();
                        isOverridden |= !isResourceAccess;
                    }

                    const bool pipelineMismatchGraphics = baseFieldType == Rtti::GetTypeID<Core::PassGraphicsPipeline>()
                        && fieldType == Rtti::GetTypeID<Core::PassComputePipeline>();
                    const bool pipelineMismatchCompute = baseFieldType == Rtti::GetTypeID<Core::PassComputePipeline>()
                        && fieldType == Rtti::GetTypeID<Core::PassGraphicsPipeline>();
                    FE_Assert(!pipelineMismatchGraphics && !pipelineMismatchCompute,
                              "Base and child pass pipelines must have the same type");
                }

                if (!isOverridden)
                    fields.push_back({ &baseField, base.m_data });
            }
        }
        else
        {
            fields.reserve(descType->m_fields.size());
        }

        for (const Rtti::FieldInfo& field : descType->m_fields)
            fields.push_back({ &field, pass.m_userPassDescPtr });

        for (const PassFieldBinding& binding : fields)
        {
            const Rtti::FieldInfo& field = *binding.m_field;
            const PassFieldElements elements = GetPassFieldElements(field);
            const Rtti::TypeID fieldType = elements.m_type->m_id;
            if (fieldType == Rtti::GetTypeID<Core::PassGraphicsPipeline>())
            {
                FE_Assert(!Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kGraphicsPipeline));
                const Core::PassGraphicsPipeline pipeline = elements.Get<Core::PassGraphicsPipeline>(field, binding.m_data);
                pass.m_pipeline = const_cast<Core::GraphicsPipeline*>(pipeline.m_pipeline);
                pass.m_specifiedStatesMask |= PassStateFlags::kGraphicsPipeline;
            }
            else if (fieldType == Rtti::GetTypeID<Core::PassComputePipeline>())
            {
                FE_Assert(!Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kComputePipeline));
                const Core::PassComputePipeline pipeline = elements.Get<Core::PassComputePipeline>(field, binding.m_data);
                pass.m_pipeline = const_cast<Core::ComputePipeline*>(pipeline.m_pipeline);
                pass.m_specifiedStatesMask |= PassStateFlags::kComputePipeline;
            }
        }

        const bool isGraphicsPipeline = Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kGraphicsPipeline);
        const bool isGraphicsPass = !Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kComputePipeline);

        for (const PassFieldBinding& binding : fields)
        {
            const Rtti::FieldInfo& field = *binding.m_field;
            const PassFieldElements elements = GetPassFieldElements(field);
            const Rtti::TypeID fieldType = elements.m_type->m_id;
            if (fieldType == Rtti::GetTypeID<Core::PassTextureAccess>())
            {
                for (uint32_t arrayIndex = 0; arrayIndex < elements.m_count; ++arrayIndex)
                {
                    const Core::PassTextureAccess textureAccess =
                        elements.Get<Core::PassTextureAccess>(field, binding.m_data, arrayIndex);
                    if (textureAccess.m_texture == nullptr)
                        continue;

                    TextureAccess access;
                    access.m_syncFlags = textureAccess.m_syncFlags;
                    access.m_accessFlags = textureAccess.m_accessFlags;
                    access.m_layout = textureAccess.m_layout;
                    access.m_subresource = textureAccess.m_subresource;
                    RegisterResource(textureAccess.m_texture, pass, access);
                }
            }
            else if (fieldType == Rtti::GetTypeID<Core::PassBufferAccess>())
            {
                for (uint32_t arrayIndex = 0; arrayIndex < elements.m_count; ++arrayIndex)
                {
                    const Core::PassBufferAccess bufferAccess =
                        elements.Get<Core::PassBufferAccess>(field, binding.m_data, arrayIndex);
                    if (bufferAccess.m_buffer == nullptr)
                        continue;

                    BufferAccess access;
                    access.m_syncFlags = bufferAccess.m_syncFlags;
                    access.m_accessFlags = bufferAccess.m_accessFlags;
                    RegisterResource(bufferAccess.m_buffer, pass, access);
                }
            }
            else if (fieldType == Rtti::GetTypeID<Core::PassIndirectArgs>())
            {
                FE_Assert(elements.m_count == 1, "Indirect arguments can only be specified once");
                SetPassIndirectArgs(pass, elements.Get<Core::PassIndirectArgs>(field, binding.m_data).m_arguments);
            }
            else if (fieldType == Rtti::GetTypeID<Core::PassColorTarget>())
            {
                pass.m_specifiedStatesMask |= PassStateFlags::kRenderTargets;
                for (uint32_t arrayIndex = 0; arrayIndex < elements.m_count; ++arrayIndex)
                {
                    FE_Assert(isGraphicsPass);

                    const Core::PassColorTarget colorTarget =
                        elements.Get<Core::PassColorTarget>(field, binding.m_data, arrayIndex);
                    if (!colorTarget.m_target.IsValid())
                        continue;

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
            else if (fieldType == Rtti::GetTypeID<Core::PassDepthTarget>())
            {
                pass.m_specifiedStatesMask |= PassStateFlags::kRenderTargets;
                FE_Assert(elements.m_count == 1, "Depth target can only be specified once");
                FE_Assert(isGraphicsPass);

                FE_Assert(!Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kDepthTarget));
                const Core::PassDepthTarget depthTarget = elements.Get<Core::PassDepthTarget>(field, binding.m_data);
                if (!depthTarget.m_target.IsValid())
                    continue;
                FE_Assert(depthTarget.m_target.m_subresource.m_mipSliceCount == 1);
                FE_Assert(depthTarget.m_target.m_subresource.m_arraySize == 1);

                bool isDepthWriteEnabled = true;
                if (isGraphicsPipeline)
                {
                    const auto* graphicsPipeline = Rtti::AssertCast<const Core::GraphicsPipeline*>(pass.m_pipeline.Get());
                    isDepthWriteEnabled = graphicsPipeline->GetDesc().m_depthStencil.m_depthWriteEnabled;
                }

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
            else if (fieldType == Rtti::GetTypeID<Core::PassViewport>())
            {
                FE_Assert(elements.m_count == 1);
                FE_Assert(isGraphicsPass);

                FE_Assert(!Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kViewport));
                const Core::PassViewport viewport = elements.Get<Core::PassViewport>(field, binding.m_data);
                pass.m_viewport = viewport.m_rect;
                pass.m_specifiedStatesMask |= PassStateFlags::kViewport;
            }
            else if (fieldType == Rtti::GetTypeID<Core::PassScissor>())
            {
                FE_Assert(elements.m_count == 1);
                FE_Assert(isGraphicsPass);

                FE_Assert(!Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kScissor));
                const Core::PassScissor scissor = elements.Get<Core::PassScissor>(field, binding.m_data);
                pass.m_scissor = scissor.m_rect;
                pass.m_specifiedStatesMask |= PassStateFlags::kScissor;
            }
            else if (fieldType == Rtti::GetTypeID<Core::PassGraphicsPipeline>()
                     || fieldType == Rtti::GetTypeID<Core::PassComputePipeline>())
            {
                // Already processed.
            }
            else
            {
                // Other arbitrary types are interpreted as push constants.
                FE_Assert(elements.m_count == 1);
                SetPassPushConstants(pass, *elements.m_type, static_cast<const std::byte*>(binding.m_data) + field.m_offset);
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
        FE_PROFILER_ZONE_TEXT("%s", pass.m_name.c_str());

        const uint32_t passIndex = pass.m_passIndex;

        for (const BufferAccess& access : pass.m_accessedBuffers)
        {
            ResourceNode& resourceNode = m_resources[access.m_localResourceIndex];
            FE_AssertDebug(resourceNode.m_resource->GetType() == Core::ResourceType::kBuffer);

            auto* buffer = Rtti::AssertCast<Buffer*>(resourceNode.m_resource.Get());
            if (buffer->GetMemoryStatus() == Core::ResourceMemory::kNotCommitted)
            {
                resourceNode.m_isOwnedByGraph = true;

                Core::ResourceCommitParams commitParams;
                commitParams.m_bindFlags = resourceNode.m_bindFlags;
                commitParams.m_memory = Core::ResourceMemory::kDeviceLocal; // TODO: host visible resources
                commitParams.m_isTransient = true;
                m_resourcePool->CommitBufferMemory(buffer, commitParams);
            }

            FE_AssertDebug(Bit::AllSet(buffer->GetInstance()->m_bindFlags, resourceNode.m_bindFlags));

            SubresourceState state = buffer->GetState();
            const bool isReadToReadTransition = Core::IsReadAccess(state.m_access) && Core::IsReadAccess(access.m_accessFlags);
            const bool isCompatibleAccess = Bit::AllSet(state.m_access, access.m_accessFlags);
            const bool isCompatibleSync = Bit::AllSet(state.m_sync, access.m_syncFlags);

            if (state.m_queueType != Core::DeviceQueueType::kGraphics)
            {
                // For now, we assume that FrameGraph can only submit commands to the graphics queue.
                // So, the only possible transition is from AsyncCopyQueue (transfer queue).
                FE_AssertDebug(state.m_queueType == Core::DeviceQueueType::kTransfer);

                const auto transferBarrier = buffer->RetrieveQueueReleaseBarrier(Core::DeviceQueueType::kGraphics);
                FE_AssertDebug(transferBarrier.has_value(), "Cannot transfer buffer ownership without matching release barrier");

                Core::BufferBarrierDesc acquireBarrier = transferBarrier->m_barrier;
                acquireBarrier.m_syncBefore = Core::BarrierSyncFlags::kNone;
                acquireBarrier.m_accessBefore = Core::BarrierAccessFlags::kNone;
                acquireBarrier.m_syncAfter = access.m_syncFlags;
                acquireBarrier.m_accessAfter = access.m_accessFlags;
                pass.m_bufferOwnershipTransferBarriers.push_back(acquireBarrier);
                pass.m_ownershipTransferWaits.push_back(transferBarrier->m_completionFence);

                state.m_queueType = Core::DeviceQueueType::kGraphics;
                state.m_sync = access.m_syncFlags;
                state.m_access = access.m_accessFlags;
                buffer->SetState(state);

                resourceNode.m_previousBarrierPassIndex = passIndex;
                continue;
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
            //
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
                    FE_VerifyDebug(previousBarrierPass.m_barrierBatcher.AddBarrier(barrier));

                    state.m_access = state.m_access | access.m_accessFlags;
                    state.m_sync = state.m_sync | access.m_syncFlags;
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

            auto* texture = Rtti::AssertCast<Texture*>(resourceNode.m_resource.Get());
            if (texture->GetMemoryStatus() == Core::ResourceMemory::kNotCommitted)
            {
                resourceNode.m_isOwnedByGraph = true;

                Core::ResourceCommitParams commitParams;
                commitParams.m_bindFlags = resourceNode.m_bindFlags;
                commitParams.m_memory = Core::ResourceMemory::kDeviceLocal; // TODO: host visible resources
                commitParams.m_isTransient = true;
                m_resourcePool->CommitTextureMemory(texture, commitParams);
            }

            FE_AssertDebug(Bit::AllSet(texture->GetInstance()->m_bindFlags, resourceNode.m_bindFlags));

            SubresourceState state = texture->GetState(access.m_subresource);

            const bool isSameQueue = state.m_queueType == Core::DeviceQueueType::kGraphics;
            if (!isSameQueue)
            {
                FE_AssertDebug(state.m_queueType == Core::DeviceQueueType::kTransfer);

                // As per Vulkan spec, both acquire and release queue ownership transfer barriers must have
                // exactly the same subresource ranges.
                const auto transferBarrier =
                    texture->RetrieveQueueReleaseBarrier(Core::DeviceQueueType::kGraphics, access.m_subresource);
                FE_AssertDebug(transferBarrier.has_value(), "Cannot transfer texture ownership without matching release barrier");

                Core::TextureBarrierDesc acquireBarrier = transferBarrier->m_barrier;
                acquireBarrier.m_syncBefore = Core::BarrierSyncFlags::kNone;
                acquireBarrier.m_accessBefore = Core::BarrierAccessFlags::kNone;
                acquireBarrier.m_syncAfter = access.m_syncFlags;
                acquireBarrier.m_accessAfter = access.m_accessFlags;
                pass.m_textureOwnershipTransferBarriers.push_back(acquireBarrier);
                pass.m_ownershipTransferWaits.push_back(transferBarrier->m_completionFence);

                if (acquireBarrier.m_layoutAfter != access.m_layout)
                {
                    Core::TextureBarrierDesc layoutBarrier;
                    layoutBarrier.m_texture = texture;
                    layoutBarrier.m_subresource = access.m_subresource;
                    layoutBarrier.m_layoutBefore = acquireBarrier.m_layoutAfter;
                    layoutBarrier.m_layoutAfter = access.m_layout;
                    layoutBarrier.m_syncBefore = access.m_syncFlags;
                    layoutBarrier.m_syncAfter = access.m_syncFlags;
                    layoutBarrier.m_accessBefore = access.m_accessFlags;
                    layoutBarrier.m_accessAfter = access.m_accessFlags;
                    layoutBarrier.m_queueBefore = Core::DeviceQueueType::kGraphics;
                    layoutBarrier.m_queueAfter = Core::DeviceQueueType::kGraphics;
                    pass.m_texturePostOwnershipBarriers.push_back(layoutBarrier);
                }

                state.m_access = access.m_accessFlags;
                state.m_layout = access.m_layout;
                state.m_sync = access.m_syncFlags;
                state.m_queueType = Core::DeviceQueueType::kGraphics;
                texture->SetState(access.m_subresource, state);

                resourceNode.m_previousBarrierPassIndex = passIndex;
                continue;
            }

            const bool isReadToReadTransition = Core::IsReadAccess(state.m_access) && Core::IsReadAccess(access.m_accessFlags);
            const bool isCompatibleLayout = state.m_layout == access.m_layout;
            const bool isCompatibleSync = Bit::AllSet(state.m_sync, access.m_syncFlags);
            const bool isCompatibleAccess = Bit::AllSet(state.m_access, access.m_accessFlags);

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
                {
                    texture->SetState(access.m_subresource, state);
                    continue;
                }

                // An earlier reader's transition can cover this reader too; otherwise a read-to-read barrier follows below.
                if (isSameQueue && resourceNode.m_previousBarrierPassIndex != kInvalidIndex)
                {
                    ResourceBarrierBatcher& previousBatcher = m_passes[resourceNode.m_previousBarrierPassIndex].m_barrierBatcher;
                    if (previousBatcher.ExtendReadBarrier(barrier))
                    {
                        state.m_access = state.m_access | access.m_accessFlags;
                        state.m_layout = access.m_layout;
                        state.m_sync = state.m_sync | access.m_syncFlags;
                        texture->SetState(access.m_subresource, state);
                        continue;
                    }
                }
            }

            pass.m_barrierBatcher.AddBarrier(barrier);
            resourceNode.m_previousBarrierPassIndex = passIndex;

            state.m_access = access.m_accessFlags;
            state.m_layout = access.m_layout;
            state.m_sync = access.m_syncFlags;
            state.m_queueType = Core::DeviceQueueType::kGraphics;
            texture->SetState(access.m_subresource, state);
        }
    }


    void FrameGraph::Compile()
    {
        FE_PROFILER_ZONE();

        AddUploadReadAccesses();

        for (ResourceNode& resource : m_resources)
            AccumulateBindFlags(resource);

        for (PassNode& pass : m_passes)
            CompilePassBarriers(pass);

        for (Core::ResourceDescriptorInfo& descriptor : m_descriptorViews)
        {
            if (descriptor.m_resource->GetMemoryStatus() == Core::ResourceMemory::kNotCommitted)
            {
                descriptor = {};
                continue;
            }

            // Empty scenes can reserve a shader view for an upload that no pass consumes.
            if (const auto* buffer = Rtti::Cast<const Buffer*>(descriptor.m_resource.Get()))
            {
                const auto shaderFlags = Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite;
                if (!Bit::AnySet(buffer->GetInstance()->m_bindFlags, shaderFlags))
                    descriptor = {};
            }
        }
        m_descriptorManager->PrepareDescriptors(m_descriptorViews);
    }


    void FrameGraph::Execute()
    {
        FE_PROFILER_ZONE();

        PrepareExecuteInternal();

        // Replay scope boundaries at their original positions, including scopes without passes.
        uint32_t scopeEventIndex = 0;
        const auto executeScopeEvents = [&](const uint32_t passIndex) {
            while (scopeEventIndex < m_scopeEvents.size())
            {
                const ScopeEvent& event = m_scopeEvents[scopeEventIndex];
                if (event.m_passIndex != passIndex)
                    break;

                if (event.m_isBegin)
                    BeginMarkerInternal(event.m_name);
                else
                    EndMarkerInternal();
                ++scopeEventIndex;
            }
        };

        for (PassNode& pass : m_passes)
        {
            FE_PROFILER_ZONE_TEXT("%s", pass.m_name.c_str());

            executeScopeEvents(pass.m_passIndex);
            BeginMarkerInternal(pass.m_name);
            ExecutePassBarriersInternal(pass);

            if (Bit::AllSet(pass.m_specifiedStatesMask, PassStateFlags::kPushConstants))
            {
                const uint32_t shaderByteSize = pass.m_pipeline->GetRootConstantsByteSize();
                FE_Assert(shaderByteSize == 0 || shaderByteSize == pass.m_pushConstants.size_bytes(),
                          "Push constants do not match the shader");
                m_currentContext->PushConstants(pass.m_pushConstants.data(), pass.m_pushConstants.size_bytes());
            }

            if (Bit::AnySet(pass.m_specifiedStatesMask, PassStateFlags::kRenderTargets))
            {
                festd::fixed_vector<Core::TextureView, Core::Limits::Pipeline::kMaxColorAttachments> renderTargets;
                for (const uint32_t colorTargetIndex : pass.m_colorTargetAccessIndices)
                {
                    const TextureAccess& access = pass.m_accessedTextures[colorTargetIndex];
                    const ResourceNode& resourceNode = m_resources[access.m_localResourceIndex];
                    auto* texture = Rtti::AssertCast<Core::Texture*>(resourceNode.m_resource.Get());
                    renderTargets.push_back(Core::TextureView::Create(texture, access.m_subresource));
                }

                Core::TextureView depthStencil = Core::TextureView::kInvalid;
                if (Bit::AllSet(pass.m_specifiedStatesMask, PassStateFlags::kDepthTarget))
                {
                    const TextureAccess& access = pass.m_accessedTextures[pass.m_depthTargetAccessIndex];
                    const ResourceNode& resourceNode = m_resources[access.m_localResourceIndex];
                    auto* texture = Rtti::AssertCast<Core::Texture*>(resourceNode.m_resource.Get());
                    depthStencil = Core::TextureView::Create(texture, access.m_subresource);
                }

                m_currentContext->SetRenderTargets(renderTargets, depthStencil);
            }

            if (Bit::AllSet(pass.m_specifiedStatesMask, PassStateFlags::kViewport))
                m_currentContext->SetViewport(pass.m_viewport);

            if (Bit::AllSet(pass.m_specifiedStatesMask, PassStateFlags::kScissor))
                m_currentContext->SetScissor(pass.m_scissor);

            if (Bit::AllSet(pass.m_specifiedStatesMask, PassStateFlags::kIndirectArgs))
                m_currentContext->SetIndirectArgs(pass.m_indirectArgs);

            if (Bit::AllSet(pass.m_specifiedStatesMask, PassStateFlags::kGraphicsPipeline))
                m_currentContext->SetPipeline(Rtti::AssertCast<const Core::GraphicsPipeline*>(pass.m_pipeline.Get()));

            if (Bit::AllSet(pass.m_specifiedStatesMask, PassStateFlags::kComputePipeline))
                m_currentContext->SetPipeline(Rtti::AssertCast<const Core::ComputePipeline*>(pass.m_pipeline.Get()));

            if (pass.m_functor)
                pass.m_execute(pass.m_functor, *m_currentContext);

            FE_Assert(m_currentContext->IsCleanState());
            EndMarkerInternal();

            if (pass.m_functor)
                pass.m_destroy(pass.m_functor, pass.m_userPassDescPtr);
        }

        executeScopeEvents(m_passes.size());
        FinishExecuteInternal();
    }


    uint32_t FrameGraph::RegisterResource(Core::Resource* resource, PassNode& pass, const TextureAccess& access)
    {
        const uint32_t resourceID = resource->GetResourceID();
        const auto [it, inserted] = m_resourceIndexMap.insert({ resourceID, m_resources.size() });
        if (inserted)
        {
            ResourceNode& newResourceNode = m_resources.emplace_back(GetAllocator());
            newResourceNode.m_resource = resource;
        }

        const uint32_t localResourceIndex = it->second;
        ResourceNode& resourceNode = m_resources[localResourceIndex];
        FE_Assert(resourceNode.m_resource == resource);

        const Core::TextureSubresourceIterator subresourceIterator(access.m_subresource);
        for (const auto [mipIndex, arrayIndex] : subresourceIterator)
        {
            // The iterator yields absolute indices; slices are relative to the accessed view, e.g. a single mip of a pyramid.
            const uint32_t mipOffset = static_cast<uint32_t>(mipIndex) - access.m_subresource.m_mostDetailedMipSlice;
            const uint32_t arrayOffset = static_cast<uint32_t>(arrayIndex) - access.m_subresource.m_firstArraySlice;
            const Core::TextureSubresource subresource = access.m_subresource.Slice(mipOffset, arrayOffset);
            const auto existing = festd::find_if(pass.m_accessedTextures, [&](const TextureAccess& other) {
                return other.m_localResourceIndex == localResourceIndex && other.m_subresource == subresource;
            });
            if (existing != pass.m_accessedTextures.end())
            {
                FE_Assert(existing->m_layout == access.m_layout, "Incompatible texture views in the same pass");
                FE_Assert(access.m_layout == Core::BarrierLayout::kShaderRead
                              || access.m_layout == Core::BarrierLayout::kShaderReadWrite,
                          "Duplicate attachment access");
                existing->m_syncFlags |= access.m_syncFlags;
                existing->m_accessFlags |= access.m_accessFlags;
                continue;
            }

            const uint32_t passAccessIndex = pass.m_accessedTextures.size();
            resourceNode.m_accesses.push_back({ pass.m_passIndex, passAccessIndex });

            TextureAccess& subresourceAccess = pass.m_accessedTextures.emplace_back(access);
            subresourceAccess.m_localResourceIndex = localResourceIndex;
            subresourceAccess.m_subresource = subresource;
        }

        return localResourceIndex;
    }


    uint32_t FrameGraph::RegisterResource(Core::Resource* resource, PassNode& pass, const BufferAccess& access)
    {
        const uint32_t resourceID = resource->GetResourceID();
        const auto [it, inserted] = m_resourceIndexMap.insert({ resourceID, m_resources.size() });
        if (inserted)
        {
            ResourceNode& newResourceNode = m_resources.emplace_back(GetAllocator());
            newResourceNode.m_resource = resource;
        }

        const uint32_t localResourceIndex = it->second;
        ResourceNode& resourceNode = m_resources[localResourceIndex];
        FE_Assert(resourceNode.m_resource == resource);

        for (BufferAccess& other : pass.m_accessedBuffers)
        {
            if (other.m_localResourceIndex != localResourceIndex)
                continue;

            other.m_syncFlags |= access.m_syncFlags;
            other.m_accessFlags |= access.m_accessFlags;
            return localResourceIndex;
        }

        const uint32_t passAccessIndex = pass.m_accessedBuffers.size();
        resourceNode.m_accesses.push_back({ pass.m_passIndex, passAccessIndex });

        BufferAccess& bufferAccess = pass.m_accessedBuffers.emplace_back(access);
        bufferAccess.m_localResourceIndex = localResourceIndex;

        return localResourceIndex;
    }
} // namespace FE::Graphics::Common
