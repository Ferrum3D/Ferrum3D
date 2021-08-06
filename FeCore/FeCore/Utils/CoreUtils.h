#pragma once
#include <FeCore/Utils/Platform.h>
#include <atomic>
#include <cstdint>
#include <intrin.h>
#include <string_view>

namespace FE
{
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

#define FE_SRCPOS() ::FE::SourcePosition(__FILE__, FE_FUNCNAME, __LINE__)
#define FE_STATIC_SRCPOS(name) static ::FE::SourcePosition name(__FILE__, FE_FUNCNAME, __LINE__)

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
#ifdef _MSC_VER
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

    template<class T, class Func>
    inline void FeAtomicBitOp(std::atomic<T>& a, uint8_t n, Func bitOp)
    {
        static_assert(std::is_integral<T>(), "T must be integral");

        T val = a.load(), res = bitOp(val, n);
        while (!a.compare_exchange_weak(val, res))
            ;
    }

    template<class T>
    inline constexpr T FeMakeAlignment(T x, T align)
    {
        return (x + (align - 1u)) & ~(align - 1u);
    }

    template<uint32_t A, class T>
    inline constexpr T FeMakeAlignment(T x)
    {
        return (x + (A - 1)) & ~(A - 1);
    }

    inline constexpr uint32_t FeNextPowerOf2(uint32_t v)
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

    inline constexpr char IntToHexChar(int n)
    {
        return "0123456789ABCDEF"[n];
    }

    inline constexpr auto FeSetBit = [](auto val, unsigned n) {
        return val | (1 << n);
    };
    inline constexpr auto FeResetBit = [](auto val, unsigned n) {
        return val & ~(1 << n);
    };
    inline constexpr auto FeXorBit = [](auto val, unsigned n) {
        return val ^ (1 << n);
    };

#ifdef _MSC_VER

    uint32_t inline FeCountTrailingZeros(uint32_t value)
    {
        unsigned long tz = 0;
        return _BitScanForward(&tz, value) ? tz : 32;
    }

    uint32_t inline FeCountLeadingZeros(uint32_t value)
    {
        unsigned long lz = 0;
        return _BitScanReverse(&lz, value) ? 31 - lz : 32;
    }

#else

    uint32_t inline FeCountTrailingZeros(uint32_t value)
    {
        return __builtin_ctz(value);
    }

    uint32_t inline FeCountLeadingZeros(uint32_t value)
    {
        return __builtin_clz(value);
    }

#endif

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

#define FE_ENUM(_name) FE_TYPED_ENUM(_name, int)

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
