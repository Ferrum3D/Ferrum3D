#pragma once
#include <OsGPU/Descriptors/DescriptorDesc.h>

namespace FE::Osmium
{
    class IDescriptorTable : public Memory::RefCountedObjectBase
    {
    public:
        FE_RTTI_Class(IDescriptorTable, "C10FCB1B-31C8-47EE-AE2F-E53463494B85");

        ~IDescriptorTable() override = default;

        virtual void Update(const DescriptorWriteBuffer& descriptorWriteBuffer) = 0;
        virtual void Update(const DescriptorWriteImage& descriptorWriteBuffer) = 0;
        virtual void Update(const DescriptorWriteSampler& descriptorWriteBuffer) = 0;
    };
} // namespace FE::Osmium
