#pragma once
#include <FeCore/Base/Platform.h>
#include <atomic>
#include <cstdint>
#include <intrin.h>
#include <string_view>

namespace FE
{
    using Int8  = int8_t;
    using Int16 = int16_t;
    using Int32 = int32_t;
    using Int64 = int64_t;

    using UInt8  = uint8_t;
    using UInt16 = uint16_t;
    using UInt32 = uint32_t;
    using UInt64 = uint64_t;

    using Float32 = float;
    using Float64 = double;

    /**
     * @brief Name of the engine
     */
    inline constexpr const char* FerrumEngineName = "Ferrum3D";

#ifdef FE_DEBUG
    inline constexpr bool IsDebugBuild = true;
#else
    inline constexpr bool IsDebugBuild = false;
#endif

    /**
     * @brief Engine version
     */
    inline constexpr struct
    {
        int Major = 0, Minor = 1, Patch = 0;
    } Ferrum3DVersion;

    struct EmptyStruct
    {
    };

    struct SourcePosition
    {
        const char* FileName;
        const char* FuncName;
        int LineNumber;

        inline SourcePosition(const char* file, const char* func, int line) noexcept
            : FileName(file)
            , FuncName(func)
            , LineNumber(line)
        {
        }
    };

#define FE_SRCPOS() ::FE::SourcePosition(__FILE__, FE_FUNCSIG, __LINE__)
#define FE_STATIC_SRCPOS(name) static ::FE::SourcePosition name(__FILE__, FE_FUNCSIG, __LINE__)

    namespace Internal
    {
        struct SVWrapper
        {
            std::string_view value;
        };

        inline constexpr std::string_view TrimTypeName(std::string_view name)
        {
            name.remove_prefix(name.find_first_not_of(" "));
            name.remove_suffix(name.length() - name.find_last_not_of(" ") - 1);
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
            fn.remove_prefix(fn.find_first_of("=") + 1);
            fn.remove_suffix(fn.length() - fn.find_last_of("]"));
#endif
            return SVWrapper{ TrimTypeName(fn) };
        }
    } // namespace Internal

    template<class T>
    inline constexpr std::string_view TypeName()
    {
        return Internal::TypeNameImpl<T>().value;
    }

    template<class T>
    inline size_t TypeHash()
    {
        return std::hash<std::string_view>{}(Internal::TypeNameImpl<T>().value);
    }

    template<class T>
    inline constexpr T AlignUp(T x, T align)
    {
        return (x + (align - 1u)) & ~(align - 1u);
    }

    template<UInt32 A, class T>
    inline constexpr T AlignUp(T x)
    {
        return (x + (A - 1)) & ~(A - 1);
    }

    template<class T>
    inline constexpr T AlignDown(T x, T align)
    {
        return ((x) & ~(align - 1));
    }

    template<UInt32 A, class T>
    inline constexpr T AlignDown(T x)
    {
        return ((x) & ~(A - 1));
    }

    inline constexpr UInt32 NextPowerOf2(UInt32 v)
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;
        return v;
    }

    inline constexpr char IntToHexChar(Int32 n)
    {
        return "0123456789ABCDEF"[n];
    }

#ifdef FE_COMPILER_MSVC

    UInt32 FE_FINLINE CountTrailingZeros(UInt32 value)
    {
        unsigned long tz = 0;
        return _BitScanForward(&tz, value) ? tz : 32;
    }

    UInt32 FE_FINLINE CountLeadingZeros(UInt32 value)
    {
        unsigned long lz = 0;
        return _BitScanReverse(&lz, value) ? 31 - lz : 32;
    }

#else

    UInt32 FE_FINLINE CountTrailingZeros(UInt32 value)
    {
        return __builtin_ctz(value);
    }

    UInt32 FE_FINLINE CountLeadingZeros(UInt32 value)
    {
        return __builtin_clz(value);
    }

#endif

    // assertion without loggers, used in modules on which loggers depend
#define FE_CORE_ASSERT(expression, msg)                                                                                          \
    do                                                                                                                           \
    {                                                                                                                            \
        if (!(expression))                                                                                                       \
        {                                                                                                                        \
            FE_DEBUGBREAK;                                                                                                       \
            (void)msg;                                                                                                           \
        }                                                                                                                        \
    }                                                                                                                            \
    while (0)

#define FE_TYPED_ENUM(_name, _type)                                                                                              \
    enum class _name : _type;                                                                                                    \
    inline _name operator|(_name a, _name b)                                                                                     \
    {                                                                                                                            \
        return _name(_type(a) | _type(b));                                                                                       \
    }                                                                                                                            \
    inline _name operator&(_name a, _name b)                                                                                     \
    {                                                                                                                            \
        return _name(_type(a) & _type(b));                                                                                       \
    }                                                                                                                            \
    inline _name& operator|=(_name& a, _name b)                                                                                  \
    {                                                                                                                            \
        return a = a | b;                                                                                                        \
    }                                                                                                                            \
    inline _name& operator&=(_name& a, _name b)                                                                                  \
    {                                                                                                                            \
        return a = a & b;                                                                                                        \
    }                                                                                                                            \
                                                                                                                                 \
    inline _name operator|(_name a, _type b)                                                                                     \
    {                                                                                                                            \
        return _name(_type(a) | b);                                                                                              \
    }                                                                                                                            \
    inline _name operator&(_name a, _type b)                                                                                     \
    {                                                                                                                            \
        return _name(_type(a) & b);                                                                                              \
    }                                                                                                                            \
    inline _name& operator|=(_name& a, _type b)                                                                                  \
    {                                                                                                                            \
        return a = a | b;                                                                                                        \
    }                                                                                                                            \
    inline _name& operator&=(_name& a, _type b)                                                                                  \
    {                                                                                                                            \
        return a = a & b;                                                                                                        \
    }                                                                                                                            \
                                                                                                                                 \
    enum class _name : _type

#define FE_ENUM(_name) FE_TYPED_ENUM(_name, Int32)

#define FE_ENUM_TO_STR(_name)                                                                                                    \
    inline std::ostream& operator<<(std::ostream& stream, _name parameter)                                                       \
    {                                                                                                                            \
        switch (parameter)

#define FE_ENUM_STR_CASE(_name)                                                                                                  \
    case _name:                                                                                                                  \
        return stream << #_name
#define FE_ENUM_STR_CASE_DEF(_name)                                                                                              \
    default:                                                                                                                     \
        return stream << #_name << "::{Unknown}";                                                                                \
        }
} // namespace FE
