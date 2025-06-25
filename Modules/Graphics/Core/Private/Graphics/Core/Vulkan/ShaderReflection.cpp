#include <FeCore/Logging/Trace.h>
#include <Graphics/Core/Vulkan/ShaderReflection.h>

namespace FE::Graphics::Vulkan
{
    namespace SpvC = spirv_cross;


    namespace
    {
        Core::Format SPIRTypeToFormat(const SpvC::SPIRType& type)
        {
            using Core::Format;

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


        Core::ShaderResourceType ConvertResourceType(const SpvC::CompilerHLSL* compiler, const SpvC::Resource* pResource)
        {
            const SpvC::SPIRType& type = compiler->get_type(pResource->type_id);

            switch (type.basetype)
            {
            case SpvC::SPIRType::SampledImage:
            case SpvC::SPIRType::Image:
                if (type.image.dim == spv::Dim::DimBuffer)
                {
                    if (type.image.sampled != 2)
                        return Core::ShaderResourceType::kBufferSRV;
                    else
                        return Core::ShaderResourceType::kBufferUAV;
                }
                else
                {
                    if (type.image.sampled != 2)
                        return Core::ShaderResourceType::kTextureSRV;
                    else
                        return Core::ShaderResourceType::kTextureUAV;
                }

            case SpvC::SPIRType::Sampler:
                return Core::ShaderResourceType::kSampler;

            case SpvC::SPIRType::Struct:
                if (type.storage == spv::StorageClassStorageBuffer)
                {
                    const SpvC::Bitset flags = compiler->get_buffer_block_flags(pResource->id);
                    const bool readonly = flags.get(spv::DecorationNonWritable);
                    if (readonly)
                        return Core::ShaderResourceType::kTextureSRV;
                    else
                        return Core::ShaderResourceType::kTextureUAV;
                }
                else if (type.storage == spv::StorageClassPushConstant || type.storage == spv::StorageClassUniform)
                {
                    return Core::ShaderResourceType::kConstantBuffer;
                }

                FE_AssertMsg(false, "Invalid resource type");
                return Core::ShaderResourceType::kNone;

            default:
                FE_AssertMsg(false, "Invalid resource type");
                return Core::ShaderResourceType::kNone;
            }
        }


        Core::ShaderResourceBinding ConvertBinding(const SpvC::CompilerHLSL* compiler, const SpvC::Resource* pResource)
        {
            const SpvC::SPIRType& resourceType = compiler->get_type(pResource->type_id);

            Core::ShaderResourceBinding result;
            result.m_name = Env::Name(compiler->get_name(pResource->id));
            result.m_type = ConvertResourceType(compiler, pResource);
            result.m_stride = 0;
            result.m_slot = static_cast<uint8_t>(compiler->get_decoration(pResource->id, spv::DecorationBinding));
            result.m_space = static_cast<uint8_t>(compiler->get_decoration(pResource->id, spv::DecorationDescriptorSet));
            result.m_count = 1;

            if (!resourceType.array.empty())
                result.m_count = static_cast<uint8_t>(resourceType.array.front());

            return result;
        }
    } // namespace


    ShaderReflection::ShaderReflection(const festd::span<const uint32_t> byteCode)
    {
        FE_PROFILER_ZONE();

        const auto compiler = festd::make_unique<SpvC::CompilerHLSL>(byteCode.data(), byteCode.size());
        const auto resources = compiler->get_shader_resources();
        ParseInputAttributes(compiler.get(), resources);
        ParseResourceBindings(compiler.get(), resources);
    }


    festd::span<const Core::ShaderInputAttribute> ShaderReflection::GetInputAttributes() const
    {
        return m_inputAttributes;
    }


    festd::span<const Core::ShaderResourceBinding> ShaderReflection::GetResourceBindings() const
    {
        return m_resourceBindings;
    }


    festd::span<const Core::ShaderRootConstant> ShaderReflection::GetRootConstants() const
    {
        return m_rootConstants;
    }


    festd::span<const Env::Name> ShaderReflection::GetSpecializationConstantNames() const
    {
        return m_specializationConstantNames;
    }


    void ShaderReflection::ParseInputAttributes(const SpvC::CompilerHLSL* compiler, const SpvC::ShaderResources& shaderResources)
    {
        FE_PROFILER_ZONE();

        for (const auto& resource : shaderResources.stage_inputs)
        {
            const auto& semantic = compiler->get_decoration_string(resource.id, spv::DecorationHlslSemanticGOOGLE);
            auto semanticSize = semantic.length();
            if (!semantic.empty() && semantic.back() == '0')
            {
                --semanticSize;
            }

            auto& current = m_inputAttributes.emplace_back();
            current.m_location = compiler->get_decoration(resource.id, spv::DecorationLocation);
            current.m_shaderSemantic = Env::Name(semantic.c_str(), static_cast<uint32_t>(semanticSize));
            current.m_elementFormat = SPIRTypeToFormat(compiler->get_type(resource.base_type_id));
        }
    }


    void ShaderReflection::ParseResourceBindings(const SpvC::CompilerHLSL* compiler, const SpvC::ShaderResources& shaderResources)
    {
        FE_PROFILER_ZONE();

        const auto parseList = [&](const SpvC::SmallVector<SpvC::Resource>& resources) {
            for (const SpvC::Resource& resource : resources)
            {
                m_resourceBindings.push_back(ConvertBinding(compiler, &resource));
            }
        };

        parseList(shaderResources.uniform_buffers);
        parseList(shaderResources.storage_buffers);
        parseList(shaderResources.storage_images);
        parseList(shaderResources.separate_images);
        parseList(shaderResources.separate_samplers);
        parseList(shaderResources.atomic_counters);
        parseList(shaderResources.acceleration_structures);

        for (const SpvC::Resource& resource : shaderResources.push_constant_buffers)
        {
            Core::ShaderRootConstant rootConstant;
            rootConstant.m_name = Env::Name(compiler->get_name(resource.id));
            rootConstant.m_offset = 0;
            rootConstant.m_byteSize =
                static_cast<uint32_t>(compiler->get_declared_struct_size(compiler->get_type(resource.base_type_id)));
            m_rootConstants.push_back(rootConstant);
        }
    }


    void ShaderReflection::ParseSpecializationConstants(const spirv_cross::CompilerHLSL* compiler)
    {
        FE_PROFILER_ZONE();

        auto specializationConstants = compiler->get_specialization_constants();
        festd::sort(specializationConstants,
                    [](const SpvC::SpecializationConstant& lhs, const SpvC::SpecializationConstant& rhs) {
                        return lhs.constant_id < rhs.constant_id;
                    });

        for (uint32_t i = 0; i < specializationConstants.size(); ++i)
            FE_Assert(specializationConstants[i].constant_id == i);

        for (const auto& specializationConstant : specializationConstants)
        {
            m_specializationConstantNames.push_back(Env::Name(compiler->get_name(specializationConstant.id)));
        }
    }


    uint32_t ShaderReflection::GetResourceBindingIndex(const Env::Name name) const
    {
        for (const Core::ShaderResourceBinding& binding : m_resourceBindings)
        {
            if (binding.m_name == name)
                return binding.m_slot;
        }

        FE_AssertMsg(false, "Shader resource \"{}\" not found", name);
        return 0;
    }


    uint32_t ShaderReflection::GetInputAttributeLocation(const Env::Name semantic) const
    {
        festd::fixed_string altSemantic{ semantic };
        if (!altSemantic.ends_with("0"))
            altSemantic += "0";

        const Env::Name altSemanticName{ altSemantic };
        for (const Core::ShaderInputAttribute& input : m_inputAttributes)
        {
            if (input.m_shaderSemantic == semantic || input.m_shaderSemantic == altSemanticName)
                return input.m_location;
        }

        FE_AssertMsg(false, "Shader semantic \"{}\" not found in input attributes", semantic);
        return 0;
    }
} // namespace FE::Graphics::Vulkan
