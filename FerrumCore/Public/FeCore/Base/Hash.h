#pragma once
#include <FeCore/Base/BaseMath.h>
#include <FeCore/Base/wyhash.h>
#include <cstdint>
#include <string_view>

#if FE_COMPILER_MSVC
#    include <intrin.h>
#    pragma intrinsic(_umul128)
#endif

namespace FE
{
    namespace Internal
    {
        // Our hash functions are based on WyHash: https://github.com/wangyi-fudan/wyhash
        // WyHash is released into public domain.
        // Here is a gist with my compile-time implementation: https://gist.github.com/n-dub/56acb5aa3665fefd9dd6ca21c2748886
        //
        // The code below assumes little-endian and implements only the most basic functionality of the original
        // library: string hashes and uint64_t hashes.
        //


        constexpr uint64_t HashSecret[] = {
            UINT64_C(0x2d358dccaa6c78a5), UINT64_C(0x8bb84b93962eacc9), UINT64_C(0x4b33a62ed433d4a3), UINT64_C(0x4d5a2da51de1aa47)
        };

        inline constexpr uint64_t WyRead4(const char* p)
        {
            return static_cast<uint64_t>(p[0]) | (static_cast<uint64_t>(p[1]) << 8) | (static_cast<uint64_t>(p[2]) << 16)
                | (static_cast<uint64_t>(p[3]) << 24);
        }

        inline constexpr uint64_t WyRead8(const char* p)
        {
            return static_cast<uint64_t>(p[0]) | (static_cast<uint64_t>(p[1]) << 8) | (static_cast<uint64_t>(p[2]) << 16)
                | (static_cast<uint64_t>(p[3]) << 24) | (static_cast<uint64_t>(p[4]) << 32) | (static_cast<uint64_t>(p[5]) << 40)
                | (static_cast<uint64_t>(p[6]) << 48) | (static_cast<uint64_t>(p[7]) << 56);
        }

        inline constexpr uint64_t WyRead3(const char* p, const size_t k)
        {
            return ((static_cast<uint64_t>(p[0])) << 16) | ((static_cast<uint64_t>(p[k >> 1])) << 8) | p[k - 1];
        }

        inline constexpr uint64_t WyMix(uint64_t A, uint64_t B)
        {
#if FE_COMPILER_MSVC
            return Math::CompileTime::Multiply128(A, B, &B) ^ B;
#else
            __uint128_t r = A;
            r *= B;
            A = static_cast<uint64_t>(r);
            B = static_cast<uint64_t>(r >> 64);
            return A ^ B;
#endif
        }

        inline constexpr uint64_t WyHash64(uint64_t A, uint64_t B)
        {
            A ^= UINT64_C(0x2d358dccaa6c78a5);
            B ^= UINT64_C(0x8bb84b93962eacc9);
            A = Math::CompileTime::Multiply128(A, B, &B);

            return WyMix(A ^ UINT64_C(0x2d358dccaa6c78a5), B ^ UINT64_C(0x8bb84b93962eacc9));
        }

        // This is useful for function signatures when compiling on Windows. `std::string_view` is a template class
        // (`std::basic_string_view< ... >`) which makes it difficult to retrieve typename from a template function
        // signature since it has multiple templates.
        struct SVWrapper final
        {
            std::string_view value;
        };

        //! @brief Remove leading and trailing spaces from a string view.
        inline constexpr std::string_view TrimTypeName(std::string_view name)
        {
            name.remove_prefix(name.find_first_not_of(' '));
            name.remove_suffix(name.length() - name.find_last_not_of(' ') - 1);
            return name;
        }

        template<class T>
        inline constexpr SVWrapper TypeNameImpl()
        {
#if FE_COMPILER_MSVC
            std::string_view fn = __FUNCSIG__;
            fn.remove_prefix(fn.find_first_of("<") + 1);
            fn.remove_suffix(fn.length() - fn.find_last_of(">"));
#else
            std::string_view fn = __PRETTY_FUNCTION__;
            fn.remove_prefix(fn.find_first_of('=') + 1);
            fn.remove_suffix(fn.length() - fn.find_last_of(']'));
#endif
            return SVWrapper{ TrimTypeName(fn) };
        }
    } // namespace Internal


    [[nodiscard]] constexpr uint64_t CompileTimeHash(const char* p, const size_t len) noexcept
    {
        uint64_t seed = Internal::HashSecret[0], a = 0, b = 0;
        seed ^= Internal::WyMix(seed ^ Internal::HashSecret[0], Internal::HashSecret[1]);
        if (len <= 16)
        {
            if (len >= 4)
            {
                a = (Internal::WyRead4(p) << 32) | Internal::WyRead4(p + ((len >> 3) << 2));
                b = (Internal::WyRead4(p + len - 4) << 32) | Internal::WyRead4(p + len - 4 - ((len >> 3) << 2));
            }
            else if (len > 0)
            {
                a = Internal::WyRead3(p, len);
                b = 0;
            }
            else
            {
                a = b = 0;
            }
        }
        else
        {
            size_t i = len;
            if (i > 48)
            {
                uint64_t see1 = seed, see2 = seed;
                do
                {
                    seed = Internal::WyMix(Internal::WyRead8(p) ^ Internal::HashSecret[1], Internal::WyRead8(p + 8) ^ seed);
                    see1 = Internal::WyMix(Internal::WyRead8(p + 16) ^ Internal::HashSecret[2], Internal::WyRead8(p + 24) ^ see1);
                    see2 = Internal::WyMix(Internal::WyRead8(p + 32) ^ Internal::HashSecret[3], Internal::WyRead8(p + 40) ^ see2);
                    p += 48;
                    i -= 48;
                }
                while (i > 48);
                seed ^= see1 ^ see2;
            }
            while (i > 16)
            {
                seed = Internal::WyMix(Internal::WyRead8(p) ^ Internal::HashSecret[1], Internal::WyRead8(p + 8) ^ seed);
                i -= 16;
                p += 16;
            }
            a = Internal::WyRead8(p + i - 16);
            b = Internal::WyRead8(p + i - 8);
        }

        return Internal::WyMix(Internal::HashSecret[1] ^ len, Internal::WyMix(a ^ Internal::HashSecret[1], b ^ seed));
    }


    [[nodiscard]] inline uint64_t DefaultHash(const void* data, const size_t len) noexcept
    {
        return wyhash(data, len, Internal::HashSecret[0], Internal::HashSecret);
    }


    [[nodiscard]] inline uint64_t DefaultHashWithSeed(const uint64_t seed, const void* data, const size_t len) noexcept
    {
        return wyhash(data, len, seed, Internal::HashSecret);
    }


    [[nodiscard]] inline uint64_t DefaultHashWithSeed(const uint64_t seed, const std::string_view str) noexcept
    {
        return DefaultHashWithSeed(seed, str.data(), str.length());
    }


    template<class T>
    inline constexpr std::string_view TypeName = Internal::TypeNameImpl<T>().value;


    template<class T>
    inline constexpr uint64_t TypeNameHash = CompileTimeHash(TypeName<T>.data(), TypeName<T>.length());


    inline void HashCombine(size_t& /* seed */) {}


    //! @brief Combine hashes of specified values with seed.
    //!
    //! @tparam Args  Types of values.
    //!
    //! @param seed Initial hash value to combine with.
    //! @param args The values to calculate hash of.
    template<class T, class... Args>
    void HashCombine(size_t& seed, const T& value, const Args&... args)
    {
        eastl::hash<T> hasher;
        seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        HashCombine(seed, args...);
    }


    template<class... TArgs>
    size_t HashAll(const TArgs&... args)
    {
        size_t seed = Internal::HashSecret[0];
        HashCombine(seed, args...);
        return seed;
    }


    struct Hasher final
    {
        explicit Hasher(const uint64_t seed = Internal::HashSecret[0])
            : m_hash(seed)
        {
        }

        ~Hasher() = default;

        Hasher(const Hasher&) = delete;
        Hasher& operator=(const Hasher&) = delete;
        Hasher(Hasher&&) = delete;
        Hasher& operator=(Hasher&&) = delete;

        Hasher& UpdateRaw(const uint64_t hash)
        {
            m_hash ^= hash + 0x9e3779b9 + (m_hash << 6) + (m_hash >> 2);
            return *this;
        }

        Hasher& UpdateRaw(const void* data, const size_t byteSize)
        {
            return UpdateRaw(DefaultHash(data, byteSize));
        }

        template<class T>
        Hasher& Update(const T& value)
        {
            return UpdateRaw(eastl::hash<T>()(value));
        }

        [[nodiscard]] uint64_t Finalize() const
        {
            return m_hash;
        }

    private:
        uint64_t m_hash;
    };
} // namespace FE
