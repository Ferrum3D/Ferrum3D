#pragma once
#include <OsGPU/Descriptors/DescriptorDesc.h>

namespace FE::GPU
{
    class IDescriptorTable : public IObject
    {
    public:
        FE_CLASS_RTTI(IDescriptorTable, "C10FCB1B-31C8-47EE-AE2F-E53463494B85");

        ~IDescriptorTable() override = default;

        virtual void Update(const DescriptorWriteBuffer& descriptorWriteBuffer) = 0;
    };
} // namespace FE::GPU
