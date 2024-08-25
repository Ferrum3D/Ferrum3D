#pragma once
#include <FeCore/Containers/SmallVector.h>
#include <HAL/Buffer.h>
#include <HAL/ImageView.h>
#include <HAL/Sampler.h>
#include <HAL/ShaderResourceType.h>

namespace FE::Graphics::HAL
{
    struct ShaderResourceEntry final
    {
        uint32_t Index;
        Rc<DeviceObject> pObject;
    };


    class ShaderResourceGroupData final
    {
        festd::small_vector<ShaderResourceEntry, 8> m_Entries;

        inline void SetImpl(uint32_t bindingIndex, DeviceObject* pObject)
        {
            auto iter = std::lower_bound(
                m_Entries.begin(), m_Entries.end(), bindingIndex, [](const ShaderResourceEntry& lhs, uint32_t rhs) {
                    return lhs.Index < rhs;
                });

            if (iter != m_Entries.end() && iter->Index == bindingIndex)
            {
                iter->pObject = pObject;
                return;
            }

            ShaderResourceEntry& entry = *m_Entries.emplace(iter);
            entry.Index = bindingIndex;
            entry.pObject = pObject;
        }

    public:
        inline void Set(uint32_t bindingIndex, Buffer* pBuffer)
        {
            SetImpl(bindingIndex, pBuffer);
        }

        inline void Set(uint32_t bindingIndex, ImageView* pImageView)
        {
            SetImpl(bindingIndex, pImageView);
        }

        inline void Set(uint32_t bindingIndex, Sampler* pSampler)
        {
            SetImpl(bindingIndex, pSampler);
        }

        inline DeviceObject* Get(uint32_t bindingIndex) const
        {
            const auto iter = std::lower_bound(
                m_Entries.begin(), m_Entries.end(), bindingIndex, [](const ShaderResourceEntry& lhs, uint32_t rhs) {
                    return lhs.Index < rhs;
                });

            FE_CORE_ASSERT(iter != m_Entries.end(), "Index out of range");
            return iter->pObject.Get();
        }

        inline festd::span<const ShaderResourceEntry> GetEntries() const
        {
            return m_Entries;
        }
    };
} // namespace FE::Graphics::HAL
