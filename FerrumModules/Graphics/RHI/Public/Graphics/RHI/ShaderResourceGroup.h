#pragma once
#include <Graphics/RHI/ShaderResourceGroupData.h>

namespace FE::Graphics::RHI
{
    struct ShaderReflection;

    struct ShaderResourceGroupDesc final
    {
        festd::span<const ShaderReflection* const> m_shadersReflection;
    };


    struct ShaderResourceGroup : public DeviceObject
    {
        FE_RTTI_Class(ShaderResourceGroup, "DC97734F-067B-4115-8225-EDAF9B365267");

        virtual RHI::ResultCode Init(const ShaderResourceGroupDesc& desc) = 0;
        virtual void Update(const ShaderResourceGroupData& data) = 0;

    protected:
        ShaderResourceGroupData m_data;
    };
} // namespace FE::Graphics::RHI
