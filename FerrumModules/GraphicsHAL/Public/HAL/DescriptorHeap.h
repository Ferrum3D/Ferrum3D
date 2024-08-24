#pragma once
#include <HAL/DescriptorDesc.h>
#include <HAL/DeviceObject.h>

namespace FE::Graphics::HAL
{
    struct DescriptorSize
    {
        uint32_t DescriptorCount = 0;
        ShaderResourceType ResourceType = ShaderResourceType::None;

        inline DescriptorSize() = default;

        inline DescriptorSize(uint32_t descriptorCount, ShaderResourceType resourceType)
            : DescriptorCount(descriptorCount)
            , ResourceType(resourceType)
        {
        }
    };


    struct DescriptorHeapDesc
    {
        festd::span<const DescriptorSize> Sizes;
        uint32_t MaxTables = 0;
    };


    class DescriptorTable;

    class DescriptorHeap : public DeviceObject
    {
    public:
        ~DescriptorHeap() override = default;

        FE_RTTI_Class(DescriptorHeap, "B6F00D47-B483-4919-87F9-25348091482F");

        virtual ResultCode Init(const DescriptorHeapDesc& desc) = 0;

        virtual Rc<DescriptorTable> AllocateDescriptorTable(festd::span<const DescriptorDesc> descriptors) = 0;

        virtual void Reset() = 0;
    };
} // namespace FE::Graphics::HAL
