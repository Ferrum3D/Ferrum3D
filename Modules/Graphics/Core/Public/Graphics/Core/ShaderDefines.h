#pragma once
#include <festd/string.h>

namespace FE::Graphics::Core
{
    struct ShaderDefine final
    {
        festd::string_view m_name;
        festd::string_view m_value;

        [[nodiscard]] uint64_t GetHash() const
        {
            Hasher hasher;
            hasher.UpdateRaw(DefaultHash(m_name));
            hasher.UpdateRaw(DefaultHash(m_value));
            return hasher.Finalize();
        }
    };


    struct DefinesStorage final
    {
        void Init(festd::span<const ShaderDefine> defines);

        [[nodiscard]] ShaderDefine operator[](uint32_t defineIndex) const;

        [[nodiscard]] festd::pmr::vector<ShaderDefine> ToVector(std::pmr::memory_resource* allocator = nullptr) const;

        festd::small_vector<uint16_t> m_defineOffsets;
        festd::small_vector<char, 256> m_storage;
        uint32_t m_defineCount = 0;
    };


    inline void DefinesStorage::Init(const festd::span<const ShaderDefine> defines)
    {
        FE_PROFILER_ZONE();

        m_defineCount = defines.size();

        uint32_t offset = 0;
        for (const ShaderDefine& define : defines)
        {
            m_defineOffsets.push_back(static_cast<uint16_t>(offset));
            offset += define.m_name.size();
            m_defineOffsets.push_back(static_cast<uint16_t>(offset));
            offset += define.m_value.size();

            FE_Assert(offset <= Constants::kMaxU16);
        }

        m_defineOffsets.push_back(static_cast<uint16_t>(offset));

        m_storage.resize(offset);

        offset = 0;
        for (const ShaderDefine& define : defines)
        {
            memcpy(m_storage.data() + offset, define.m_name.data(), define.m_name.size());
            offset += define.m_name.size();
            memcpy(m_storage.data() + offset, define.m_value.data(), define.m_value.size());
            offset += define.m_value.size();
        }
    }


    inline ShaderDefine DefinesStorage::operator[](const uint32_t defineIndex) const
    {
        const uint32_t nameStartOffset = m_defineOffsets[defineIndex * 2];
        const uint32_t valueStartOffset = m_defineOffsets[defineIndex * 2 + 1];
        const uint32_t endOffset = m_defineOffsets[defineIndex * 2 + 2];

        ShaderDefine define;
        define.m_name = festd::string_view{ m_storage.data() + nameStartOffset, valueStartOffset - nameStartOffset };
        define.m_value = festd::string_view{ m_storage.data() + valueStartOffset, endOffset - valueStartOffset };
        return define;
    }


    inline festd::pmr::vector<ShaderDefine> DefinesStorage::ToVector(std::pmr::memory_resource* allocator) const
    {
        if (allocator == nullptr)
            allocator = std::pmr::get_default_resource();

        festd::pmr::vector<ShaderDefine> defines{ allocator };
        defines.reserve(m_defineCount);

        for (uint32_t defineIndex = 0; defineIndex < m_defineCount; ++defineIndex)
            defines.push_back((*this)[defineIndex]);

        return defines;
    }
} // namespace FE::Graphics::Core
