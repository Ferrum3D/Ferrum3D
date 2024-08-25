#pragma once
#include <HAL/ShaderResourceGroupData.h>

namespace FE::Graphics::HAL
{
    class ShaderReflection;

    struct ShaderResourceGroupDesc final
    {
        festd::span<const ShaderReflection* const> ShadersReflection;
    };


    class ShaderResourceGroup : public DeviceObject
    {
    protected:
        ShaderResourceGroupData m_Data;

    public:
        FE_RTTI_Class(ShaderResourceGroup, "DC97734F-067B-4115-8225-EDAF9B365267");

        virtual HAL::ResultCode Init(const ShaderResourceGroupDesc& desc) = 0;
        virtual void Update(const ShaderResourceGroupData& data) = 0;
    };
} // namespace FE::Graphics::HAL
