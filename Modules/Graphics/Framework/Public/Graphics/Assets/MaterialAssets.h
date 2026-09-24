#pragma once
#include <Core/IO/Assets.h>
#include <Core/Math/Vector4.h>
#include <Graphics/Assets/Assets.h>
#include <Graphics/Core/GraphicsPipeline.h>
#include <festd/string.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    struct MaterialInstanceRuntime;

    enum class MaterialParameterType : uint32_t
    {
        kScalar,
        kVector,
        kTexture2D,
    };


    enum class MaterialParameterScope : uint32_t
    {
        kMaterial,
        kInstance,
    };


    struct MaterialParameterDesc final
    {
        Env::Name m_name;
        MaterialParameterType m_type = MaterialParameterType::kScalar;
        MaterialParameterScope m_scope = MaterialParameterScope::kMaterial;
        Vector4 m_defaultValue = Vector4::kZero;

        FE_RTTI_Reflect("1E04CE30-9984-481C-B70B-F641777F69C4");
        FE_RTTI_Serialize();
    };


    struct MaterialSpecializerDesc final
    {
        Env::Name m_name;
        festd::vector<int32_t> m_values;
        int32_t m_defaultValue = 0;

        FE_RTTI_Reflect("A4A47248-86F8-4FC6-B312-A92713568E55");
        FE_RTTI_Serialize();
    };


    struct MaterialTechniqueDesc final
    {
        Env::Name m_role;
        festd::string m_permutationKey;
        Env::Name m_vertexShader;
        Env::Name m_amplificationShader;
        Env::Name m_meshShader;
        Env::Name m_pixelShader;
        festd::string m_shaderDefines;
        Core::RasterizationState m_rasterization = Core::RasterizationState::kFillNoCull;
        Core::DepthStencilState m_depthStencil = Core::DepthStencilState::kDisabled;
        Core::ColorBlendState m_blend = Core::ColorBlendState::Create(Core::TargetColorBlending::kDisabled);

        FE_RTTI_Reflect("726163F8-FA99-4077-BE18-0D7C050054DF");
        FE_RTTI_Serialize();
    };


    struct MaterialAsset final
    {
        festd::string m_name;
        festd::vector<MaterialSpecializerDesc> m_specializers;
        festd::vector<MaterialParameterDesc> m_parameters;
        festd::vector<MaterialTechniqueDesc> m_techniques;

        FE_RTTI("8A685626-DFE1-4E08-A4B0-9BE046F58A5D");
        FE_RTTI_Reflect();
        FE_RTTI_Serialize();
    };


    struct MaterialParameterValue final
    {
        Env::Name m_name;
        Vector4 m_value = Vector4::kZero;
        IO::Link<TextureAsset> m_texture;

        FE_RTTI_Reflect("5304E304-6125-47B1-9BFC-EBD8874B6D53");
        FE_RTTI_Serialize();
    };


    struct MaterialInstanceAsset final
    {
        IO::Link<MaterialAsset> m_material;
        festd::string m_permutationKey;
        festd::vector<MaterialParameterValue> m_parameters;
        FE_SKIP_SERIALIZING MaterialInstanceRuntime* m_runtime = nullptr;

        FE_RTTI("44F1653F-B716-4E1D-A631-EAA8E728DC08");
        FE_RTTI_Reflect();
        FE_RTTI_Serialize();
    };
} // namespace FE::Graphics
