#pragma once
#include <FeCore/Base/BaseMath.h>
#include <FeCore/Base/Hash.h>
#include <FeCore/Base/Platform.h>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <festd/base.h>
#include <festd/span.h>
#include <intrin.h>
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
    inline constexpr uint32_t kInvalidIndex = static_cast<uint32_t>(-1);

    template<class TEnum>
    inline constexpr std::underlying_type_t<TEnum> kDefaultErrorCode = std::numeric_limits<std::underlying_type_t<TEnum>>::min();


    //! @brief Empty structure with no members.
    struct EmptyStruct final
    {
    };


    //! @brief Location in source file.
    struct SourceLocation final
    {
        const char* m_fileName = "";
        uint32_t m_lineNumber = 0;

        SourceLocation() = default;

        static constexpr SourceLocation Current(const char* file = __builtin_FILE(), uint32_t line = __builtin_LINE()) noexcept
        {
            return { file, line };
        }

    private:
        constexpr SourceLocation(const char* file, uint32_t line) noexcept
            : m_fileName(file)
            , m_lineNumber(line)
        {
        }
    };


    template<class T, class TValue, TValue TInvalidValue = std::numeric_limits<TValue>::max()>
    struct TypedHandle
    {
        using BaseType = TValue;

        TValue m_value = TInvalidValue;

        void Reset()
        {
            m_value = TInvalidValue;
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_value != TInvalidValue;
        }

        explicit operator TValue() const
        {
            return m_value;
        }

        explicit operator bool() const
        {
            return IsValid();
        }

        friend bool operator==(TypedHandle lhs, TypedHandle rhs)
        {
            return lhs.m_value == rhs.m_value;
        }

        friend bool operator!=(TypedHandle lhs, TypedHandle rhs)
        {
            return lhs.m_value != rhs.m_value;
        }

    private:
        TypedHandle() = default;

        friend T;
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

            void* allocate(size_t n, int = 0)
            {
                return m_pMemoryResource->allocate(n);
            }

            void* allocate(size_t n, size_t alignment, size_t, int = 0)
            {
                return m_pMemoryResource->allocate(n, alignment);
            }

            void deallocate(void* p, size_t size)
            {
                m_pMemoryResource->deallocate(p, size);
            }

            const char* get_name() const
            {
                return "";
            }

            void set_name(const char*) {}

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

    namespace Memory
    {
        inline constexpr size_t kDefaultAlignment = 16;
        inline constexpr size_t kCacheLineSize = 64;


        template<class T>
        void Copy(festd::span<const T> source, festd::span<T> destination)
        {
            FE_CORE_ASSERT(source.size() == destination.size(), "Size mismatch");
            eastl::copy(source.begin(), source.end(), destination.begin());
        }


        template<class T>
        void Copy(festd::span<T> source, festd::span<T> destination)
        {
            FE_CORE_ASSERT(source.size() == destination.size(), "Size mismatch");
            eastl::copy(source.begin(), source.end(), destination.begin());
        }


        inline festd::span<const std::byte> MakeByteSpan(const char* str, uint32_t length = 0)
        {
            return { reinterpret_cast<const std::byte*>(str), length ? length : static_cast<uint32_t>(__builtin_strlen(str)) };
        }
    } // namespace Memory


//! @brief Typed alloca() wrapper.
#define FE_StackAlloc(type, arraySize) static_cast<type*>(alloca(sizeof(type) * (arraySize)))


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
} // namespace FE
