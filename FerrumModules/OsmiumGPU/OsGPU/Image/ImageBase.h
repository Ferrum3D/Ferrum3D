#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Image/IImage.h>

namespace FE::Osmium
{
    class ImageBase : public IImage
    {
        eastl::vector<eastl::vector<ResourceState>> m_States;

    public:
        FE_CLASS_RTTI(ImageBase, "40A58181-9BB7-4EC1-9DFA-A27B3F586B5A");

        inline ImageBase(UInt16 arraySize, UInt16 mipLevelCount)
        {
            m_States.resize(arraySize);
            for (auto& arr : m_States)
            {
                arr.resize(mipLevelCount, ResourceState::Undefined);
            }
        }

        ~ImageBase() override = default;

        inline void SetState(const ImageSubresourceRange& subresourceRange, ResourceState state) override
        {
            for (uint32_t i = 0; i < subresourceRange.ArraySliceCount; ++i)
            {
                for (uint32_t j = 0; j < subresourceRange.MipSliceCount; ++j)
                {
                    m_States[i + subresourceRange.MinArraySlice][j + subresourceRange.MinMipSlice] = state;
                }
            }
        }

        [[nodiscard]] inline ResourceState GetState(const ImageSubresourceRange& subresourceRange) const
        {
            auto state = m_States[subresourceRange.MinArraySlice][subresourceRange.MinMipSlice];
            for (uint32_t i = 0; i < subresourceRange.ArraySliceCount; ++i)
            {
                for (uint32_t j = 0; j < subresourceRange.MipSliceCount; ++j)
                {
                    FE_ASSERT_MSG(m_States[i + subresourceRange.MinArraySlice][j + subresourceRange.MinMipSlice] == state,
                                  "States in specified subresource range where different");
                }
            }

            return state;
        }

        [[nodiscard]] inline ResourceState GetState(UInt16 arraySlice, UInt16 mipSlice) const override
        {
            return m_States[arraySlice][mipSlice];
        }
    };
} // namespace FE::Osmium
