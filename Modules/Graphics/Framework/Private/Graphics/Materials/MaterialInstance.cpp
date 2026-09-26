#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/ShaderReflection.h>
#include <Graphics/Materials/MaterialInstance.h>

namespace FE::Graphics
{
    namespace
    {
        bool LayoutsMatch(const Core::ShaderStructLayout& lhs, const Core::ShaderStructLayout& rhs)
        {
            if (lhs.m_byteSize != rhs.m_byteSize || lhs.m_members.size() != rhs.m_members.size())
                return false;

            for (uint32_t index = 0; index < lhs.m_members.size(); ++index)
            {
                const Core::ShaderStructMember& a = lhs.m_members[index];
                const Core::ShaderStructMember& b = rhs.m_members[index];
                if (a != b)
                    return false;
            }

            return true;
        }
    } // namespace


    MaterialInstanceRuntime::MaterialInstanceRuntime(const MaterialAsset* material, const MaterialInstanceAsset* instance,
                                                     MaterialParameterAllocator* allocator,
                                                     Core::PipelineFactory* pipelineFactory)
        : m_material(material)
        , m_instance(instance)
        , m_allocator(allocator)
        , m_pipelineFactory(pipelineFactory)
    {
        FE_Assert(material && instance && allocator && pipelineFactory);
        for (const MaterialTechniqueDesc& technique : m_material->m_techniques)
        {
            if (technique.m_permutationKey != m_instance->m_permutationKey || !technique.m_pixelShader.IsValid())
                continue;

            const Env::Name defines =
                technique.m_shaderDefines.empty() ? Env::Name::kEmpty : Env::Name(technique.m_shaderDefines);
            const Core::ShaderReflection* reflection =
                m_pipelineFactory->GetShaderLibrary()->GetCompiledReflection(technique.m_pixelShader, defines);
            FE_Assert(reflection != nullptr, "Failed to compile material shader");

            for (const Core::ShaderStructLayout& layout : reflection->GetStructLayouts())
            {
                if (layout.m_name == Env::Name("MaterialParameters"))
                {
                    FE_Assert(m_materialLayout.m_byteSize == 0 || LayoutsMatch(m_materialLayout, layout),
                              "Material parameter layouts differ between techniques");
                    m_materialLayout = layout;
                }
                else if (layout.m_name == Env::Name("InstanceParameters"))
                {
                    FE_Assert(m_instanceLayout.m_byteSize == 0 || LayoutsMatch(m_instanceLayout, layout),
                              "Instance parameter layouts differ between techniques");
                    m_instanceLayout = layout;
                }
            }
        }

        m_materialBytes = PackParameters(MaterialParameterScope::kMaterial);
        m_instanceBytes = PackParameters(MaterialParameterScope::kInstance);
        m_materialAllocation = m_allocator->Allocate(m_materialBytes.size());
        m_allocator->Write(m_materialAllocation, m_materialBytes.data(), m_materialBytes.size());
        m_allocator->Register(this);
    }


    MaterialInstanceRuntime::~MaterialInstanceRuntime()
    {
        m_allocator->Unregister(this);
        for (const MaterialParameterAllocator::Allocation allocation : m_instanceAllocations)
            m_allocator->Free(allocation);
        if (m_materialAllocation.IsValid())
            m_allocator->Free(m_materialAllocation);
    }


    const MaterialParameterValue* MaterialInstanceRuntime::FindValue(const Env::Name name) const
    {
        for (const MaterialParameterValue& value : m_instance->m_parameters)
        {
            if (value.m_name == name)
                return &value;
        }
        return nullptr;
    }


    const Core::ShaderStructLayout& MaterialInstanceRuntime::GetLayout(const MaterialParameterScope scope) const
    {
        return scope == MaterialParameterScope::kMaterial ? m_materialLayout : m_instanceLayout;
    }


    festd::vector<std::byte> MaterialInstanceRuntime::PackParameters(const MaterialParameterScope scope) const
    {
        const Core::ShaderStructLayout& layout = GetLayout(scope);
        festd::vector<std::byte> bytes(Math::Max(layout.m_byteSize, 16u));
        memset(bytes.data(), 0, bytes.size());

        for (const MaterialParameterDesc& parameter : m_material->m_parameters)
        {
            if (parameter.m_scope != scope)
                continue;

            const Core::ShaderStructMember* member = nullptr;
            for (const Core::ShaderStructMember& candidate : layout.m_members)
            {
                if (candidate.m_name == parameter.m_name)
                {
                    member = &candidate;
                    break;
                }
            }

            FE_Assert(member != nullptr, "Material parameter missing in shader reflection");

            const uint32_t offset = member->m_offset;
            FE_Assert(offset + member->m_byteSize <= bytes.size());

            const MaterialParameterValue* value = FindValue(parameter.m_name);
            if (parameter.m_type == MaterialParameterType::kTexture2D)
            {
                FE_Assert(member->m_type == Core::ShaderStructMemberType::kTexture2DDescriptor);

                uint32_t textureIndex = kInvalidIndex;
                if (value != nullptr && value->m_texture.GetAssetID().IsValid())
                {
                    const IO::AssetRead<TextureAsset> texture = value->m_texture.GetAssetHandle().Read();
                    FE_Assert(texture && texture->m_texture);
                    textureIndex = texture->m_descriptorIndex;
                    FE_Assert(textureIndex != kInvalidIndex);
                }

                memcpy(bytes.data() + offset, &textureIndex, sizeof(textureIndex));
            }
            else
            {
                const Vector4 parameterValue = value != nullptr ? value->m_value : parameter.m_defaultValue;
                memcpy(bytes.data() + offset, &parameterValue, Math::Min<uint32_t>(member->m_byteSize, sizeof(parameterValue)));
            }
        }

        return bytes;
    }


    BufferPointer MaterialInstanceRuntime::GetMaterialParameters() const
    {
        return m_materialAllocation.m_devicePointer;
    }


    MaterialParameterAllocator::Allocation MaterialInstanceRuntime::AllocateInstanceParameters()
    {
        MaterialParameterAllocator::Allocation allocation = m_allocator->Allocate(m_instanceBytes.size());
        m_allocator->Write(allocation, m_instanceBytes.data(), m_instanceBytes.size());
        m_instanceAllocations.push_back(allocation);
        return allocation;
    }


    void MaterialInstanceRuntime::RefreshDescriptors()
    {
        const festd::vector<std::byte> materialBytes = PackParameters(MaterialParameterScope::kMaterial);
        if (materialBytes.size() != m_materialBytes.size()
            || memcmp(materialBytes.data(), m_materialBytes.data(), materialBytes.size()) != 0)
        {
            m_materialBytes = materialBytes;
            m_allocator->Write(m_materialAllocation, m_materialBytes.data(), m_materialBytes.size());
        }

        const festd::vector<std::byte> instanceBytes = PackParameters(MaterialParameterScope::kInstance);
        if (instanceBytes.size() != m_instanceBytes.size()
            || memcmp(instanceBytes.data(), m_instanceBytes.data(), instanceBytes.size()) != 0)
        {
            m_instanceBytes = instanceBytes;
            for (const MaterialParameterAllocator::Allocation allocation : m_instanceAllocations)
                m_allocator->Write(allocation, m_instanceBytes.data(), m_instanceBytes.size());
        }
    }


    Core::GraphicsPipeline* MaterialInstanceRuntime::GetPipeline(const Env::Name role, const Core::Format colorFormat)
    {
        for (const PipelineEntry& entry : m_pipelines)
        {
            if (entry.m_role == role && entry.m_colorFormat == colorFormat)
                return entry.m_pipeline;
        }

        const MaterialTechniqueDesc* technique = nullptr;
        for (const MaterialTechniqueDesc& candidate : m_material->m_techniques)
        {
            if (candidate.m_role == role && candidate.m_permutationKey == m_instance->m_permutationKey)
            {
                technique = &candidate;
                break;
            }
        }

        if (technique == nullptr)
        {
            Logger::LogError("Material '{}' lacks technique '{}' for permutation '{}'",
                             m_material->m_name,
                             role,
                             m_instance->m_permutationKey);
        }

        FE_Assert(technique != nullptr, "Missing material technique");

        Core::GraphicsPipelineRequest request;
        if (technique->m_vertexShader.IsValid())
            request.m_desc.SetVertexShader(technique->m_vertexShader);
        if (technique->m_amplificationShader.IsValid())
            request.m_desc.SetAmplificationShader(technique->m_amplificationShader);
        if (technique->m_meshShader.IsValid())
            request.m_desc.SetMeshShader(technique->m_meshShader);
        if (technique->m_pixelShader.IsValid())
            request.m_desc.SetPixelShader(technique->m_pixelShader);
        request.m_desc.SetDSVFormat(Core::Format::kD32_SFLOAT_S8_UINT)
            .SetDepthStencil(technique->m_depthStencil)
            .SetRasterization(technique->m_rasterization);

        if (role != "DepthOnly")
            request.m_desc.SetRTVFormat(colorFormat).SetColorBlend(technique->m_blend);
        request.m_defines = technique->m_shaderDefines.empty() ? Env::Name::kEmpty : Env::Name(technique->m_shaderDefines);

        Core::GraphicsPipeline* pipeline = m_pipelineFactory->CreateGraphicsPipeline(request);
        pipeline->GetCompletionWaitGroup()->Wait();
        FE_Assert(pipeline->IsReady(), "Failed to compile material technique");
        PipelineEntry& entry = m_pipelines.emplace_back();
        entry.m_role = role;
        entry.m_colorFormat = colorFormat;
        entry.m_pipeline = pipeline;
        return pipeline;
    }
} // namespace FE::Graphics
