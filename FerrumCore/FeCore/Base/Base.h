#pragma once
#include <EASTL/functional.h>
#include <EASTL/vector.h>
#include <FeCore/Base/Hash.h>
#include <FeCore/Base/Platform.h>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <intrin.h>
#include <mutex>
#include <string_view>

namespace FE
{
    using Int8 = int8_t;
    using Int16 = int16_t;
    using Int32 = int32_t;
    using Int64 = int64_t;

    using UInt8 = uint8_t;
    using UInt16 = uint16_t;
    using UInt32 = uint32_t;
    using UInt64 = uint64_t;

    using USize = UInt64;
    using SSize = Int64;

    static_assert(sizeof(size_t) == sizeof(Int64));

#ifdef FE_DEBUG
    //! \brief True on debug builds.
    inline constexpr bool IsDebugBuild = true;
#else
    //! \brief True on debug builds.
    inline constexpr bool IsDebugBuild = false;
#endif

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
                              //!< This can be a function signature depending on compiler.

        Int32 LineNumber; //!< Number of line in source file.

        //! Create a source position with specified data.
        //!
        //! \note It's recommended to use macros: \ref FE_SRCPOS and \ref FE_STATIC_SRCPOS
        inline SourcePosition(const char* file, const char* func, Int32 line) noexcept
            : FileName(file)
            , FuncName(func)
            , LineNumber(line)
        {
        }
    };


    //! \brief Align up an integer.
    //!
    //! \param [in] x     - Value to align.
    //! \param [in] align - Alignment to use.
    template<class T, class U = T>
    inline T AlignUp(T x, U align)
    {
        return (x + (align - 1u)) & ~(align - 1u);
    }

    //! \brief Align up a pointer.
    //!
    //! \param [in] x     - Value to align.
    //! \param [in] align - Alignment to use.
    template<class T>
    inline T* AlignUpPtr(const T* x, USize align)
    {
        return reinterpret_cast<T*>(AlignUp(reinterpret_cast<USize>(x), align));
    }

    //! \brief Align up an integer.
    //!
    //! \param [in] x     - Value to align.
    //! \tparam A         - Alignment to use.
    template<UInt32 A, class T>
    inline constexpr T AlignUp(T x)
    {
        return (x + (A - 1)) & ~(A - 1);
    }

    //! \brief Align down an integer.
    //!
    //! \param [in] x     - Value to align.
    //! \param [in] align - Alignment to use.
    template<class T, class U = T>
    inline T AlignDown(T x, U align)
    {
        return (x & ~(align - 1));
    }

    //! \brief Align down a pointer.
    //!
    //! \param [in] x     - Value to align.
    //! \param [in] align - Alignment to use.
    template<class T>
    inline constexpr T* AlignDownPtr(const T* x, USize align)
    {
        return reinterpret_cast<T*>(AlignDown(reinterpret_cast<USize>(x), align));
    }

    //! \brief Align down an integer.
    //!
    //! \param [in] x     - Value to align.
    //! \tparam A         - Alignment to use.
    template<UInt32 A, class T>
    inline constexpr T AlignDown(T x)
    {
        return (x & ~(A - 1));
    }

    //! \brief Create a bitmask.
    //!
    //! \param [in] bitCount  - The number of ones in the created mask.
    //! \param [in] leftShift - The number of zeros to the right of the created mask.
    template<class T>
    inline constexpr T MakeMask(T bitCount, T leftShift)
    {
        auto typeBitCount = sizeof(T) * 8;
        auto mask = bitCount == typeBitCount ? static_cast<T>(-1) : ((1 << bitCount) - 1);
        return static_cast<T>(mask << leftShift);
    }

    //! \brief Define std::hash<> for a type.
#define FE_MAKE_HASHABLE(TypeName, Template, ...)                                                                                \
    template<Template>                                                                                                           \
    struct eastl::hash<TypeName>                                                                                                 \
    {                                                                                                                            \
        inline size_t operator()(const TypeName& value) const noexcept                                                           \
        {                                                                                                                        \
            size_t seed = 0;                                                                                                     \
            ::FE::HashCombine(seed, __VA_ARGS__);                                                                                \
            return seed;                                                                                                         \
        }                                                                                                                        \
    };

#if FE_DEBUG
    //! \brief Assertion without loggers, used in modules on which loggers depend.
    //!
    //! If assertion fails this function will use \ref FE_DEBUGBREAK.
#    define FE_CORE_ASSERT(expression, msg)                                                                                      \
        do                                                                                                                       \
        {                                                                                                                        \
            assert((expression) && (msg));                                                                                       \
        }                                                                                                                        \
        while (0)
#else
    //! \brief Assertion without loggers, used in modules on which loggers depend.
    //!
    //! If assertion fails this function will use \ref FE_DEBUGBREAK.
#    define FE_CORE_ASSERT(expression, msg)                                                                                      \
        do                                                                                                                       \
        {                                                                                                                        \
            (void)(expression);                                                                                                  \
            (void)(msg);                                                                                                         \
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
