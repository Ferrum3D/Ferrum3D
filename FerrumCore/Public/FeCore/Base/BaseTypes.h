#pragma once
#include <FeCore/Base/CompilerTraits.h>
#include <FeCore/Base/PlatformTraits.h>

namespace FE
{
    namespace Constants
    {
        inline constexpr float PI = 3.14159265358979323f;

        inline constexpr float kEpsilon = 1e-6f;
        inline constexpr float kInfinity = std::numeric_limits<float>::infinity();
        inline constexpr float kNan = std::numeric_limits<float>::quiet_NaN();

        inline constexpr float kMaxFloat = FLT_MAX;
        inline constexpr float kMinFloat = FLT_MIN;

        inline constexpr int8_t kMinI8 = INT8_MIN;
        inline constexpr int16_t kMinI16 = INT16_MIN;
        inline constexpr int32_t kMinI32 = INT32_MIN;
        inline constexpr int64_t kMinI64 = INT64_MIN;

        inline constexpr int8_t kMaxI8 = INT8_MAX;
        inline constexpr int16_t kMaxI16 = INT16_MAX;
        inline constexpr int32_t kMaxI32 = INT32_MAX;
        inline constexpr int64_t kMaxI64 = INT64_MAX;

        inline constexpr uint8_t kMaxU8 = UINT8_MAX;
        inline constexpr uint16_t kMaxU16 = UINT16_MAX;
        inline constexpr uint32_t kMaxU32 = UINT32_MAX;
        inline constexpr uint64_t kMaxU64 = UINT64_MAX;

        template<class T>
        inline constexpr T kMaxValue = std::numeric_limits<T>::max();

        template<class T>
        inline constexpr T kMinValue = std::numeric_limits<T>::min();
    } // namespace Constants


    //! @brief Invalid index value.
    inline constexpr uint32_t kInvalidIndex = Constants::kMaxU32;


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
        constexpr SourceLocation(const char* file, const uint32_t line) noexcept
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

        friend bool operator==(const TypedHandle lhs, const TypedHandle rhs)
        {
            return lhs.m_value == rhs.m_value;
        }

        friend bool operator!=(const TypedHandle lhs, const TypedHandle rhs)
        {
            return lhs.m_value != rhs.m_value;
        }

    private:
        TypedHandle() = default;

        friend T;
    };


    namespace Memory::Internal
    {
        struct EASTLPolymorphicAllocator final
        {
            EASTLPolymorphicAllocator(std::pmr::memory_resource* pMemoryResource = nullptr)
                : m_memoryResource(pMemoryResource)
            {
                if (m_memoryResource == nullptr)
                    m_memoryResource = std::pmr::get_default_resource();
            }

            EASTLPolymorphicAllocator(const char*, std::pmr::memory_resource* pMemoryResource = nullptr)
                : m_memoryResource(pMemoryResource)
            {
                if (m_memoryResource == nullptr)
                    m_memoryResource = std::pmr::get_default_resource();
            }

            void* allocate(size_t n, int = 0)
            {
                return m_memoryResource->allocate(n);
            }

            void* allocate(size_t n, size_t alignment, size_t, int = 0)
            {
                return m_memoryResource->allocate(n, alignment);
            }

            void deallocate(void* p, size_t size)
            {
                m_memoryResource->deallocate(p, size);
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

        private:
            std::pmr::memory_resource* m_memoryResource = nullptr;
        };
    } // namespace Memory::Internal


    namespace Memory
    {
        inline constexpr size_t kDefaultAlignment = 16;
        inline constexpr size_t kCacheLineSize = 64;
    } // namespace Memory
} // namespace FE
