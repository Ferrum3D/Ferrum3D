#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Descriptors/IDescriptorTable.h>

namespace FE::Osmium
{
    class VKDevice;
    class VKDescriptorHeap;

    class VKDescriptorTable : public IDescriptorTable
    {
        VKDevice* m_Device;
        Rc<VKDescriptorHeap> m_Heap;

        VkDescriptorSetLayout m_Layout;
        VkDescriptorSet m_Set;

        size_t m_LayoutHash;

        eastl::vector<DescriptorDesc> m_Descriptors;

    public:
        FE_RTTI_Class(VKDescriptorTable, "262CD421-E748-4F4C-A732-2ABB951D486A");

        VKDescriptorTable(VKDevice& dev, VKDescriptorHeap& heap, const ArraySlice<DescriptorDesc>& descriptors);
        ~VKDescriptorTable() override;

        void Update(const DescriptorWriteBuffer& descriptorWriteBuffer) override;
        void Update(const DescriptorWriteImage& descriptorWriteBuffer) override;
        void Update(const DescriptorWriteSampler& descriptorWriteBuffer) override;

        inline VkDescriptorSet GetNativeSet();
        inline VkDescriptorSetLayout GetNativeSetLayout();
    };

    inline VkDescriptorSet VKDescriptorTable::GetNativeSet()
    {
        return m_Set;
    }

    inline VkDescriptorSetLayout VKDescriptorTable::GetNativeSetLayout()
    {
        return m_Layout;
    }
} // namespace FE::Osmium
