#pragma once
#include <FeCore/Base/Base.h>
#include <array>
#include <cctype>
#include <string_view>

#if FE_PLATFORM_WINDOWS
#    include <guiddef.h>
#else
typedef struct _GUID
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;
#endif

namespace FE
{
    //! @brief A struct to work with UUIDs.
    struct alignas(8) UUID
    {
        std::array<uint8_t, 16> Data{};

        UUID() = default;

        UUID(const UUID& other) noexcept
        {
            memcpy(Data.data(), other.Data.data(), 16);
        }

        UUID& operator=(const UUID& other) noexcept
        {
            memcpy(Data.data(), other.Data.data(), 16);
            return *this;
        }

        static UUID FromGUID(const GUID& value) noexcept
        {
            UUID result;
            memcpy(result.Data.data(), &value, 16);
            *reinterpret_cast<uint32_t*>(&result.Data[0]) = FE_BYTE_SWAP_UINT32(*reinterpret_cast<uint32_t*>(&result.Data[0]));
            *reinterpret_cast<uint16_t*>(&result.Data[4]) = FE_BYTE_SWAP_UINT16(*reinterpret_cast<uint16_t*>(&result.Data[4]));
            *reinterpret_cast<uint16_t*>(&result.Data[6]) = FE_BYTE_SWAP_UINT16(*reinterpret_cast<uint16_t*>(&result.Data[6]));
            return result;
        }

        //! @brief Parse a UUID from a string in form `"xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"`.
        explicit UUID(const char* str) noexcept
        {
            const bool result = TryParse(str, *this, false);
            FE_CORE_ASSERT(result, "Invalid format");
        }

        static bool TryParse(const char* str, UUID& result, bool assertLength = true)
        {
            static char digits[] = "0123456789ABCDEF";
            constexpr auto getValue = [](char c) {
                return static_cast<uint8_t>(std::find(digits, digits + 16, std::toupper(c)) - digits);
            };

            size_t idx = 0;
            auto parse = [&](int32_t n) -> bool {
                for (int32_t i = 0; i < n; ++i)
                {
                    auto v1 = getValue(*str++);
                    if (v1 >= 16)
                        return false;

                    auto v2 = getValue(*str++);
                    if (v2 >= 16)
                        return false;

                    result.Data[idx] = v1 << 4;
                    result.Data[idx++] |= v2 & 0x0F;
                }

                return true;
            };

            // clang-format off
            if (!parse(4))  return false;
            if (*str++ != '-') return false;
            if (!parse(2))  return false;
            if (*str++ != '-') return false;
            if (!parse(2))  return false;
            if (*str++ != '-') return false;
            if (!parse(2))  return false;
            if (*str++ != '-') return false;
            if (!parse(6))  return false;
            // clang-format on

            if (*str != '\0' && assertLength)
                return false;

            return true;
        }
    };

    inline bool operator<(const UUID& lhs, const UUID& rhs)
    {
        auto* l = reinterpret_cast<const uint64_t*>(lhs.Data.data());
        auto* r = reinterpret_cast<const uint64_t*>(rhs.Data.data());

        if (l[0] != r[0])
        {
            return l[0] < r[0];
        }

        return l[1] < r[1];
    }

    inline bool operator==(const UUID& lhs, const UUID& rhs)
    {
        return std::equal(lhs.Data.begin(), lhs.Data.end(), rhs.Data.begin());
    }

    inline bool operator!=(const UUID& lhs, const UUID& rhs)
    {
        return !(lhs == rhs);
    }
} // namespace FE

template<>
struct eastl::hash<FE::UUID>
{
    size_t operator()(const FE::UUID& value) const noexcept
    {
        return FE::DefaultHash(value.Data.data(), value.Data.size());
    }
};
