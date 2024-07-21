#pragma once
#include <FeCore/Base/CompilerTraits.h>
#include <cstdint>
#include <string_view>

#if FE_COMPILER_MSVC
#    include <intrin.h>
#    pragma intrinsic(_umul128)
#endif

namespace FE
{
    FE_FINLINE constexpr uint64_t Multiply128(uint64_t x, uint64_t y, uint64_t* carry)
    {
        const uint64_t x0 = static_cast<uint32_t>(x), x1 = x >> 32;
        const uint64_t y0 = static_cast<uint32_t>(y), y1 = y >> 32;
        const uint64_t p11 = x1 * y1, p01 = x0 * y1;
        const uint64_t p10 = x1 * y0, p00 = x0 * y0;
        const uint64_t middle = p10 + (p00 >> 32) + (uint32_t)p01;
        *carry = p11 + (middle >> 32) + (p01 >> 32);
        return (middle << 32) | (uint32_t)p00;
    }


    namespace Internal
    {
        // Our hash functions are based on WyHash: https://github.com/wangyi-fudan/wyhash
        // WyHash is released into public domain.
        // Here is a gist with my compile-time implementation: https://gist.github.com/n-dub/56acb5aa3665fefd9dd6ca21c2748886
        //
        // The code below assumes little-endian and implements only the most basic functionality of the original
        // library: string hashes and uint64_t hashes.
        //


        constexpr uint64_t kDefaultSecret[] = {
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

        inline constexpr uint64_t WyRead3(const char* p, size_t k)
        {
            return ((static_cast<uint64_t>(p[0])) << 16) | ((static_cast<uint64_t>(p[k >> 1])) << 8) | p[k - 1];
        }

        inline constexpr uint64_t WyMix(uint64_t A, uint64_t B)
        {
#if FE_COMPILER_MSVC
            return Multiply128(A, B, &B) ^ B;
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
            A = Multiply128(A, B, &B);

            return WyMix(A ^ UINT64_C(0x2d358dccaa6c78a5), B ^ UINT64_C(0x8bb84b93962eacc9));
        }

        // This is useful for function signatures when compiling on Windows. `std::string_view` is a template class
        // (`std::basic_string_view< ... >`) which makes it difficult to retrieve typename from a template function
        // signature since it has multiple templates.
        struct SVWrapper final
        {
            std::string_view value;
        };

        //! \brief Remove leading and trailing spaces from a string view.
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


    inline constexpr uint64_t DefaultHash(const void* data, size_t len) noexcept
    {
        const char* p = static_cast<const char*>(data);
        uint64_t seed = Internal::kDefaultSecret[0], a = 0, b = 0;
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
                    seed = Internal::WyMix(Internal::WyRead8(p) ^ Internal::kDefaultSecret[1], Internal::WyRead8(p + 8) ^ seed);
                    see1 = Internal::WyMix(Internal::WyRead8(p + 16) ^ Internal::kDefaultSecret[2],
                                           Internal::WyRead8(p + 24) ^ see1);
                    see2 = Internal::WyMix(Internal::WyRead8(p + 32) ^ Internal::kDefaultSecret[3],
                                           Internal::WyRead8(p + 40) ^ see2);
                    p += 48;
                    i -= 48;
                }
                while (i > 48);
                seed ^= see1 ^ see2;
            }
            while (i > 16)
            {
                seed = Internal::WyMix(Internal::WyRead8(p) ^ Internal::kDefaultSecret[1], Internal::WyRead8(p + 8) ^ seed);
                i -= 16;
                p += 16;
            }
            a = Internal::WyRead8(p + i - 16);
            b = Internal::WyRead8(p + i - 8);
        }

        return Internal::WyMix(Internal::kDefaultSecret[1] ^ len, Internal::WyMix(a ^ Internal::kDefaultSecret[1], b ^ seed));
    }


    inline constexpr uint64_t DefaultHash(std::string_view str) noexcept
    {
        return DefaultHash(str.data(), str.length());
    }


    template<class T>
    inline constexpr std::string_view TypeName = Internal::TypeNameImpl<T>().value;


    template<class T>
    inline constexpr uint64_t TypeNameHash = DefaultHash(TypeName<T>);


    inline void HashCombine(size_t& /* seed */) {}


    //! \brief Combine hashes of specified values with seed.
    //!
    //! \tparam Args - Types of values.
    //!
    //! \param [in,out] seed - Initial hash value to combine with.
    //! \param [in]     args - The values to calculate hash of.
    template<typename T, typename... Args>
    inline void HashCombine(size_t& seed, const T& value, const Args&... args)
    {
        eastl::hash<T> hasher;
        seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        HashCombine(seed, args...);
    }
} // namespace FE
