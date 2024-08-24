#pragma once
#include <FeCore/Strings/StringSlice.h>

namespace FE
{
    //! \brief String class that never allocates.
    template<size_t TCapacity>
    class FixedString final
    {
        static_assert(TCapacity <= (1 << 24));

        using SizeBaseType = std::conditional_t<TCapacity <= 255, uint16_t, uint32_t>;

        TChar m_Data[TCapacity];
        SizeBaseType m_Zero : 8;
        SizeBaseType m_Size : (sizeof(SizeBaseType) - 1) * 8;

    public:
        inline static constexpr uint32_t Cap = TCapacity;

        using Iterator = Internal::StrIterator;

        inline FixedString() noexcept
            : m_Zero(0)
            , m_Size(0)
        {
            *m_Data = 0;
        }

        inline FixedString(uint32_t length, TChar value) noexcept
            : m_Zero(0)
        {
            FE_CORE_ASSERT(length <= TCapacity, "Overflow");
            memset(m_Data, value, length);
            m_Size = static_cast<SizeBaseType>(length);
            m_Data[m_Size] = 0;
        }

        inline FixedString(const TChar* str, uint32_t byteSize) noexcept
            : m_Zero(0)
        {
            FE_CORE_ASSERT(byteSize <= TCapacity, "Overflow");
            memcpy(m_Data, str, byteSize);
            m_Size = static_cast<SizeBaseType>(byteSize);
            m_Data[m_Size] = 0;
        }

        inline FixedString(StringSlice slice) noexcept
            : FixedString(slice.Data(), slice.Size())
        {
        }

        inline FixedString(const TChar* str) noexcept
            : FixedString(str, Str::ByteLength(str))
        {
        }

        inline FixedString(Iterator begin, Iterator end) noexcept
            : FixedString(begin.m_Iter, end.m_Iter - begin.m_Iter)
        {
        }

        [[nodiscard]] inline const TChar* Data() const noexcept
        {
            return m_Data;
        }

        [[nodiscard]] inline TChar* Data() noexcept
        {
            return m_Data;
        }

        // O(1)
        [[nodiscard]] inline TChar ByteAt(uint32_t index) const
        {
            FE_CORE_ASSERT(index < Size(), "Invalid index");
            return Data()[index];
        }

        // O(N)
        [[nodiscard]] inline TCodepoint CodePointAt(uint32_t index) const
        {
            return Str::CodepointAt(Data(), Size(), index);
        }

        // O(1)
        [[nodiscard]] inline uint32_t Size() const noexcept
        {
            return m_Size;
        }

        [[nodiscard]] inline bool Empty() const noexcept
        {
            return Size() == 0;
        }

        // O(1)
        [[nodiscard]] inline uint32_t Capacity() const noexcept
        {
            return TCapacity;
        }

        // O(N)
        [[nodiscard]] inline uint32_t Length() const noexcept
        {
            auto ptr = Data();
            return UTF8::Length(ptr, Size());
        }

        inline operator StringSlice() const noexcept
        {
            return { Data(), Size() };
        }

        inline StringSlice Substring(uint32_t beginIndex, uint32_t length) const
        {
            auto begin = Data();
            auto end = Data();
            UTF8::Advance(begin, beginIndex);
            UTF8::Advance(end, beginIndex + length);
            return StringSlice(begin, end - begin);
        }

        [[nodiscard]] inline StringSlice ASCIISubstring(uint32_t beginIndex, uint32_t length) const
        {
            auto begin = Data() + beginIndex;
            auto end = Data() + beginIndex + length;
            return StringSlice(begin, end - begin);
        }

        inline void Clear() noexcept
        {
            m_Size = 0;
            *m_Data = 0;
        }

        inline FixedString& Append(const TChar* str, uint32_t count)
        {
            FE_CORE_ASSERT(count == 0 || str != nullptr, "Couldn't append more than 0 chars from a null string");
            assert(count + Size() <= Capacity());
            if (count == 0)
                return *this;

            memcpy(Data() + Size(), str, count);
            m_Size += static_cast<SizeBaseType>(count);
            m_Data[m_Size] = 0;
            return *this;
        }

        inline FixedString& Append(StringSlice str)
        {
            return Append(str.Data(), str.Size());
        }

        inline FixedString& Append(TChar cp)
        {
            assert(Size() < Capacity());
            return Append(&str, 1);
        }

        inline FixedString& Append(const TChar* str)
        {
            return Append(str, Str::ByteLength(str));
        }

        inline void Resize(uint32_t length, TChar value) noexcept
        {
            assert(length <= TCapacity);
            memset(m_Data, value, length);
            m_Size = static_cast<SizeBaseType>(length);
            m_Data[m_Size] = 0;
        }

        inline FixedString& operator+=(StringSlice str)
        {
            return Append(str);
        }

        inline FixedString& operator/=(const StringSlice& str)
        {
            Append('/');
            return Append(str);
        }

        inline friend FixedString operator+(const FixedString& lhs, StringSlice rhs)
        {
            FixedString t{ lhs };
            t += rhs;
            return t;
        }

        inline friend FixedString operator/(const FixedString& lhs, StringSlice rhs)
        {
            FixedString t{ lhs };
            t /= rhs;
            return t;
        }

        [[nodiscard]] inline Iterator FindFirstOf(Iterator start, TCodepoint search) const noexcept
        {
            const uint32_t size = Size();
            const TChar* data = Data();
            assert(start.m_Iter >= data && start.m_Iter <= data + size);

            const uint32_t searchSize = data + size - start.m_Iter;
            return Str::FindFirstOf(start.m_Iter, searchSize, search);
        }

        [[nodiscard]] inline Iterator FindFirstOf(TCodepoint search) const noexcept
        {
            return FindFirstOf(begin(), search);
        }

        [[nodiscard]] inline Iterator FindLastOf(TCodepoint search) const noexcept
        {
            return Str::FindLastOf(Data(), Size(), search);
        }

        [[nodiscard]] inline festd::pmr::vector<StringSlice> Split(TCodepoint c = ' ',
                                                                   std::pmr::memory_resource* pAllocator = nullptr) const
        {
            return StringSlice{ Data(), Size() }.Split(c, pAllocator);
        }

        [[nodiscard]] inline festd::pmr::vector<StringSlice> SplitLines(std::pmr::memory_resource* pAllocator = nullptr) const
        {
            return StringSlice{ Data(), Size() }.SplitLines(pAllocator);
        }

        [[nodiscard]] inline StringSlice StripLeft(StringSlice chars = "\n\r\t ") const noexcept
        {
            return { Iterator{ Str::StripLeft(Data(), Size(), chars.Data(), chars.Size()) }, end() };
        }

        [[nodiscard]] inline StringSlice StripRight(StringSlice chars = "\n\r\t ") const noexcept
        {
            return { begin(), Iterator{ Str::StripRight(Data(), Size(), chars.Data(), chars.Size()) } };
        }

        [[nodiscard]] inline StringSlice Strip(StringSlice chars = "\n\r\t ") const noexcept
        {
            return StripLeft(chars).StripRight(chars);
        }

        [[nodiscard]] inline int32_t Compare(const StringSlice& other) const noexcept
        {
            return UTF8::Compare(Data(), other.Data(), Size(), other.Size());
        }

        [[nodiscard]] inline bool IsEqualTo(const StringSlice& other, bool caseSensitive = true) const noexcept
        {
            return UTF8::AreEqual(Data(), other.Data(), Size(), other.Size(), caseSensitive);
        }

        [[nodiscard]] inline bool StartsWith(StringSlice prefix, bool caseSensitive = true) const noexcept
        {
            if (prefix.Size() > Size())
                return false;

            return UTF8::AreEqual(Data(), prefix.Data(), prefix.Size(), prefix.Size(), caseSensitive);
        }

        [[nodiscard]] inline bool EndsWith(StringSlice suffix, bool caseSensitive = true) const noexcept
        {
            if (suffix.Size() > Size())
                return false;

            return UTF8::AreEqual(Data() + Size() - suffix.Size(), suffix.Data(), suffix.Size(), suffix.Size(), caseSensitive);
        }

        [[nodiscard]] inline static FixedString Join(const StringSlice& separator, festd::span<StringSlice> strings)
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

        [[nodiscard]] inline Iterator begin() const noexcept
        {
            auto ptr = Data();
            return Iterator(ptr);
        }

        [[nodiscard]] inline Iterator end() const noexcept
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
