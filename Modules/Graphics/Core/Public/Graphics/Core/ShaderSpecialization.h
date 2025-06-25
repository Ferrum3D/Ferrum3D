#pragma once
#include <festd/string.h>
#include <festd/vector.h>

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


    Env::Name CombineDefines(festd::span<const ShaderDefine> defines);

    festd::pmr::vector<ShaderDefine> SplitDefines(Env::Name defineStorage, std::pmr::memory_resource* allocator);


    struct ShaderSpecializationConstant final
    {
        Env::Name m_name;
        int32_t m_value = 0;

        [[nodiscard]] uint64_t GetHash() const
        {
            Hasher hasher;
            hasher.UpdateRaw(m_name.GetHash());
            hasher.Update(m_value);
            return hasher.Finalize();
        }
    };
} // namespace FE::Graphics::Core
