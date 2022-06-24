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
        VKDescriptorHeap* m_Heap;

        vk::UniqueDescriptorSetLayout m_Layout;
        vk::DescriptorSet m_Set;

        List<DescriptorDesc> m_Descriptors;

    public:
        FE_CLASS_RTTI(VKDescriptorTable, "262CD421-E748-4F4C-A732-2ABB951D486A");

        VKDescriptorTable(VKDevice& dev, VKDescriptorHeap& heap, const List<DescriptorDesc>& descriptors);

        void Update(const DescriptorWriteBuffer& descriptorWriteBuffer) override;

        inline vk::DescriptorSet& GetNativeSet();
        inline vk::DescriptorSetLayout& GetNativeSetLayout();
    };

    inline vk::DescriptorSet& VKDescriptorTable::GetNativeSet()
    {
        return m_Set;
    }

    inline vk::DescriptorSetLayout& VKDescriptorTable::GetNativeSetLayout()
    {
        return m_Layout.get();
    }
}
