#pragma once
#include <FeCore/Base/Base.h>
#include <array>
#include <cctype>
#include <string_view>

#if FE_WINDOWS
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
    //! \brief A struct to work with UUIDs.
    struct alignas(8) UUID
    {
        std::array<UInt8, 16> Data{};

        inline UUID() = default;

        inline UUID(const UUID& other) noexcept
        {
            memcpy(Data.data(), other.Data.data(), 16);
        }

        inline UUID& operator=(const UUID& other) noexcept
        {
            memcpy(Data.data(), other.Data.data(), 16);
            return *this;
        }

        inline static UUID FromGUID(const GUID& value) noexcept
        {
            UUID result;
            memcpy(result.Data.data(), &value, 16);
            *reinterpret_cast<UInt32*>(&result.Data[0]) = FE_BYTE_SWAP_UINT32(*reinterpret_cast<UInt32*>(&result.Data[0]));
            *reinterpret_cast<UInt16*>(&result.Data[4]) = FE_BYTE_SWAP_UINT16(*reinterpret_cast<UInt16*>(&result.Data[4]));
            *reinterpret_cast<UInt16*>(&result.Data[6]) = FE_BYTE_SWAP_UINT16(*reinterpret_cast<UInt16*>(&result.Data[6]));
            return result;
        }

        //! \brief Parse a UUID from a string in form `"xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"`.
        inline explicit UUID(const char* str) noexcept
        {
            FE_CORE_ASSERT(TryParse(str, *this), "Invalid format");
        }

        inline static bool TryParse(const char* str, UUID& result, bool assertLength = true)
        {
            static char digits[]    = "0123456789ABCDEF";
            constexpr auto getValue = [](char c) {
                return static_cast<UInt8>(std::find(digits, digits + 16, std::toupper(c)) - digits);
            };

            USize idx  = 0;
            auto parse = [&](Int32 n) -> bool {
                for (Int32 i = 0; i < n; ++i)
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
            if (!parse(4))       return false;
            if (*str++ != '-')   return false;
            if (!parse(2))       return false;
            if (*str++ != '-')   return false;
            if (!parse(2))       return false;
            if (*str++ != '-')   return false;
            if (!parse(2))       return false;
            if (*str++ != '-')   return false;
            if (!parse(6))       return false;
            // clang-format on

            if (*str != '\0' && assertLength)
                return false;

            return true;
        }
    };

    inline bool operator==(const UUID& lhs, const UUID& rhs)
    {
        return std::equal(lhs.Data.begin(), lhs.Data.end(), rhs.Data.begin());
    }

    inline bool operator!=(const UUID& lhs, const UUID& rhs)
    {
        return !(lhs == rhs);
    }
} // namespace FE

namespace std
{
    // TODO: Get rid of std::hash, we need compile-time hash calculation.

    //!\ brief Calculate hash of a \ref FE::UUID.
    template<>
    struct hash<FE::UUID>
    {
        inline size_t operator()(const FE::UUID& value) const noexcept
        {
            return hash<string_view>{}(string_view(reinterpret_cast<const char*>(value.Data.data()), value.Data.size()));
        }
    };
} // namespace std
