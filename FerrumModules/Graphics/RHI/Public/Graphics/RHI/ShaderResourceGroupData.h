#pragma once
#include <festd/vector.h>
#include <Graphics/RHI/Buffer.h>
#include <Graphics/RHI/ImageView.h>
#include <Graphics/RHI/Sampler.h>
#include <Graphics/RHI/ShaderResourceType.h>

namespace FE::Graphics::RHI
{
    struct ShaderResourceEntry final
    {
        uint32_t m_index;
        Rc<DeviceObject> m_object;
    };


    struct ShaderResourceGroupData final
    {
        void Set(uint32_t bindingIndex, Buffer* pBuffer)
        {
            SetImpl(bindingIndex, pBuffer);
        }

        void Set(uint32_t bindingIndex, ImageView* pImageView)
        {
            SetImpl(bindingIndex, pImageView);
        }

        void Set(uint32_t bindingIndex, Sampler* pSampler)
        {
            SetImpl(bindingIndex, pSampler);
        }

        DeviceObject* Get(uint32_t bindingIndex) const
        {
            const auto iter = std::lower_bound(
                m_entries.begin(), m_entries.end(), bindingIndex, [](const ShaderResourceEntry& lhs, uint32_t rhs) {
                    return lhs.m_index < rhs;
                });

            FE_CORE_ASSERT(iter != m_entries.end(), "Index out of range");
            return iter->m_object.Get();
        }

        festd::span<const ShaderResourceEntry> GetEntries() const
        {
            return m_entries;
        }

    private:
        festd::small_vector<ShaderResourceEntry, 8> m_entries;

        void SetImpl(uint32_t bindingIndex, DeviceObject* object)
        {
            FE_Assert(object);

            auto iter = std::lower_bound(
                m_entries.begin(), m_entries.end(), bindingIndex, [](const ShaderResourceEntry& lhs, uint32_t rhs) {
                    return lhs.m_index < rhs;
                });

            if (iter != m_entries.end() && iter->m_index == bindingIndex)
            {
                iter->m_object = object;
                return;
            }

            ShaderResourceEntry& entry = *m_entries.emplace(iter);
            entry.m_index = bindingIndex;
            entry.m_object = object;
        }
    };
} // namespace FE::Graphics::RHI
