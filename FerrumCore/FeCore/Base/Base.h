#pragma once
#include <EASTL/fixed_function.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/functional.h>
#include <EASTL/intrusive_list.h>
#include <EASTL/sort.h>
#include <EASTL/vector.h>
#include <FeCore/Base/Hash.h>
#include <FeCore/Base/Platform.h>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <intrin.h>
#include <mutex>
#include <string_view>
#include <tracy/Tracy.hpp>

namespace FE
{
    static_assert(sizeof(size_t) == sizeof(int64_t));

#ifdef FE_DEBUG
    //! \brief True on debug builds.
    inline constexpr bool IsDebugBuild = true;
#else
    //! \brief True on debug builds.
    inline constexpr bool IsDebugBuild = false;
#endif

    inline constexpr uint32_t InvalidIndex = static_cast<uint32_t>(-1);

    template<class TEnum>
    inline constexpr std::underlying_type_t<TEnum> DefaultErrorCode = std::numeric_limits<std::underlying_type_t<TEnum>>::min();

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

        int32_t LineNumber; //!< Number of line in source file.

        //! Create a source position with specified data.
        //!
        //! \note It's recommended to use macros: \ref FE_SRCPOS and \ref FE_STATIC_SRCPOS
        inline SourcePosition(const char* file, const char* func, int32_t line) noexcept
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
        return static_cast<T>((x + (align - 1u)) & ~(align - 1u));
    }

    //! \brief Align up a pointer.
    //!
    //! \param [in] x     - Value to align.
    //! \param [in] align - Alignment to use.
    template<class T>
    inline T* AlignUpPtr(const T* x, size_t align)
    {
        return reinterpret_cast<T*>(AlignUp(reinterpret_cast<size_t>(x), align));
    }

    //! \brief Align up an integer.
    //!
    //! \param [in] x     - Value to align.
    //! \tparam A         - Alignment to use.
    template<uint32_t A, class T>
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
    inline constexpr T* AlignDownPtr(const T* x, size_t align)
    {
        return reinterpret_cast<T*>(AlignDown(reinterpret_cast<size_t>(x), align));
    }

    //! \brief Align down an integer.
    //!
    //! \param [in] x     - Value to align.
    //! \tparam A         - Alignment to use.
    template<uint32_t A, class T>
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


    template<class T1, class T2>
    inline auto CeilDivide(T1 x, T2 y) -> std::enable_if_t<std::is_unsigned_v<T1> && std::is_unsigned_v<T2>, decltype(x / y)>
    {
        return (x + y - 1) / y;
    }


    template<class TTo, class TFrom>
    inline std::enable_if_t<std::is_default_constructible_v<TTo> && sizeof(TTo) == sizeof(TFrom), TTo> bit_cast(
        const TFrom& value)
    {
        TTo result;
        memcpy(&result, &value, sizeof(TTo));
        return result;
    }

    template<class T>
    inline constexpr auto enum_cast(T value) -> std::underlying_type_t<T>
    {
        return static_cast<std::underlying_type_t<T>>(value);
    }


    template<class T, class TValue, TValue TInvalidValue = std::numeric_limits<TValue>::max()>
    struct TypedHandle
    {
        using BaseType = TValue;

        TValue Value = TInvalidValue;

        inline void Reset() noexcept
        {
            Value = TInvalidValue;
        }

        inline bool IsValid() const noexcept
        {
            return Value != TInvalidValue;
        }

        inline explicit operator TValue() const noexcept
        {
            return Value;
        }

        inline explicit operator bool() const noexcept
        {
            return IsValid();
        }

        inline friend bool operator==(const TypedHandle& lhs, const TypedHandle& rhs)
        {
            return lhs.Value == rhs.Value;
        }

        inline friend bool operator!=(const TypedHandle& lhs, const TypedHandle& rhs)
        {
            return lhs.Value != rhs.Value;
        }
    };


    namespace Memory::Internal
    {
        class EASTLPolymorphicAllocator final
        {
            std::pmr::memory_resource* m_pMemoryResource = nullptr;

        public:
            EASTLPolymorphicAllocator(std::pmr::memory_resource* pMemoryResource = nullptr)
                : m_pMemoryResource(pMemoryResource)
            {
                if (m_pMemoryResource == nullptr)
                    m_pMemoryResource = std::pmr::get_default_resource();
            }

            EASTLPolymorphicAllocator(const char*, std::pmr::memory_resource* pMemoryResource = nullptr)
                : m_pMemoryResource(pMemoryResource)
            {
                if (m_pMemoryResource == nullptr)
                    m_pMemoryResource = std::pmr::get_default_resource();
            }

            inline void* allocate(size_t n, int = 0)
            {
                return m_pMemoryResource->allocate(n);
            }

            inline void* allocate(size_t n, size_t alignment, size_t, int = 0)
            {
                return m_pMemoryResource->allocate(n, alignment);
            }

            inline void deallocate(void* p, size_t size)
            {
                m_pMemoryResource->deallocate(p, size);
            }

            inline const char* get_name() const
            {
                return "";
            }

            inline void set_name(const char*) {}

            friend bool operator==(const EASTLPolymorphicAllocator&, const EASTLPolymorphicAllocator&)
            {
                return true;
            }

            friend bool operator!=(const EASTLPolymorphicAllocator&, const EASTLPolymorphicAllocator&)
            {
                return false;
            }
        };
    } // namespace Memory::Internal


    namespace festd
    {
        namespace pmr
        {
            template<class T>
            using vector = eastl::vector<T, Memory::Internal::EASTLPolymorphicAllocator>;
        }


        template<class T, class TAllocator = Memory::Internal::EASTLDefaultAllocator>
        using vector = eastl::vector<T, TAllocator>;

        template<class T, uint32_t TSize>
        using fixed_vector = eastl::fixed_vector<T, TSize, false>;

        using intrusive_list_node = eastl::intrusive_list_node;

        template<class T = intrusive_list_node>
        using intrusive_list = eastl::intrusive_list<T>;

        template<int32_t TByteSize, class TFunc>
        using fixed_function = eastl::fixed_function<TByteSize, TFunc>;
    } // namespace festd


//! \brief Typed alloca() wrapper.
#define FE_StackAlloc(type, arraySize) static_cast<type*>(alloca(sizeof(type) * arraySize))


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
#    define FE_CORE_ASSERT(expression, msg)                                                                                      \
        do                                                                                                                       \
        {                                                                                                                        \
            assert((expression) && (msg));                                                                                       \
        }                                                                                                                        \
        while (0)
#else
    //! \brief Assertion without loggers, used in modules on which loggers depend.
#    define FE_CORE_ASSERT(expression, msg)                                                                                      \
        do                                                                                                                       \
        {                                                                                                                        \
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
