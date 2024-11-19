#pragma once
#include <Graphics/RHI/Image.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::RHI
{
    struct ImageBase : public Image
    {
        FE_RTTI_Class(ImageBase, "40A58181-9BB7-4EC1-9DFA-A27B3F586B5A");

        void InitState(uint16_t arraySize, uint16_t mipLevelCount)
        {
            m_states.resize(arraySize);
            for (auto& arr : m_states)
            {
                arr.resize(mipLevelCount, ResourceState::kUndefined);
            }
        }

        ~ImageBase() override = default;

        void SetState(const ImageSubresourceRange& subresourceRange, ResourceState state) override
        {
            for (uint32_t i = 0; i < subresourceRange.m_arraySliceCount; ++i)
            {
                for (uint32_t j = 0; j < subresourceRange.m_mipSliceCount; ++j)
                {
                    m_states[i + subresourceRange.m_minArraySlice][j + subresourceRange.m_minMipSlice] = state;
                }
            }
        }

        [[nodiscard]] ResourceState GetState(const ImageSubresourceRange& subresourceRange) const
        {
            auto state = m_states[subresourceRange.m_minArraySlice][subresourceRange.m_minMipSlice];
            for (uint32_t i = 0; i < subresourceRange.m_arraySliceCount; ++i)
            {
                for (uint32_t j = 0; j < subresourceRange.m_mipSliceCount; ++j)
                {
                    FE_AssertMsg(m_states[i + subresourceRange.m_minArraySlice][j + subresourceRange.m_minMipSlice] == state,
                                 "States in specified subresource range where different");
                }
            }

            return state;
        }

        [[nodiscard]] ResourceState GetState(uint16_t arraySlice, uint16_t mipSlice) const override
        {
            return m_states[arraySlice][mipSlice];
        }

    private:
        festd::vector<festd::vector<ResourceState>> m_states;
    };
} // namespace FE::Graphics::RHI
