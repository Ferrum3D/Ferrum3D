#include <FeCore/Console/FeLog.h>
#include <OsGPU/Shader/VKShaderReflection.h>

namespace FE::Osmium
{
    namespace SpvC = spirv_cross;

    inline static Format SPIRTypeToFormat(const SpvC::SPIRType& type)
    {
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

    VKShaderReflection::VKShaderReflection(const eastl::vector<uint32_t>& byteCode)
    {
        auto compiler = SpvC::CompilerHLSL(byteCode.data(), byteCode.size());
        ParseInputAttributes(compiler);
    }

    eastl::vector<ShaderInputAttribute> VKShaderReflection::GetInputAttributes()
    {
        return m_InputAttributes;
    }

    void VKShaderReflection::ParseInputAttributes(SpvC::CompilerHLSL& compiler)
    {
        SpvC::ShaderResources resources = compiler.get_shader_resources();
        for (const auto& resource : resources.stage_inputs)
        {
            const auto& semantic = compiler.get_decoration_string(resource.id, spv::DecorationHlslSemanticGOOGLE);
            auto semanticSize = semantic.length();
            if (!semantic.empty() && semantic.back() == '0')
            {
                --semanticSize;
            }

            auto& current = m_InputAttributes.push_back();
            current.Location = compiler.get_decoration(resource.id, spv::DecorationLocation);
            current.ShaderSemantic = String(semantic.c_str(), semanticSize);
            current.ElementFormat = SPIRTypeToFormat(compiler.get_type(resource.base_type_id));
        }
    }

    uint32_t VKShaderReflection::GetInputAttributeLocation(StringSlice semantic)
    {
        for (auto& input : m_InputAttributes)
        {
            if (input.ShaderSemantic == semantic)
            {
                return input.Location;
            }
        }

        FE_UNREACHABLE("Shader semantic \"{}\" not found in input attributes", semantic);
        return 0;
    }
} // namespace FE::Osmium
