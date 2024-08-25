#include <FeCore/Console/FeLog.h>
#include <HAL/Vulkan/ShaderReflection.h>

namespace FE::Graphics::Vulkan
{
    namespace SpvC = spirv_cross;


    inline static HAL::Format SPIRTypeToFormat(const SpvC::SPIRType& type)
    {
        using HAL::Format;

        switch (type.basetype)
        {
        case SpvC::SPIRType::BaseType::Half:
            switch (type.vecsize)
            {
            case 1:
                return Format::R16_SFloat;
            case 2:
                return Format::R16G16_SFloat;
            case 3:
                return Format::R16G16B16_SFloat;
            case 4:
                return Format::R16G16B16A16_SFloat;
            default:
                FE_UNREACHABLE("VecSize {} in parameters is not allowed", type.vecsize);
                return Format::None;
            }
        case SpvC::SPIRType::BaseType::Float:
            switch (type.vecsize)
            {
            case 1:
                return Format::R32_SFloat;
            case 2:
                return Format::R32G32_SFloat;
            case 3:
                return Format::R32G32B32_SFloat;
            case 4:
                return Format::R32G32B32A32_SFloat;
            default:
                FE_UNREACHABLE("VecSize {} in parameters is not allowed", type.vecsize);
                return Format::None;
            }
        case SpvC::SPIRType::BaseType::Int:
            switch (type.vecsize)
            {
            case 1:
                return Format::R32_SInt;
            case 2:
                return Format::R32G32_SInt;
            case 3:
                return Format::R32G32B32_SInt;
            case 4:
                return Format::R32G32B32A32_SInt;
            default:
                FE_UNREACHABLE("VecSize {} in parameters is not allowed", type.vecsize);
                return Format::None;
            }
        case SpvC::SPIRType::BaseType::UInt:
            switch (type.vecsize)
            {
            case 1:
                return Format::R32_UInt;
            case 2:
                return Format::R32G32_UInt;
            case 3:
                return Format::R32G32B32_UInt;
            case 4:
                return Format::R32G32B32A32_UInt;
            default:
                FE_UNREACHABLE("VecSize {} in parameters is not allowed", type.vecsize);
                return Format::None;
            }
        default:
            FE_UNREACHABLE("Invalid format");
            return Format::None;
        }
    }


    inline static HAL::ShaderResourceType ConvertResourceType(const SpvC::CompilerHLSL* pCompiler,
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
                    return HAL::ShaderResourceType::BufferSRV;
                else
                    return HAL::ShaderResourceType::BufferUAV;
            }
            else
            {
                if (type.image.sampled != 2)
                    return HAL::ShaderResourceType::TextureSRV;
                else
                    return HAL::ShaderResourceType::TextureUAV;
            }
        case SpvC::SPIRType::Sampler:
            return HAL::ShaderResourceType::Sampler;
        case SpvC::SPIRType::Struct:
            if (type.storage == spv::StorageClassStorageBuffer)
            {
                const SpvC::Bitset flags = pCompiler->get_buffer_block_flags(pResource->id);
                const bool readonly = flags.get(spv::DecorationNonWritable);
                if (readonly)
                    return HAL::ShaderResourceType::TextureSRV;
                else
                    return HAL::ShaderResourceType::TextureUAV;
            }
            else if (type.storage == spv::StorageClassPushConstant || type.storage == spv::StorageClassUniform)
            {
                return HAL::ShaderResourceType::ConstantBuffer;
            }

            FE_UNREACHABLE("Invalid resource type");
            return HAL::ShaderResourceType::None;
        default:
            FE_UNREACHABLE("Invalid resource type");
            return HAL::ShaderResourceType::None;
        }
    }


    inline static HAL::ShaderResourceBinding ConvertBinding(const SpvC::CompilerHLSL* pCompiler, const SpvC::Resource* pResource)
    {
        const SpvC::SPIRType& resourceType = pCompiler->get_type(pResource->type_id);

        HAL::ShaderResourceBinding result;
        result.Name = StringSlice(pCompiler->get_name(pResource->id));
        result.Type = ConvertResourceType(pCompiler, pResource);
        result.Slot = static_cast<uint8_t>(pCompiler->get_decoration(pResource->id, spv::DecorationBinding));
        result.Space = static_cast<uint8_t>(pCompiler->get_decoration(pResource->id, spv::DecorationDescriptorSet));
        result.Count = 1;

        if (!resourceType.array.empty())
            result.Count = static_cast<uint8_t>(resourceType.array.front());

        FE_ASSERT(result.Count != 0);
        return result;
    }


    ShaderReflection::ShaderReflection(festd::span<uint32_t> byteCode)
    {
        const auto compiler = festd::make_unique<SpvC::CompilerHLSL>(byteCode.data(), byteCode.size());
        const auto resources = compiler->get_shader_resources();
        ParseInputAttributes(compiler.get(), resources);
        ParseResourceBindings(compiler.get(), resources);
    }


    festd::span<const HAL::ShaderInputAttribute> ShaderReflection::GetInputAttributes() const
    {
        return m_InputAttributes;
    }


    festd::span<const HAL::ShaderResourceBinding> ShaderReflection::GetResourceBindings() const
    {
        return m_ResourceBindings;
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

            auto& current = m_InputAttributes.emplace_back();
            current.Location = pCompiler->get_decoration(resource.id, spv::DecorationLocation);
            current.ShaderSemantic = StringSlice(semantic.c_str(), static_cast<uint32_t>(semanticSize));
            current.ElementFormat = SPIRTypeToFormat(pCompiler->get_type(resource.base_type_id));
        }
    }


    void ShaderReflection::ParseResourceBindings(const SpvC::CompilerHLSL* pCompiler,
                                                 const SpvC::ShaderResources& shaderResources)
    {
        const auto parseList = [&](const SpvC::SmallVector<SpvC::Resource>& resources) {
            for (const SpvC::Resource& resource : resources)
            {
                m_ResourceBindings.push_back(ConvertBinding(pCompiler, &resource));
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


    uint32_t ShaderReflection::GetResourceBindingIndex(StringSlice name) const
    {
        for (const HAL::ShaderResourceBinding& binding : m_ResourceBindings)
        {
            if (binding.Name == name)
            {
                return binding.Slot;
            }
        }

        FE_UNREACHABLE("Shader resource \"{}\" not found", name);
        return 0;
    }


    uint32_t ShaderReflection::GetInputAttributeLocation(StringSlice semantic) const
    {
        for (const HAL::ShaderInputAttribute& input : m_InputAttributes)
        {
            if (input.ShaderSemantic == semantic)
            {
                return input.Location;
            }
        }

        FE_UNREACHABLE("Shader semantic \"{}\" not found in input attributes", semantic);
        return 0;
    }
} // namespace FE::Graphics::Vulkan
