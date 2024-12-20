﻿#include <FeCore/Logging/Trace.h>
#include <Graphics/RHI/Vulkan/ShaderReflection.h>

namespace FE::Graphics::Vulkan
{
    namespace SpvC = spirv_cross;


    inline static RHI::Format SPIRTypeToFormat(const SpvC::SPIRType& type)
    {
        using RHI::Format;

        switch (type.basetype)
        {
        case SpvC::SPIRType::BaseType::Half:
            FE_AssertMsg(false, "Float16 is not supported");
            return Format::kUndefined;

        case SpvC::SPIRType::BaseType::Float:
            switch (type.vecsize)
            {
            case 1:
                return Format::kR32_SFLOAT;
            case 2:
                return Format::kR32G32_SFLOAT;
            case 3:
                return Format::kR32G32B32_SFLOAT;
            case 4:
                return Format::kR32G32B32A32_SFLOAT;
            default:
                FE_AssertMsg(false, "VecSize {} in parameters is not allowed", type.vecsize);
                return Format::kUndefined;
            }

        case SpvC::SPIRType::BaseType::Int:
            switch (type.vecsize)
            {
            case 1:
                return Format::kR32_SINT;
            case 2:
                return Format::kR32G32_SINT;
            case 3:
                return Format::kR32G32B32_SINT;
            case 4:
                return Format::kR32G32B32A32_SINT;
            default:
                FE_AssertMsg(false, "VecSize {} in parameters is not allowed", type.vecsize);
                return Format::kUndefined;
            }

        case SpvC::SPIRType::BaseType::UInt:
            switch (type.vecsize)
            {
            case 1:
                return Format::kR32_UINT;
            case 2:
                return Format::kR32G32_UINT;
            case 3:
                return Format::kR32G32B32_UINT;
            case 4:
                return Format::kR32G32B32A32_UINT;
            default:
                FE_AssertMsg(false, "VecSize {} in parameters is not allowed", type.vecsize);
                return Format::kUndefined;
            }

        default:
            FE_AssertMsg(false, "Invalid format");
            return Format::kUndefined;
        }
    }


    inline static RHI::ShaderResourceType ConvertResourceType(const SpvC::CompilerHLSL* pCompiler,
                                                              const SpvC::Resource* pResource)
    {
        const SpvC::SPIRType& type = pCompiler->get_type(pResource->type_id);

        switch (type.basetype)
        {
        case SpvC::SPIRType::SampledImage:
        case SpvC::SPIRType::Image:
            if (type.image.dim == spv::Dim::DimBuffer)
            {
                if (type.image.sampled != 2)
                    return RHI::ShaderResourceType::kBufferSRV;
                else
                    return RHI::ShaderResourceType::kBufferUAV;
            }
            else
            {
                if (type.image.sampled != 2)
                    return RHI::ShaderResourceType::kTextureSRV;
                else
                    return RHI::ShaderResourceType::kTextureUAV;
            }

        case SpvC::SPIRType::Sampler:
            return RHI::ShaderResourceType::kSampler;

        case SpvC::SPIRType::Struct:
            if (type.storage == spv::StorageClassStorageBuffer)
            {
                const SpvC::Bitset flags = pCompiler->get_buffer_block_flags(pResource->id);
                const bool readonly = flags.get(spv::DecorationNonWritable);
                if (readonly)
                    return RHI::ShaderResourceType::kTextureSRV;
                else
                    return RHI::ShaderResourceType::kTextureUAV;
            }
            else if (type.storage == spv::StorageClassPushConstant || type.storage == spv::StorageClassUniform)
            {
                return RHI::ShaderResourceType::kConstantBuffer;
            }

            FE_AssertMsg(false, "Invalid resource type");
            return RHI::ShaderResourceType::kNone;

        default:
            FE_AssertMsg(false, "Invalid resource type");
            return RHI::ShaderResourceType::kNone;
        }
    }


    inline static RHI::ShaderResourceBinding ConvertBinding(const SpvC::CompilerHLSL* pCompiler, const SpvC::Resource* pResource)
    {
        const SpvC::SPIRType& resourceType = pCompiler->get_type(pResource->type_id);

        RHI::ShaderResourceBinding result;
        result.m_name = Env::Name(pCompiler->get_name(pResource->id));
        result.m_type = ConvertResourceType(pCompiler, pResource);
        result.m_slot = static_cast<uint8_t>(pCompiler->get_decoration(pResource->id, spv::DecorationBinding));
        result.m_space = static_cast<uint8_t>(pCompiler->get_decoration(pResource->id, spv::DecorationDescriptorSet));
        result.m_count = 1;

        if (!resourceType.array.empty())
            result.m_count = static_cast<uint8_t>(resourceType.array.front());

        FE_Assert(result.m_count != 0);
        return result;
    }


    ShaderReflection::ShaderReflection(festd::span<uint32_t> byteCode)
    {
        const auto compiler = festd::make_unique<SpvC::CompilerHLSL>(byteCode.data(), byteCode.size());
        const auto resources = compiler->get_shader_resources();
        ParseInputAttributes(compiler.get(), resources);
        ParseResourceBindings(compiler.get(), resources);
    }


    festd::span<const RHI::ShaderInputAttribute> ShaderReflection::GetInputAttributes() const
    {
        return m_inputAttributes;
    }


    festd::span<const RHI::ShaderResourceBinding> ShaderReflection::GetResourceBindings() const
    {
        return m_resourceBindings;
    }


    void ShaderReflection::ParseInputAttributes(const SpvC::CompilerHLSL* pCompiler, const SpvC::ShaderResources& shaderResources)
    {
        for (const auto& resource : shaderResources.stage_inputs)
        {
            const auto& semantic = pCompiler->get_decoration_string(resource.id, spv::DecorationHlslSemanticGOOGLE);
            auto semanticSize = semantic.length();
            if (!semantic.empty() && semantic.back() == '0')
            {
                --semanticSize;
            }

            auto& current = m_inputAttributes.emplace_back();
            current.m_location = pCompiler->get_decoration(resource.id, spv::DecorationLocation);
            current.m_shaderSemantic = Env::Name(semantic.c_str(), static_cast<uint32_t>(semanticSize));
            current.m_elementFormat = SPIRTypeToFormat(pCompiler->get_type(resource.base_type_id));
        }
    }


    void ShaderReflection::ParseResourceBindings(const SpvC::CompilerHLSL* pCompiler,
                                                 const SpvC::ShaderResources& shaderResources)
    {
        const auto parseList = [&](const SpvC::SmallVector<SpvC::Resource>& resources) {
            for (const SpvC::Resource& resource : resources)
            {
                m_resourceBindings.push_back(ConvertBinding(pCompiler, &resource));
            }
        };

        parseList(shaderResources.uniform_buffers);
        parseList(shaderResources.storage_buffers);
        parseList(shaderResources.storage_images);
        parseList(shaderResources.separate_images);
        parseList(shaderResources.separate_samplers);
        parseList(shaderResources.atomic_counters);
        parseList(shaderResources.acceleration_structures);
    }


    uint32_t ShaderReflection::GetResourceBindingIndex(Env::Name name) const
    {
        for (const RHI::ShaderResourceBinding& binding : m_resourceBindings)
        {
            if (binding.m_name == name)
                return binding.m_slot;
        }

        FE_AssertMsg(false, "Shader resource \"{}\" not found", name);
        return 0;
    }


    uint32_t ShaderReflection::GetInputAttributeLocation(Env::Name semantic) const
    {
        for (const RHI::ShaderInputAttribute& input : m_inputAttributes)
        {
            if (input.m_shaderSemantic == semantic)
                return input.m_location;
        }

        FE_AssertMsg(false, "Shader semantic \"{}\" not found in input attributes", semantic);
        return 0;
    }
} // namespace FE::Graphics::Vulkan
