#pragma once
#include <Graphics/Core/DeviceObject.h>
#include <Graphics/Core/ShaderDefines.h>
#include <Graphics/Core/ShaderStage.h>

namespace FE::Graphics::Core
{
    struct ShaderHandle final : public TypedHandle<ShaderHandle, uint32_t>
    {
    };


    struct ShaderPermutationDesc final
    {
        Env::Name m_name;
        ShaderStage m_stage = ShaderStage::kVertex;
        festd::span<const ShaderDefine> m_defines;

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
        FE_RTTI_Class(ShaderLibrary, "BE44FCFD-5540-49F6-AECE-569BE88A8450");

        virtual ShaderHandle GetShader(const ShaderPermutationDesc& desc) = 0;
    };
} // namespace FE::Graphics::Core
