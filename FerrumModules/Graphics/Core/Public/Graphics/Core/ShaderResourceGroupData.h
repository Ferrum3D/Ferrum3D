#pragma once
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Image.h>
#include <Graphics/Core/Sampler.h>
#include <festd/vector.h>

namespace FE::Graphics::Core
{
    struct ShaderResourceEntry final
    {
        enum class Type : uint32_t
        {
            kImage,
            kBuffer,
            kSampler,
        };

        Type m_type;
        uint32_t m_index;
        Rc<Resource> m_resource;

        union
        {
            ImageSubresource m_imageSubresource;
            BufferSubresource m_bufferSubresource;
            SamplerState m_sampler;
        };
    };


    struct ShaderResourceGroupData final
    {
        void Set(const uint32_t bindingIndex, Buffer* buffer, const BufferSubresource& subresource)
        {
            ShaderResourceEntry& entry = SetImpl(bindingIndex);
            entry.m_type = ShaderResourceEntry::Type::kBuffer;
            entry.m_resource = buffer;
            entry.m_bufferSubresource = subresource;
        }

        void Set(const uint32_t bindingIndex, Buffer* buffer)
        {
            const BufferDesc& bufferDesc = buffer->GetDesc();
            ShaderResourceEntry& entry = SetImpl(bindingIndex);
            entry.m_type = ShaderResourceEntry::Type::kBuffer;
            entry.m_resource = buffer;
            entry.m_bufferSubresource.m_offset = 0;
            entry.m_bufferSubresource.m_size = bufferDesc.m_size;
        }

        void Set(const uint32_t bindingIndex, Image* image, const ImageSubresource& subresource)
        {
            ShaderResourceEntry& entry = SetImpl(bindingIndex);
            entry.m_type = ShaderResourceEntry::Type::kImage;
            entry.m_resource = image;
            entry.m_imageSubresource = subresource;
        }

        void Set(const uint32_t bindingIndex, Image* image)
        {
            const ImageDesc& imageDesc = image->GetDesc();
            ShaderResourceEntry& entry = SetImpl(bindingIndex);
            entry.m_type = ShaderResourceEntry::Type::kImage;
            entry.m_resource = image;
            entry.m_imageSubresource.m_mostDetailedMipSlice = 0;
            entry.m_imageSubresource.m_mipSliceCount = imageDesc.m_mipSliceCount;
            entry.m_imageSubresource.m_firstArraySlice = 0;
            entry.m_imageSubresource.m_arraySize = imageDesc.m_arraySize;
        }

        void Set(const uint32_t bindingIndex, const SamplerState sampler)
        {
            ShaderResourceEntry& entry = SetImpl(bindingIndex);
            entry.m_type = ShaderResourceEntry::Type::kSampler;
            entry.m_sampler = sampler;
        }

        [[nodiscard]] Resource* GetResource(const uint32_t bindingIndex) const
        {
            const auto iter = std::lower_bound(
                m_entries.begin(), m_entries.end(), bindingIndex, [](const ShaderResourceEntry& lhs, const uint32_t rhs) {
                    return lhs.m_index < rhs;
                });

            FE_Assert(iter != m_entries.end(), "Index out of range");
            FE_Assert(iter->m_type == ShaderResourceEntry::Type::kImage || iter->m_type == ShaderResourceEntry::Type::kBuffer,
                      "Not a resource");
            return iter->m_resource.Get();
        }

        [[nodiscard]] Image* GetImage(const uint32_t bindingIndex) const
        {
            return fe_assert_cast<Image*>(GetResource(bindingIndex));
        }

        [[nodiscard]] Buffer* GetBuffer(const uint32_t bindingIndex) const
        {
            return fe_assert_cast<Buffer*>(GetResource(bindingIndex));
        }

        [[nodiscard]] SamplerState GetSampler(const uint32_t bindingIndex) const
        {
            const auto iter = std::lower_bound(
                m_entries.begin(), m_entries.end(), bindingIndex, [](const ShaderResourceEntry& lhs, const uint32_t rhs) {
                    return lhs.m_index < rhs;
                });

            FE_Assert(iter != m_entries.end(), "Index out of range");
            FE_Assert(iter->m_type == ShaderResourceEntry::Type::kSampler, "Not a sampler");
            return iter->m_sampler;
        }

        [[nodiscard]] festd::span<const ShaderResourceEntry> GetEntries() const
        {
            return m_entries;
        }

    private:
        ShaderResourceEntry& SetImpl(const uint32_t bindingIndex)
        {
            const auto iter = std::lower_bound(
                m_entries.begin(), m_entries.end(), bindingIndex, [](const ShaderResourceEntry& lhs, const uint32_t rhs) {
                    return lhs.m_index < rhs;
                });

            if (iter != m_entries.end() && iter->m_index == bindingIndex)
            {
                return *iter;
            }

            ShaderResourceEntry& entry = *m_entries.emplace(iter, ShaderResourceEntry{});
            entry.m_index = bindingIndex;
            return entry;
        }

        festd::small_vector<ShaderResourceEntry, 8> m_entries;
    };
} // namespace FE::Graphics::Core
