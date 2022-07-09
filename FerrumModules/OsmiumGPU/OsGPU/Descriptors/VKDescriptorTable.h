#pragma once
#include <OsGPU/Descriptors/IDescriptorTable.h>
#include <OsGPU/Common/VKConfig.h>
#include <FeCore/Containers/List.h>

namespace FE::Osmium
{
    class VKDevice;
    class VKDescriptorHeap;

    class VKDescriptorTable : public Object<IDescriptorTable>
    {
        VKDevice* m_Device;
        Shared<VKDescriptorHeap> m_Heap;

        VkDescriptorSetLayout m_Layout;
        VkDescriptorSet m_Set;

        List<DescriptorDesc> m_Descriptors;

    public:
        FE_CLASS_RTTI(VKDescriptorTable, "262CD421-E748-4F4C-A732-2ABB951D486A");

        VKDescriptorTable(VKDevice& dev, VKDescriptorHeap& heap, const List<DescriptorDesc>& descriptors);
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
}
