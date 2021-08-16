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

    //! \brief Name of engine.
    inline constexpr const char* FerrumEngineName = u8"Ferrum3D";

#ifdef FE_DEBUG
    //! \brief True on debug builds.
    inline constexpr bool IsDebugBuild = true;
#else
    //! \brief True on debug builds.
    inline constexpr bool IsDebugBuild = false;
#endif

    //! \brief Engine version.
    inline constexpr struct
    {
        int Major = 0, Minor = 1, Patch = 0;
    } FerrumVersion;

    //! \brief Empty structure with no members.
    struct EmptyStruct
    {
    };

    //! \brief Position in source file.
    //!
    //! Represents source position as name of file and function and line number.
    struct SourcePosition
    {
        const char* FileName; //!< Name of source file.
        const char* FuncName; //!< Name of function.
                              //!< This can be a fuction signature depending on compiler.

        int LineNumber; //!< Number of line in source file.

        //! Create a source position with specified data.
        //!
        //! \note It's recommended to use macros: \ref FE_SRCPOS and \ref FE_STATIC_SRCPOS
        inline SourcePosition(const char* file, const char* func, int line) noexcept
            : FileName(file)
            , FuncName(func)
            , LineNumber(line)
        {
        }
    };

#define FE_SRCPOS() ::FE::SourcePosition(__FILE__, FE_FUNCSIG, __LINE__)
#define FE_STATIC_SRCPOS(name) static ::FE::SourcePosition name(__FILE__, FE_FUNCSIG, __LINE__)

    //! \internal
    namespace Internal
    {
        //! \brief A simple `std::string_view` wrapper
        //!
        //! This is useful for function signatures when compiling with MSVC. `std::string_view` is a template class
        //! (`std::basic_string_view< ... >`). It makes difficult to retrieve typename from a template function
        //! signature since it makes more than one template.
        struct SVWrapper
        {
            std::string_view value; //!< Actual value of the string view.
        };

        //! \brief Remove leading and trailing spaces from a string view.
        inline constexpr std::string_view TrimTypeName(std::string_view name)
        {
            name.remove_prefix(name.find_first_not_of(' '));
            name.remove_suffix(name.length() - name.find_last_not_of(' ') - 1);
            return name;
        }

        //! \brief Internal implementation of \ref FE::TypeName.
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
    //! \endinternal

    //! \brief Get name of a type as a compile-time constant.
    //!
    //! This implementation uses the `__PRETTY_FUNCTION__` hack to retrieve typename from a function signature
    //! at compile-time.
    template<class T>
    inline constexpr std::string_view TypeName()
    {
        return Internal::TypeNameImpl<T>().value;
    }

    //! \brief Align up an integer.
    //! \param [in] x     - Value to align.
    //! \param [in] align - Alignment to use.
    template<class T>
    inline constexpr T AlignUp(T x, T align)
    {
        return (x + (align - 1u)) & ~(align - 1u);
    }

    //! \brief Align up an integer.
    //! \param [in] x     - Value to align.
    //! \tparam A         - Alignment to use.
    template<UInt32 A, class T>
    inline constexpr T AlignUp(T x)
    {
        return (x + (A - 1)) & ~(A - 1);
    }

    //! \brief Align down an integer.
    //! \param [in] x     - Value to align.
    //! \param [in] align - Alignment to use.
    template<class T>
    inline constexpr T AlignDown(T x, T align)
    {
        return ((x) & ~(align - 1));
    }

    //! \brief Align down an integer.
    //! \param [in] x     - Value to align.
    //! \tparam A         - Alignment to use.
    template<UInt32 A, class T>
    inline constexpr T AlignDown(T x)
    {
        return ((x) & ~(A - 1));
    }

    //! TODO: move to math.
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

    //! TODO: move to math.
    inline constexpr char IntToHexChar(Int32 n)
    {
        return "0123456789ABCDEF"[n];
    }

#ifdef FE_COMPILER_MSVC

    //! TODO: move to math.
    UInt32 FE_FINLINE CountTrailingZeros(UInt32 value)
    {
        unsigned long tz = 0;
        return _BitScanForward(&tz, value) ? tz : 32;
    }

    //! TODO: move to math.
    UInt32 FE_FINLINE CountLeadingZeros(UInt32 value)
    {
        unsigned long lz = 0;
        return _BitScanReverse(&lz, value) ? 31 - lz : 32;
    }

#else

    //! TODO: move to math.
    UInt32 FE_FINLINE CountTrailingZeros(UInt32 value)
    {
        return __builtin_ctz(value);
    }

    //! TODO: move to math.
    UInt32 FE_FINLINE CountLeadingZeros(UInt32 value)
    {
        return __builtin_clz(value);
    }

#endif

#if FE_DEBUG
    //! \brief Assertion without loggers, used in modules on which loggers depend.
    //!
    //! If assertion fails this function will use \ref FE_DEBUGBREAK.
#    define FE_CORE_ASSERT(expression, msg)                                                                                      \
        do                                                                                                                       \
        {                                                                                                                        \
            if (!(expression))                                                                                                   \
            {                                                                                                                    \
                FE_DEBUGBREAK;                                                                                                   \
                (void)msg;                                                                                                       \
            }                                                                                                                    \
        }                                                                                                                        \
        while (0)
#else
    //! \brief Assertion without loggers, used in modules on which loggers depend.
    //!
    //! If assertion fails this function will use \ref FE_DEBUGBREAK.
#    define FE_CORE_ASSERT(expression, msg)                                                                                      \
        do                                                                                                                       \
        {                                                                                                                        \
            (void)expression;                                                                                                    \
            (void)msg;                                                                                                           \
        }                                                                                                                        \
        while (0)
#endif

    //! \brief Define bitwise operations on `enum`.
    //!
    //! The macro defines bitwise or, and, xor operators.
#define FE_ENUM_OPERATORS(Name)                                                                                                  \
    inline constexpr Name operator|(Name a, Name b)                                                                              \
    {                                                                                                                            \
        return Name(((std::underlying_type_t<Name>)a) | ((std::underlying_type_t<Name>)b));                                      \
    }                                                                                                                            \
    inline constexpr Name& operator|=(Name& a, Name b)                                                                           \
    {                                                                                                                            \
        return a = a | b;                                                                                                        \
    }                                                                                                                            \
    inline constexpr Name operator&(Name a, Name b)                                                                              \
    {                                                                                                                            \
        return Name(((std::underlying_type_t<Name>)a) & ((std::underlying_type_t<Name>)b));                                      \
    }                                                                                                                            \
    inline constexpr Name& operator&=(Name& a, Name b)                                                                           \
    {                                                                                                                            \
        return a = a & b;                                                                                                        \
    }                                                                                                                            \
    inline constexpr Name operator^(Name a, Name b)                                                                              \
    {                                                                                                                            \
        return Name(((std::underlying_type_t<Name>)a) ^ ((std::underlying_type_t<Name>)b));                                      \
    }                                                                                                                            \
    inline constexpr Name& operator^=(Name& a, Name b)                                                                           \
    {                                                                                                                            \
        return a = a ^ b;                                                                                                        \
    }
} // namespace FE
