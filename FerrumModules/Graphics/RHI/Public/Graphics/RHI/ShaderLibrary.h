#pragma once
#include <Graphics/RHI/DeviceObject.h>
#include <Graphics/RHI/ShaderStage.h>

namespace FE::Graphics::RHI
{
    struct ShaderHandle final : public TypedHandle<ShaderHandle, uint32_t>
    {
    };


    struct ShaderDefine final
    {
        festd::string_view m_name;
        festd::string_view m_value;
    };


    struct ShaderPermutationDesc final
    {
        Env::Name m_name;
        ShaderStage m_stage = ShaderStage::kVertex;
        festd::span<ShaderDefine> m_defines;

        [[nodiscard]] uint64_t GetHash() const
        {
            Hasher hasher;
            hasher.UpdateRaw(m_name.GetHash());
            hasher.Update(festd::to_underlying(m_stage));

            for (const auto& [name, value] : m_defines)
            {
                hasher.UpdateRaw(DefaultHash(name));
                hasher.UpdateRaw(DefaultHash(value));
            }

            return hasher.Finalize();
        }
    };


    struct ShaderLibrary : public DeviceObject
    {
        [[nodiscard]] HLSLShaderVersion GetShaderVersion() const;

        virtual ShaderHandle GetShader(const ShaderPermutationDesc& desc) = 0;

    protected:
        HLSLShaderVersion m_hlslVersion = HLSLShaderVersion::kDefault;
    };


    inline HLSLShaderVersion ShaderLibrary::GetShaderVersion() const
    {
        return m_hlslVersion;
    }
} // namespace FE::Graphics::RHI
