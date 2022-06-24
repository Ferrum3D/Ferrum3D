#pragma once
#include <FeCore/Base/Base.h>
#include <array>
#include <cctype>
#include <string_view>

namespace FE
{
    //! \brief A struct to work with UUIDs.
    struct alignas(16) UUID
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

        //! \brief Parse a UUID from a string in form `"xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"`.
        inline explicit UUID(const char* str)
        {
            static char digits[]    = "0123456789ABCDEF";
            constexpr auto getValue = [](char c) {
                return static_cast<UInt8>(std::find(digits, digits + 16, std::toupper(c)) - digits);
            };

            size_t idx = 0;
            auto parse = [&](Int32 n) {
                for (Int32 i = 0; i < n; ++i)
                {
                    Data[idx] = getValue(*str++) << 4;
                    Data[idx++] |= getValue(*str++) & 0x0F;
                }
            };

            parse(4);
            FE_CORE_ASSERT(*str++ == '-', "Invalid format");
            parse(2);
            FE_CORE_ASSERT(*str++ == '-', "Invalid format");
            parse(2);
            FE_CORE_ASSERT(*str++ == '-', "Invalid format");
            parse(2);
            FE_CORE_ASSERT(*str++ == '-', "Invalid format");
            parse(6);
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
