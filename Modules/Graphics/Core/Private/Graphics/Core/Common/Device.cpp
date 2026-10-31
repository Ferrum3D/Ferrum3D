#include <Graphics/Core/Common/Device.h>
#include <Graphics/Core/Format.h>
#include <Graphics/Core/Resource.h>

namespace FE::Graphics::Common
{
    Device::Device()
    {
#if FE_DEVELOPMENT
#    define FE_FORMAT_FUNC(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index) Core::Format::k##name,

        constexpr Core::Format formats[] = { FE_EXPAND_FORMATS(FE_FORMAT_FUNC) };

        festd::vector<uint32_t> formatIndices;
        formatIndices.reserve(festd::size(formats));

        for (const Core::Format format : formats)
        {
            const Core::FormatInfo formatInfo{ format };
            for (const uint32_t index : formatIndices)
            {
                FE_Assert(formatInfo.m_formatIndex != index);
            }

            formatIndices.push_back(formatInfo.m_formatIndex);
        }
#endif
    }


    uint32_t Device::RegisterResource(Core::Resource* resource)
    {
        FE_PROFILER_ZONE();

        std::lock_guard lock{ m_resourceListLock };
        m_resourceList.push_back(*resource);

        if (!m_resourceIdFreeList.empty())
        {
            const uint32_t id = m_resourceIdFreeList.back();
            m_resourceIdFreeList.pop_back();
            return id;
        }

        return m_currentResourceId++;
    }


    void Device::UnregisterResource(const uint32_t id, Core::Resource* resource)
    {
        FE_PROFILER_ZONE();

        std::lock_guard lock{ m_resourceListLock };
        festd::intrusive_list<>::remove(*resource);
        m_resourceIdFreeList.push_back(id);
    }
} // namespace FE::Graphics::Common
