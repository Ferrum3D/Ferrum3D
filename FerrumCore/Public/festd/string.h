#pragma once
#include <FeCore/Modules/Environment.h>
#include <festd/Internal/StringStorageImpl.h>

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
            char* data = TStorage::InitializeImpl(0, TStorage::GetAllocator());
            data[0] = '\0';
        }

        template<class = std::enable_if_t<TStorage::kHasAllocator>>
        BasicStringImpl(std::pmr::memory_resource* allocator)
            : TStorage(allocator)
        {
        }

        ~BasicStringImpl()
        {
            TStorage::DestroyImpl(TStorage::GetAllocator());
        }

        BasicStringImpl(const BasicStringImpl& other)
        {
            const uint32_t size = other.size();
            char* data = TStorage::InitializeImpl(size, TStorage::GetAllocator());
            memcpy(data, other.data(), size);
            data[size] = '\0';
        }

        BasicStringImpl(BasicStringImpl&& other) noexcept
        {
            memcpy(this, &other, sizeof(*this));
            other.InitializeImpl(0, other.GetAllocator());
        }

        BasicStringImpl& operator=(const BasicStringImpl& other) noexcept
        {
            if (this == &other)
                return *this;

            const uint32_t size = other.size();

            char* data;
            if (TStorage::kHasAllocator && other.GetAllocator() != TStorage::GetAllocator())
            {
                TStorage::DestroyImpl(TStorage::GetAllocator());
                data = TStorage::InitializeImpl(size, other.GetAllocator());
            }
            else
            {
                data = TStorage::Reinitialize(size, TStorage::GetAllocator());
            }

            memcpy(data, other.data(), size);
            data[size] = '\0';
            return *this;
        }

        BasicStringImpl& operator=(BasicStringImpl&& other) noexcept
        {
            if (this == &other)
                return *this;

            TStorage::DestroyImpl(TStorage::GetAllocator());
            memcpy(this, &other, sizeof(*this));
            other.InitializeImpl(0, other.GetAllocator());
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
            TStorage::ResizeImpl(byteSize, TStorage::GetAllocator());
        }

        void resize(const uint32_t byteSize, const char value)
        {
            const uint32_t initialSize = size();
            char* data = TStorage::ResizeImpl(byteSize, TStorage::GetAllocator());

            if (byteSize > initialSize)
                memset(data + initialSize, value, byteSize - initialSize);

            data[initialSize + byteSize] = '\0';
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
            resize_uninitialized(oldSize + byteSize);
            char* bytes = data();
            memcpy(bytes + oldSize, str, byteSize);
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
            TStorage::SetAllocator(allocator);
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
            return festd::ascii_view{ str + startIndex, Math::Min(length, TBase::size() - startIndex) };
        }

        [[nodiscard]] StringImpl<BasicStringViewImpl> substr(const uint32_t startIndex, uint32_t length = kInvalidIndex) const
        {
            const Iter currentBegin = begin();
            const Iter currentEnd = end();
            const Iter startIt = currentBegin + startIndex;
            Iter endIt = startIt;
            while (length-- && endIt != currentEnd)
                ++endIt;

            return StringImpl<BasicStringViewImpl>{ startIt, endIt };
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
            while (position != begin())
            {
                if (*position == codepoint)
                    break;
                --position;
            }

            if (*position != codepoint)
                position = end();

            return position;
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
            const uint32_t size = TBase::size();
            const char* data = TBase::data();
            return str.size() <= size && memcmp(data, str.data(), str.size()) == 0;
        }

        [[nodiscard]] bool ends_with(const StringImpl<BasicStringViewImpl> str) const
        {
            const uint32_t size = TBase::size();
            const char* data = TBase::data();
            return str.size() <= size && memcmp(data + size - str.size(), str.data(), str.size()) == 0;
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
        lhs.reserve(lhs.size() + rhs.size());
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
        return lhs.size() == rhs.size() && memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
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
        return lhs.size() == ASCII::Length(rhs) && memcmp(lhs.data(), rhs, lhs.size()) == 0;
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
        return ASCII::Length(lhs) == rhs.size() && memcmp(lhs, rhs.data(), rhs.size()) == 0;
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
        return lhs.size() == rhs.size() && memcmp(lhs.c_str(), rhs.data(), lhs.size()) == 0;
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
        return lhs.size() == rhs.size() && memcmp(lhs.data(), rhs.c_str(), lhs.size()) == 0;
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

    template<uint32_t TCapacity>
    using DefaultFixedStringStorage = DefaultAllocatorStringStorage<FixedStringStorage<TCapacity>>;
} // namespace FE::Internal


namespace FE::festd
{
    using string = FE::Internal::StringImpl<FE::Internal::BasicStringImpl<FE::Internal::DefaultDynamicStringStorage>>;
    using string_view = FE::Internal::StringImpl<FE::Internal::BasicStringViewImpl>;

    template<uint32_t TCapacity>
    using basic_fixed_string =
        FE::Internal::StringImpl<FE::Internal::BasicStringImpl<FE::Internal::DefaultFixedStringStorage<TCapacity>>>;
    using fixed_string = basic_fixed_string<256>;

    static_assert(sizeof(string) == sizeof(uintptr_t) * 3);
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
} // namespace FE
