#pragma once
#include <HAL/DescriptorDesc.h>
#include <HAL/DeviceObject.h>

namespace FE::Graphics::HAL
{
    class DescriptorTable : public DeviceObject
    {
    public:
        FE_RTTI_Class(DescriptorTable, "C10FCB1B-31C8-47EE-AE2F-E53463494B85");

        ~DescriptorTable() override = default;

        virtual void Update(const DescriptorWriteBuffer& descriptorWriteBuffer) = 0;
        virtual void Update(const DescriptorWriteImage& descriptorWriteBuffer) = 0;
        virtual void Update(const DescriptorWriteSampler& descriptorWriteBuffer) = 0;
    };
} // namespace FE::Graphics::HAL
