#pragma once
#include <EASTL/fixed_function.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/functional.h>
#include <EASTL/intrusive_list.h>
#include <EASTL/sort.h>
#include <EASTL/span.h>
#include <EASTL/vector.h>
#include <FeCore/Base/BaseMath.h>
#include <FeCore/Base/Hash.h>
#include <FeCore/Base/Platform.h>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <intrin.h>
#include <mutex>
#include <string_view>
#include <tracy/Tracy.hpp>

#if FE_DEBUG
//! @brief Assertion without loggers, used in modules on which loggers depend.
#    define FE_CORE_ASSERT(expression, msg)                                                                                      \
        do                                                                                                                       \
        {                                                                                                                        \
            assert((expression) && (msg));                                                                                       \
        }                                                                                                                        \
        while (0)
#else
//! @brief Assertion without loggers, used in modules on which loggers depend.
#    define FE_CORE_ASSERT(expression, msg)                                                                                      \
        do                                                                                                                       \
        {                                                                                                                        \
        }                                                                                                                        \
        while (0)
#endif

namespace FE
{
    static_assert(sizeof(size_t) == sizeof(int64_t));

#ifdef FE_DEBUG
    //! @brief True on debug builds.
    inline constexpr bool IsDebugBuild = true;
#else
    //! @brief True on debug builds.
    inline constexpr bool IsDebugBuild = false;
#endif

    inline constexpr uint32_t kInvalidIndex = static_cast<uint32_t>(-1);

    template<class TEnum>
    inline constexpr std::underlying_type_t<TEnum> DefaultErrorCode = std::numeric_limits<std::underlying_type_t<TEnum>>::min();

    //! @brief Empty structure with no members.
    struct EmptyStruct
    {
    };

    //! @brief Location in source file.
    struct SourceLocation
    {
        const char* FileName = "";
        uint32_t LineNumber = 0;

        inline SourceLocation() = default;

        inline static constexpr SourceLocation Current(const char* file = __builtin_FILE(),
                                                       uint32_t line = __builtin_LINE()) noexcept
        {
            return SourceLocation(file, line);
        }

    private:
        inline constexpr SourceLocation(const char* file, uint32_t line) noexcept
            : FileName(file)
            , LineNumber(line)
        {
        }
    };


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

        template<class T, uint32_t TSize>
        using fixed_vector = eastl::fixed_vector<T, TSize, false>;

        inline constexpr size_t dynamic_extent = static_cast<size_t>(-1);

        using eastl::fixed_function;
        using eastl::intrusive_list;
        using eastl::intrusive_list_node;
        using eastl::span;
        using eastl::vector;
    } // namespace festd


    namespace Memory
    {
        inline constexpr size_t kDefaultAlignment = 16;
        inline constexpr size_t kCacheLineSize = 64;


        template<class T>
        inline void Copy(festd::span<const T> source, festd::span<T> destination)
        {
            FE_CORE_ASSERT(source.size() == destination.size(), "Size mismatch");
            eastl::copy(source.begin(), source.end(), destination.begin());
        }


        template<class T>
        inline void Copy(festd::span<T> source, festd::span<T> destination)
        {
            FE_CORE_ASSERT(source.size() == destination.size(), "Size mismatch");
            eastl::copy(source.begin(), source.end(), destination.begin());
        }


        inline constexpr festd::span<const std::byte> MakeByteSpan(const char* str, uint32_t length = 0)
        {
            return { reinterpret_cast<const std::byte*>(str), length ? length : (uint32_t)__builtin_strlen(str) };
        }
    } // namespace Memory


//! @brief Typed alloca() wrapper.
#define FE_StackAlloc(type, arraySize) static_cast<type*>(alloca(sizeof(type) * arraySize))


    //! @brief Define std::hash<> for a type.
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

    //! @brief Define bitwise operations on `enum`.
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
