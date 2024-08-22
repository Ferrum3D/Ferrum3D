#pragma once
#include <HAL/DescriptorTable.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    class Device;
    class DescriptorHeap;
    class DescriptorAllocator;

    class DescriptorTable : public HAL::DescriptorTable
    {
        Rc<DescriptorHeap> m_Heap;
        Rc<DescriptorAllocator> m_DescriptorAllocator;

        VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
        VkDescriptorSet m_Set = VK_NULL_HANDLE;

        size_t m_LayoutHash = 0;

        eastl::vector<HAL::DescriptorDesc> m_Descriptors;

    public:
        FE_RTTI_Class(DescriptorTable, "262CD421-E748-4F4C-A732-2ABB951D486A");

        DescriptorTable(HAL::Device* pDevice, DescriptorHeap* pHeap, DescriptorAllocator* pDescriptorAllocator,
                        festd::span<const HAL::DescriptorDesc> descriptors);
        ~DescriptorTable() override;

        void Update(const HAL::DescriptorWriteBuffer& descriptorWriteBuffer) override;
        void Update(const HAL::DescriptorWriteImage& descriptorWriteBuffer) override;
        void Update(const HAL::DescriptorWriteSampler& descriptorWriteBuffer) override;

        inline VkDescriptorSet GetNativeSet() const;
        inline VkDescriptorSetLayout GetNativeSetLayout() const;
    };

    inline VkDescriptorSet DescriptorTable::GetNativeSet() const
    {
        return m_Set;
    }

    inline VkDescriptorSetLayout DescriptorTable::GetNativeSetLayout() const
    {
        return m_Layout;
    }

    FE_ENABLE_IMPL_CAST(DescriptorTable);
} // namespace FE::Graphics::Vulkan
