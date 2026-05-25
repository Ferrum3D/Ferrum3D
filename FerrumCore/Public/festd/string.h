#pragma once
#include <FeCore/Modules/Environment.h>
#include <festd/Internal/StringStorageImpl.h>
#include <type_traits>

namespace FE::Internal
{
    template<class TStorage>
    struct DefaultAllocatorStringStorage : public TStorage
    {
        static constexpr bool kHasAllocator = false;

        std::pmr::memory_resource* GetAllocator() const
        {
            return std::pmr::get_default_resource();
        }

        void SetAllocator(std::pmr::memory_resource*) {}
    };


    template<class TStorage>
    struct PolymorphicAllocatorStringStorage : public TStorage
    {
        static constexpr bool kHasAllocator = true;

        std::pmr::memory_resource* m_allocator;

        PolymorphicAllocatorStringStorage()
        {
            m_allocator = std::pmr::get_default_resource();
        }

        PolymorphicAllocatorStringStorage(std::pmr::memory_resource* allocator)
            : m_allocator(allocator)
        {
            if (allocator == nullptr)
                m_allocator = std::pmr::get_default_resource();
        }

        void SetAllocator(std::pmr::memory_resource* allocator)
        {
            if (allocator == nullptr)
                allocator = std::pmr::get_default_resource();

            m_allocator = allocator;
        }

        std::pmr::memory_resource* GetAllocator() const
        {
            return m_allocator;
        }
    };


    struct BasicStringViewImpl
    {
        constexpr BasicStringViewImpl()
            : m_data(nullptr)
            , m_size(0)
        {
        }

        constexpr BasicStringViewImpl(const char* str, const uint32_t byteSize)
            : m_data(str)
            , m_size(byteSize)
        {
        }

        constexpr BasicStringViewImpl(const char* str)
            : m_data(str)
            , m_size(ASCII::Length(str))
        {
        }

        BasicStringViewImpl(const StrIterator begin, const StrIterator end)
            : BasicStringViewImpl(reinterpret_cast<const char*>(begin.m_iter), static_cast<uint32_t>(end.m_iter - begin.m_iter))
        {
        }

        BasicStringViewImpl(const char* begin, const char* end)
            : BasicStringViewImpl(begin, static_cast<uint32_t>(end - begin))
        {
        }

        [[nodiscard]] constexpr uint32_t size() const
        {
            return m_size;
        }

        [[nodiscard]] constexpr const char* data() const
        {
            return m_data;
        }

    private:
        const char* m_data;
        uint32_t m_size;
    };


    template<class TStorage>
    struct BasicStringImpl : private TStorage
    {
        using Iter = StrIterator;

        BasicStringImpl()
        {
            if (char* data = TStorage::InitializeImpl(0, TStorage::GetAllocator()))
                data[0] = '\0';
        }

        template<class = std::enable_if_t<TStorage::kHasAllocator>>
        BasicStringImpl(std::pmr::memory_resource* allocator)
            : TStorage(allocator)
        {
            if (char* data = TStorage::InitializeImpl(0, TStorage::GetAllocator()))
                data[0] = '\0';
        }

        ~BasicStringImpl()
        {
            TStorage::DestroyImpl(TStorage::GetAllocator());
        }

        BasicStringImpl(const BasicStringImpl& other)
        {
            if constexpr (TStorage::kHasAllocator)
                TStorage::SetAllocator(other.GetAllocator());

            const uint32_t size = other.size();
            char* data = TStorage::InitializeImpl(size, TStorage::GetAllocator());
            memcpy(data, other.data(), size);
            data[size] = '\0';
        }

        BasicStringImpl(BasicStringImpl&& other)
        {
            MoveStorageFrom(other);
        }

        BasicStringImpl& operator=(const BasicStringImpl& other)
        {
            if (this == &other)
                return *this;

            const uint32_t size = other.size();

            char* data;
            if (TStorage::kHasAllocator && other.GetAllocator() != TStorage::GetAllocator())
            {
                TStorage::DestroyImpl(TStorage::GetAllocator());
                TStorage::SetAllocator(other.GetAllocator());
                data = TStorage::InitializeImpl(size, TStorage::GetAllocator());
            }
            else
            {
                data = TStorage::Reinitialize(size, TStorage::GetAllocator());
            }

            memcpy(data, other.data(), size);
            data[size] = '\0';
            return *this;
        }

        BasicStringImpl& operator=(BasicStringImpl&& other)
        {
            if (this == &other)
                return *this;

            TStorage::DestroyImpl(TStorage::GetAllocator());
            MoveStorageFrom(other);
            return *this;
        }

        BasicStringImpl(const uint32_t length, const char value)
        {
            char* data = TStorage::InitializeImpl(length, TStorage::GetAllocator());
            memset(data, value, length);
            data[length] = '\0';
        }

        BasicStringImpl(const char* str, uint32_t byteSize)
        {
            char* data = TStorage::InitializeImpl(byteSize, TStorage::GetAllocator());
            memcpy(data, str, byteSize);
            data[byteSize] = '\0';
        }

        BasicStringImpl(const char* str)
            : BasicStringImpl(str, ASCII::Length(str))
        {
        }

        BasicStringImpl(const StrIterator begin, const StrIterator end)
            : BasicStringImpl(begin.m_iter, static_cast<uint32_t>(end.m_iter - begin.m_iter))
        {
        }

        BasicStringImpl(const char* begin, const char* end)
            : BasicStringImpl(begin, static_cast<uint32_t>(end - begin))
        {
        }

        char* reinitialize(const uint32_t byteSize)
        {
            return TStorage::Reinitialize(byteSize, TStorage::GetAllocator());
        }

        void reserve(const uint32_t byteSize)
        {
            TStorage::ReserveImpl(byteSize, TStorage::GetAllocator());
        }

        void resize_uninitialized(const uint32_t byteSize)
        {
            char* bytes = TStorage::ResizeImpl(byteSize, TStorage::GetAllocator());
            bytes[byteSize] = '\0';
        }

        void resize(const uint32_t byteSize, const char value)
        {
            const uint32_t initialSize = size();
            char* data = TStorage::ResizeImpl(byteSize, TStorage::GetAllocator());

            if (byteSize > initialSize)
                memset(data + initialSize, value, byteSize - initialSize);

            data[byteSize] = '\0';
        }

        void clear()
        {
            resize_uninitialized(0);
        }

        void shrink_to_fit()
        {
            TStorage::ShrinkImpl(TStorage::GetAllocator());
        }

        void assign(const char* str, const uint32_t byteSize)
        {
            const char* oldData = data();
            const uint32_t oldSize = size();
            const uintptr_t sourceAddress = reinterpret_cast<uintptr_t>(str);
            const uintptr_t oldDataAddress = reinterpret_cast<uintptr_t>(oldData);
            const bool overlaps = byteSize != 0 && sourceAddress >= oldDataAddress && sourceAddress <= oldDataAddress + oldSize;
            if (overlaps)
            {
                std::pmr::memory_resource* allocator = TStorage::GetAllocator();
                char* temporary = Memory::AllocateArray<char>(allocator, byteSize + 1);
                memcpy(temporary, str, byteSize);
                temporary[byteSize] = '\0';

                resize_uninitialized(byteSize);
                memcpy(data(), temporary, byteSize + 1);
                allocator->deallocate(temporary, byteSize + 1, alignof(char));
                return;
            }

            resize_uninitialized(byteSize);
            char* bytes = data();
            memcpy(bytes, str, byteSize);
            bytes[byteSize] = '\0';
        }

        void assign(const Iter begin, const Iter end)
        {
            assign(begin.m_iter, static_cast<uint32_t>(end.m_iter - begin.m_iter));
        }

        void append(const char* str, const uint32_t byteSize)
        {
            const uint32_t oldSize = size();
            const char* oldData = data();
            const uintptr_t sourceAddress = reinterpret_cast<uintptr_t>(str);
            const uintptr_t oldDataAddress = reinterpret_cast<uintptr_t>(oldData);
            const bool overlaps = byteSize != 0 && sourceAddress >= oldDataAddress && sourceAddress < oldDataAddress + oldSize;
            const uint32_t sourceOffset = overlaps ? static_cast<uint32_t>(sourceAddress - oldDataAddress) : 0;

            resize_uninitialized(oldSize + byteSize);
            char* bytes = data();
            if (overlaps)
                str = bytes + sourceOffset;

            memmove(bytes + oldSize, str, byteSize);
            bytes[oldSize + byteSize] = '\0';
        }

        void append(const char* str)
        {
            append(str, ASCII::Length(str));
        }

        void append(const Iter begin, const Iter end)
        {
            append(begin.m_iter, static_cast<uint32_t>(end.m_iter - begin.m_iter));
        }

        void append(const int32_t codepoint)
        {
            char bytes[4];
            const int32_t bytesWritten = UTF8::Encode(codepoint, bytes);
            append(bytes, bytesWritten);
        }

        void push_back(const char byte)
        {
            const uint32_t oldSize = size();
            resize_uninitialized(oldSize + 1);
            char* bytes = data();
            bytes[oldSize] = byte;
            bytes[oldSize + 1] = '\0';
        }

        [[nodiscard]] std::pmr::memory_resource* get_allocator() const
        {
            return TStorage::GetAllocator();
        }

        template<class = std::enable_if_t<TStorage::kHasAllocator>>
        void set_allocator(std::pmr::memory_resource* allocator)
        {
            if (allocator == nullptr)
                allocator = std::pmr::get_default_resource();

            if (allocator == TStorage::GetAllocator())
                return;

            BasicStringImpl temporary{ allocator };
            temporary.assign(data(), size());
            *this = std::move(temporary);
        }

        [[nodiscard]] uint32_t size() const
        {
            return TStorage::SizeImpl();
        }

        [[nodiscard]] uint32_t length() const
        {
            return UTF8::Length(data(), size());
        }

        [[nodiscard]] uint32_t capacity() const
        {
            return TStorage::CapacityImpl();
        }

        [[nodiscard]] char* data()
        {
            return TStorage::DataImpl();
        }

        [[nodiscard]] const char* data() const
        {
            return TStorage::DataImpl();
        }

        [[nodiscard]] const char* c_str() const
        {
            return TStorage::DataImpl();
        }

    private:
        void MoveStorageFrom(BasicStringImpl& other) noexcept
        {
            static_assert(std::is_trivially_copyable_v<TStorage>);
            memcpy(static_cast<TStorage*>(this), static_cast<TStorage*>(&other), sizeof(TStorage));
            other.ResetMovedFromImpl();
        }
    };


    template<class TBase>
    struct StringImpl : public TBase
    {
        using Iter = StrIterator;
        using Base = TBase;

        using TBase::TBase;

        using value_type = char;

        StringImpl(festd::ascii_view str)
            : TBase(str.data(), static_cast<uint32_t>(str.length()))
        {
        }

        template<class TOtherBase>
        StringImpl(const StringImpl<TOtherBase>& other)
            : TBase(other.data(), other.size())
        {
        }

        explicit StringImpl(const Env::Name name)
            : StringImpl(festd::ascii_view{ name })
        {
        }

        [[nodiscard]] char byte_at(uint32_t byteIndex) const
        {
            return TBase::data()[byteIndex];
        }

        [[nodiscard]] int32_t codepoint_at(const uint32_t codepointIndex) const
        {
            return *(begin() + codepointIndex);
        }

        [[nodiscard]] festd::ascii_view substr_ascii(const uint32_t startIndex, uint32_t length = kInvalidIndex) const
        {
            const char* str = TBase::data();
            if (startIndex > TBase::size())
                return {};

            return festd::ascii_view{ str + startIndex, Math::Min(length, TBase::size() - startIndex) };
        }

        [[nodiscard]] StringImpl<BasicStringViewImpl> substr(const uint32_t startIndex, uint32_t length = kInvalidIndex) const
        {
            const Iter currentBegin = begin();
            const Iter currentEnd = end();
            Iter startIt = currentBegin;
            uint32_t remainingStart = startIndex;
            while (remainingStart-- && startIt != currentEnd)
                ++startIt;

            Iter endIt = startIt;
            while (length-- && endIt != currentEnd)
                ++endIt;

            return StringImpl<BasicStringViewImpl>{ startIt, endIt };
        }

        [[nodiscard]] StringImpl<BasicStringViewImpl> substr(const Iter startIter, const Iter endIter) const
        {
            return StringImpl<BasicStringViewImpl>{ startIter, endIter };
        }

        [[nodiscard]] StringImpl<BasicStringViewImpl> substr(const Iter startIter) const
        {
            return StringImpl<BasicStringViewImpl>{ startIter, end() };
        }

        [[nodiscard]] int32_t compare(const char* other) const
        {
            return UTF8::Compare(TBase::data(), other, TBase::size(), ASCII::Length(other));
        }

        template<class TOtherStorage>
        [[nodiscard]] int32_t compare(const StringImpl<TOtherStorage>& other) const
        {
            return UTF8::Compare(TBase::data(), other.data(), TBase::size(), other.size());
        }

        [[nodiscard]] Iter find_first_of(Iter position, const int32_t codepoint) const
        {
            while (position != end())
            {
                if (*position == codepoint)
                    break;
                ++position;
            }

            return position;
        }

        [[nodiscard]] Iter find_first_of(const int32_t codepoint) const
        {
            return find_first_of(begin(), codepoint);
        }

        [[nodiscard]] Iter find_last_of(Iter position, const int32_t codepoint) const
        {
            const Iter first = begin();
            while (position != first)
            {
                --position;
                if (*position == codepoint)
                    return position;
            }

            return end();
        }

        [[nodiscard]] Iter find_last_of(const int32_t codepoint) const
        {
            return find_last_of(end(), codepoint);
        }

        [[nodiscard]] StringImpl<BasicStringViewImpl> strip_left() const
        {
            const Iter nonSpace = eastl::find_if(begin(), end(), [](const int32_t codepoint) {
                return !UTF8::IsSpace(codepoint);
            });

            return StringImpl<BasicStringViewImpl>{ nonSpace, end() };
        }

        [[nodiscard]] StringImpl<BasicStringViewImpl> strip_right() const
        {
            const eastl::reverse_iterator<Iter> nonSpace = eastl::find_if(rbegin(), rend(), [](const int32_t codepoint) {
                return !UTF8::IsSpace(codepoint);
            });

            return StringImpl<BasicStringViewImpl>{ begin(), nonSpace.base() };
        }

        [[nodiscard]] StringImpl<BasicStringViewImpl> strip() const
        {
            return strip_left().strip_right();
        }

        [[nodiscard]] bool empty() const
        {
            return TBase::size() == 0;
        }

        [[nodiscard]] bool starts_with(const StringImpl<BasicStringViewImpl> str) const
        {
            if (str.size() == 0)
                return true;

            const uint32_t size = TBase::size();
            const char* data = TBase::data();
            return str.size() <= size && memcmp(data, str.data(), str.size()) == 0;
        }

        [[nodiscard]] bool ends_with(const StringImpl<BasicStringViewImpl> str) const
        {
            if (str.size() == 0)
                return true;

            const uint32_t size = TBase::size();
            const char* data = TBase::data();
            return str.size() <= size && memcmp(data + size - str.size(), str.data(), str.size()) == 0;
        }

        [[nodiscard]] Iter find(const StringImpl<BasicStringViewImpl> str) const
        {
            const uint32_t byteSize = TBase::size();
            const uint32_t otherByteSize = str.size();
            if (otherByteSize == 0)
                return begin();

            if (otherByteSize > byteSize)
                return end();

            const char* data = TBase::data();
            const char* otherData = str.data();
            for (uint32_t i = 0; i < byteSize - otherByteSize + 1; ++i)
            {
                if (memcmp(data + i, otherData, otherByteSize) == 0)
                    return Iter{ data + i };
            }

            return end();
        }

        [[nodiscard]] explicit operator Env::Name() const noexcept
        {
            return Env::Name{ TBase::data(), TBase::size() };
        }

        [[nodiscard]] Iter begin() const
        {
            return Iter{ TBase::data() };
        }

        [[nodiscard]] Iter end() const
        {
            return Iter{ TBase::data() + TBase::size() };
        }

        [[nodiscard]] eastl::reverse_iterator<Iter> rbegin() const
        {
            return eastl::reverse_iterator<Iter>(end());
        }

        [[nodiscard]] eastl::reverse_iterator<Iter> rend() const
        {
            return eastl::reverse_iterator<Iter>(begin());
        }
    };


    template<class TStorage>
    StringImpl<BasicStringImpl<TStorage>>& operator+=(StringImpl<BasicStringImpl<TStorage>>& lhs,
                                                      const StringImpl<BasicStringViewImpl> rhs)
    {
        lhs.append(rhs.data(), rhs.size());
        return lhs;
    }


    template<class TStorage>
    StringImpl<BasicStringImpl<TStorage>> operator+(const StringImpl<BasicStringImpl<TStorage>>& lhs,
                                                    const StringImpl<BasicStringViewImpl> rhs)
    {
        StringImpl<BasicStringImpl<TStorage>> result;
        result.reserve(lhs.size() + rhs.size());
        result.append(lhs.data(), lhs.size());
        result.append(rhs.data(), rhs.size());
        return result;
    }


    inline bool operator==(const StringImpl<BasicStringViewImpl> lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return lhs.size() == rhs.size() && (lhs.size() == 0 || memcmp(lhs.data(), rhs.data(), lhs.size()) == 0);
    }

    inline bool operator!=(const StringImpl<BasicStringViewImpl> lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return !(lhs == rhs);
    }

    inline bool operator<(const StringImpl<BasicStringViewImpl> lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return lhs.compare(rhs) < 0;
    }

    inline bool operator>(const StringImpl<BasicStringViewImpl> lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return lhs.compare(rhs) > 0;
    }

    inline bool operator<=(const StringImpl<BasicStringViewImpl> lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return lhs.compare(rhs) <= 0;
    }

    inline bool operator>=(const StringImpl<BasicStringViewImpl> lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return lhs.compare(rhs) >= 0;
    }


    inline bool operator==(const StringImpl<BasicStringViewImpl> lhs, const char* rhs)
    {
        return lhs.size() == ASCII::Length(rhs) && (lhs.size() == 0 || memcmp(lhs.data(), rhs, lhs.size()) == 0);
    }

    inline bool operator!=(const StringImpl<BasicStringViewImpl> lhs, const char* rhs)
    {
        return !(lhs == rhs);
    }

    inline bool operator<(const StringImpl<BasicStringViewImpl> lhs, const char* rhs)
    {
        return lhs.compare(rhs) < 0;
    }

    inline bool operator>(const StringImpl<BasicStringViewImpl> lhs, const char* rhs)
    {
        return lhs.compare(rhs) > 0;
    }

    inline bool operator<=(const StringImpl<BasicStringViewImpl> lhs, const char* rhs)
    {
        return lhs.compare(rhs) <= 0;
    }

    inline bool operator>=(const StringImpl<BasicStringViewImpl> lhs, const char* rhs)
    {
        return lhs.compare(rhs) >= 0;
    }


    inline bool operator==(const char* lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return ASCII::Length(lhs) == rhs.size() && (rhs.size() == 0 || memcmp(lhs, rhs.data(), rhs.size()) == 0);
    }

    inline bool operator!=(const char* lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return !(lhs == rhs);
    }

    inline bool operator<(const char* lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return StringImpl<BasicStringViewImpl>{ lhs }.compare(rhs) < 0;
    }

    inline bool operator>(const char* lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return StringImpl<BasicStringViewImpl>{ lhs }.compare(rhs) > 0;
    }

    inline bool operator<=(const char* lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return StringImpl<BasicStringViewImpl>{ lhs }.compare(rhs) <= 0;
    }

    inline bool operator>=(const char* lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return StringImpl<BasicStringViewImpl>{ lhs }.compare(rhs) >= 0;
    }


    inline bool operator==(const Env::Name lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return lhs.size() == rhs.size() && (lhs.size() == 0 || memcmp(lhs.c_str(), rhs.data(), lhs.size()) == 0);
    }

    inline bool operator!=(const Env::Name lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return !(lhs == rhs);
    }

    inline bool operator<(const Env::Name lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return StringImpl<BasicStringViewImpl>{ lhs }.compare(rhs) < 0;
    }

    inline bool operator>(const Env::Name lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return StringImpl<BasicStringViewImpl>{ lhs }.compare(rhs) > 0;
    }

    inline bool operator<=(const Env::Name lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return StringImpl<BasicStringViewImpl>{ lhs }.compare(rhs) <= 0;
    }

    inline bool operator>=(const Env::Name lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return StringImpl<BasicStringViewImpl>{ lhs }.compare(rhs) >= 0;
    }


    inline bool operator==(const StringImpl<BasicStringViewImpl> lhs, const Env::Name rhs)
    {
        return lhs.size() == rhs.size() && (lhs.size() == 0 || memcmp(lhs.data(), rhs.c_str(), lhs.size()) == 0);
    }

    inline bool operator!=(const StringImpl<BasicStringViewImpl> lhs, const Env::Name rhs)
    {
        return !(lhs == rhs);
    }

    inline bool operator<(const StringImpl<BasicStringViewImpl> lhs, const Env::Name rhs)
    {
        return lhs.compare(StringImpl<BasicStringViewImpl>{ rhs }) < 0;
    }

    inline bool operator>(const StringImpl<BasicStringViewImpl> lhs, const Env::Name rhs)
    {
        return lhs.compare(StringImpl<BasicStringViewImpl>{ rhs }) > 0;
    }

    inline bool operator<=(const StringImpl<BasicStringViewImpl> lhs, const Env::Name rhs)
    {
        return lhs.compare(StringImpl<BasicStringViewImpl>{ rhs }) <= 0;
    }

    inline bool operator>=(const StringImpl<BasicStringViewImpl> lhs, const Env::Name rhs)
    {
        return lhs.compare(StringImpl<BasicStringViewImpl>{ rhs }) >= 0;
    }


    using DefaultDynamicStringStorage = DefaultAllocatorStringStorage<DynamicStringStorage>;
    using PolymorphicStringStorage = PolymorphicAllocatorStringStorage<DynamicStringStorage>;

    template<uint32_t TCapacity>
    using DefaultFixedStringStorage = DefaultAllocatorStringStorage<FixedStringStorage<TCapacity>>;

    template<uint32_t TCapacity>
    using DefaultInlineStringStorage = DefaultAllocatorStringStorage<InlineStringStorage<TCapacity>>;

    template<uint32_t TCapacity>
    using PolymorphicInlineStringStorage = PolymorphicAllocatorStringStorage<InlineStringStorage<TCapacity>>;
} // namespace FE::Internal


namespace FE::festd
{
    namespace pmr
    {
        using string = FE::Internal::StringImpl<FE::Internal::BasicStringImpl<FE::Internal::PolymorphicStringStorage>>;

        template<uint32_t TCapacity>
        using basic_inline_string =
            FE::Internal::StringImpl<FE::Internal::BasicStringImpl<FE::Internal::PolymorphicInlineStringStorage<TCapacity>>>;
        using inline_string = basic_inline_string<256>;
    }

    using string = FE::Internal::StringImpl<FE::Internal::BasicStringImpl<FE::Internal::DefaultDynamicStringStorage>>;
    using string_view = FE::Internal::StringImpl<FE::Internal::BasicStringViewImpl>;

    template<uint32_t TCapacity>
    using basic_fixed_string =
        FE::Internal::StringImpl<FE::Internal::BasicStringImpl<FE::Internal::DefaultFixedStringStorage<TCapacity>>>;
    using fixed_string = basic_fixed_string<256>;

    template<uint32_t TCapacity>
    using basic_inline_string =
        FE::Internal::StringImpl<FE::Internal::BasicStringImpl<FE::Internal::DefaultInlineStringStorage<TCapacity>>>;
    using inline_string = basic_inline_string<256>;

    static_assert(sizeof(string) == sizeof(uintptr_t) * 3);
    static_assert(sizeof(pmr::string) == sizeof(string) + sizeof(uintptr_t));
    static_assert(sizeof(string_view) == sizeof(uintptr_t) * 2);
} // namespace FE::festd


namespace FE
{
    constexpr uint64_t CompileTimeHash(const festd::string_view str)
    {
        return CompileTimeHash(str.data(), str.size());
    }


    inline uint64_t DefaultHash(const festd::string_view str)
    {
        return DefaultHash(str.data(), str.size());
    }


    namespace Str
    {
        inline festd::string_view Duplicate(const festd::string_view str, std::pmr::memory_resource* allocator)
        {
            void* memory = allocator->allocate(str.size() + 1);
            memcpy(memory, str.data(), str.size());
            static_cast<char*>(memory)[str.size()] = '\0';
            return { static_cast<char*>(memory), str.size() };
        }
    } // namespace Str
} // namespace FE

FE_RTTI_Reflect(FE::festd::string, "A9081CB4-1614-47E0-9E5D-BC667A526021");
FE_RTTI_Reflect(FE::festd::string_view, "E72E9011-13E9-4E8B-B51A-F156E1F4A980");
FE_RTTI_Reflect(FE::festd::pmr::string, "DA1CF83D-8C08-48D8-BD35-4F7C96657E0D");
FE_RTTI_Reflect(FE::festd::fixed_string, "4C224BCA-8F0A-4C01-973D-D8BB85AF3411");
FE_RTTI_Reflect(FE::festd::inline_string, "44A6DCE9-DF70-4A80-8258-2FF71D336989");
FE_RTTI_Reflect(FE::festd::pmr::inline_string, "28F0C62D-B326-4E5B-9267-D55812B06217");


template<class TBase>
struct eastl::hash<FE::Internal::StringImpl<TBase>>
{
    size_t operator()(const FE::Internal::StringImpl<TBase>& str) const
    {
        return FE::DefaultHash(str.data(), str.size());
    }
};
