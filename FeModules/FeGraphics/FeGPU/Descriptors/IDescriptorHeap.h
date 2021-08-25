#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/RTTI/RTTI.h>
#include <FeGPU/Descriptors/IDescriptorTable.h>

namespace FE::GPU
{
    struct DescriptorSize
    {
        UInt32 DescriptorCount          = 0;
        ShaderResourceType ResourceType = ShaderResourceType::None;

        DescriptorSize() = default;

        DescriptorSize(UInt32 descriptorCount, ShaderResourceType resourceType)
            : DescriptorCount(descriptorCount)
            , ResourceType(resourceType)
        {
        }

        FE_STRUCT_RTTI(DescriptorSize, "E197D774-8F77-44AD-B841-898E0BEFC798");
    };

    struct DescriptorHeapDesc
    {
        Vector<DescriptorSize> Sizes;
        UInt32 MaxSets = 0;

        FE_STRUCT_RTTI(DescriptorHeapDesc, "CAB78665-9C43-41B4-9E51-36ACBB293B32");
    };

    class IDescriptorHeap : public IObject
    {
    public:
        FE_CLASS_RTTI(IDescriptorHeap, "B6F00D47-B483-4919-87F9-25348091482F");

        ~IDescriptorHeap() override = default;

        virtual RefCountPtr<IDescriptorTable> AllocateDescriptorTable(const Vector<DescriptorDesc>& descriptors) = 0;
    };
} // namespace FE::GPU
