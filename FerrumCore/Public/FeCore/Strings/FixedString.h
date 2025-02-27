#pragma once
#include <FeCore/Strings/StringSlice.h>

namespace FE
{
    //! @brief String class that never allocates.
    template<size_t TCapacity>
    class FixedString final
    {
        static_assert(TCapacity <= (1 << 24));

        using SizeBaseType = std::conditional_t<TCapacity <= 255, uint16_t, uint32_t>;

        TChar m_data[TCapacity];
        SizeBaseType m_zero : 8;
        SizeBaseType m_size : (sizeof(SizeBaseType) - 1) * 8;

    public:
        static constexpr uint32_t kCapacity = TCapacity;

        using Iterator = Internal::StrIterator;

        FixedString() noexcept
            : m_zero(0)
            , m_size(0)
        {
            *m_data = 0;
        }

        FixedString(uint32_t length, const TChar value) noexcept
            : m_zero(0)
        {
            FE_CORE_ASSERT(length <= TCapacity, "Overflow");
            memset(m_data, value, length);
            m_size = static_cast<SizeBaseType>(length);
            m_data[m_size] = 0;
        }

        FixedString(const TChar* str, uint32_t byteSize) noexcept
            : m_zero(0)
        {
            FE_CORE_ASSERT(byteSize <= TCapacity, "Overflow");
            memcpy(m_data, str, byteSize);
            m_size = static_cast<SizeBaseType>(byteSize);
            m_data[m_size] = 0;
        }

        FixedString(const StringSlice slice) noexcept
            : FixedString(slice.Data(), slice.Size())
        {
        }

        FixedString(const TChar* str) noexcept
            : FixedString(str, Str::ByteLength(str))
        {
        }

        FixedString(Iterator begin, const Iterator end) noexcept
            : FixedString(begin.m_Iter, end.m_Iter - begin.m_Iter)
        {
        }

        [[nodiscard]] const TChar* Data() const noexcept
        {
            return m_data;
        }

        [[nodiscard]] TChar* Data() noexcept
        {
            return m_data;
        }

        // O(1)
        [[nodiscard]] TChar ByteAt(const uint32_t index) const
        {
            FE_CORE_ASSERT(index < Size(), "Invalid index");
            return Data()[index];
        }

        // O(N)
        [[nodiscard]] TCodepoint CodePointAt(const uint32_t index) const
        {
            return Str::CodepointAt(Data(), Size(), index);
        }

        // O(1)
        [[nodiscard]] uint32_t Size() const noexcept
        {
            return m_size;
        }

        [[nodiscard]] bool Empty() const noexcept
        {
            return Size() == 0;
        }

        // O(1)
        [[nodiscard]] constexpr uint32_t Capacity() const noexcept
        {
            return TCapacity;
        }

        // O(N)
        [[nodiscard]] uint32_t Length() const noexcept
        {
            auto ptr = Data();
            return UTF8::Length(ptr, Size());
        }

        operator StringSlice() const noexcept
        {
            return { Data(), Size() };
        }

        [[nodiscard]] explicit operator Env::Name() const noexcept
        {
            return Env::Name{ Data(), Size() };
        }

        StringSlice Substring(const uint32_t beginIndex, const uint32_t length) const
        {
            auto begin = Data();
            auto end = Data();
            UTF8::Advance(begin, beginIndex);
            UTF8::Advance(end, beginIndex + length);
            return StringSlice(begin, end - begin);
        }

        [[nodiscard]] StringSlice ASCIISubstring(const uint32_t beginIndex, const uint32_t length) const
        {
            auto begin = Data() + beginIndex;
            auto end = Data() + beginIndex + length;
            return StringSlice(begin, end - begin);
        }

        void Clear() noexcept
        {
            m_size = 0;
            *m_data = 0;
        }

        FixedString& Append(const TChar* str, uint32_t count)
        {
            FE_CORE_ASSERT(count == 0 || str != nullptr, "Couldn't append more than 0 chars from a null string");
            assert(count + Size() <= Capacity());
            if (count == 0)
                return *this;

            memcpy(Data() + Size(), str, count);
            m_size += static_cast<SizeBaseType>(count);
            m_data[m_size] = 0;
            return *this;
        }

        FixedString& Append(const StringSlice str)
        {
            return Append(str.Data(), str.Size());
        }

        FixedString& Append(const TChar c)
        {
            assert(Size() < Capacity());
            return Append(&c, 1);
        }

        FixedString& Append(const TChar* str)
        {
            return Append(str, Str::ByteLength(str));
        }

        void Resize(uint32_t length, const TChar value) noexcept
        {
            assert(length <= TCapacity);
            memset(m_data, value, length);
            m_size = static_cast<SizeBaseType>(length);
            m_data[m_size] = 0;
        }

        FixedString& operator+=(const StringSlice str)
        {
            return Append(str);
        }

        FixedString& operator/=(const StringSlice str)
        {
            if (!Empty() && m_data[m_size - 1] != '/')
                Append('/');
            return Append(str);
        }

        friend FixedString operator+(const FixedString& lhs, const StringSlice rhs)
        {
            FixedString t{ lhs };
            t += rhs;
            return t;
        }

        friend FixedString operator/(const FixedString& lhs, const StringSlice rhs)
        {
            FixedString t{ lhs };
            t /= rhs;
            return t;
        }

        [[nodiscard]] Iterator FindFirstOf(const Iterator start, const TCodepoint search) const noexcept
        {
            const uint32_t size = Size();
            const TChar* data = Data();
            assert(start.m_Iter >= data && start.m_Iter <= data + size);

            const uint32_t searchSize = data + size - start.m_Iter;
            return Str::FindFirstOf(start.m_Iter, searchSize, search);
        }

        [[nodiscard]] Iterator FindFirstOf(const TCodepoint search) const noexcept
        {
            return FindFirstOf(begin(), search);
        }

        [[nodiscard]] Iterator FindLastOf(const TCodepoint search) const noexcept
        {
            return Str::FindLastOf(Data(), Size(), search);
        }

        [[nodiscard]] festd::pmr::vector<StringSlice> Split(const TCodepoint c = ' ',
                                                            std::pmr::memory_resource* pAllocator = nullptr) const
        {
            return StringSlice{ Data(), Size() }.Split(c, pAllocator);
        }

        [[nodiscard]] festd::pmr::vector<StringSlice> SplitLines(std::pmr::memory_resource* pAllocator = nullptr) const
        {
            return StringSlice{ Data(), Size() }.SplitLines(pAllocator);
        }

        [[nodiscard]] StringSlice StripLeft(const StringSlice chars = "\n\r\t ") const noexcept
        {
            return { Iterator{ Str::StripLeft(Data(), Size(), chars.Data(), chars.Size()) }, end() };
        }

        [[nodiscard]] StringSlice StripRight(const StringSlice chars = "\n\r\t ") const noexcept
        {
            return { begin(), Iterator{ Str::StripRight(Data(), Size(), chars.Data(), chars.Size()) } };
        }

        [[nodiscard]] StringSlice Strip(const StringSlice chars = "\n\r\t ") const noexcept
        {
            return StripLeft(chars).StripRight(chars);
        }

        [[nodiscard]] int32_t Compare(const StringSlice& other) const noexcept
        {
            return UTF8::Compare(Data(), other.Data(), Size(), other.Size());
        }

        [[nodiscard]] bool IsEqualTo(const StringSlice& other, const bool caseSensitive = true) const noexcept
        {
            return UTF8::AreEqual(Data(), other.Data(), Size(), other.Size(), caseSensitive);
        }

        [[nodiscard]] bool StartsWith(const StringSlice prefix, const bool caseSensitive = true) const noexcept
        {
            if (prefix.Size() > Size())
                return false;

            return UTF8::AreEqual(Data(), prefix.Data(), prefix.Size(), prefix.Size(), caseSensitive);
        }

        [[nodiscard]] bool EndsWith(const StringSlice suffix, const bool caseSensitive = true) const noexcept
        {
            if (suffix.Size() > Size())
                return false;

            return UTF8::AreEqual(Data() + Size() - suffix.Size(), suffix.Data(), suffix.Size(), suffix.Size(), caseSensitive);
        }

        [[nodiscard]] static FixedString Join(const StringSlice& separator, const festd::span<StringSlice> strings)
        {
            FixedString result;
            for (uint32_t i = 0; i < strings.size(); ++i)
            {
                result.Append(strings[i]);
                if (i != strings.size() - 1)
                    result.Append(separator);
            }

            return result;
        }

        [[nodiscard]] Iterator begin() const noexcept
        {
            auto ptr = Data();
            return Iterator(ptr);
        }

        [[nodiscard]] Iterator end() const noexcept
        {
            auto ptr = Data();
            auto size = Size();
            return Iterator(ptr + size);
        }
    };


    using FixStr32 = FixedString<32>;
    using FixStr64 = FixedString<64>;
    using FixStr128 = FixedString<128>;
    using FixStr256 = FixedString<256>;
    using FixStr512 = FixedString<512>;
} // namespace FE


template<uint32_t TCapacity>
struct eastl::hash<FE::FixedString<TCapacity>>
{
    inline uint32_t operator()(const FE::FixedString<TCapacity>& str) const noexcept
    {
        return FE::DefaultHash(str.Data(), str.Size());
    }
};
