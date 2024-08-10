#pragma once
#include <FeCore/Containers/ArraySlice.h>
#include <OsGPU/Descriptors/DescriptorDesc.h>

namespace FE::Osmium
{
    struct DescriptorSize
    {
        uint32_t DescriptorCount = 0;
        ShaderResourceType ResourceType = ShaderResourceType::None;

        DescriptorSize() = default;

        DescriptorSize(uint32_t descriptorCount, ShaderResourceType resourceType)
            : DescriptorCount(descriptorCount)
            , ResourceType(resourceType)
        {
        }

        FE_RTTI_Base(DescriptorSize, "E197D774-8F77-44AD-B841-898E0BEFC798");
    };

    struct DescriptorHeapDesc
    {
        ArraySlice<DescriptorSize> Sizes;
        uint32_t MaxTables = 0;

        FE_RTTI_Base(DescriptorHeapDesc, "CAB78665-9C43-41B4-9E51-36ACBB293B32");
    };

    class IDescriptorTable;

    class IDescriptorHeap : public Memory::RefCountedObjectBase
    {
    public:
        FE_RTTI_Class(IDescriptorHeap, "B6F00D47-B483-4919-87F9-25348091482F");

        ~IDescriptorHeap() override = default;

        virtual Rc<IDescriptorTable> AllocateDescriptorTable(const ArraySlice<DescriptorDesc>& descriptors) = 0;

        virtual void Reset() = 0;
    };
} // namespace FE::Osmium
