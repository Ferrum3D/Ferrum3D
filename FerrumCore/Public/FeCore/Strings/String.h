#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Strings/StringSlice.h>

namespace FE
{
    //! @brief String class with UTF-8 support.
    class String final
    {
        static constexpr uint32_t kShortCapacity = 24;

        struct LongMode
        {
            TChar* Data;
            uint32_t Capacity;
            uint32_t Size;
            uint64_t Indicator;
        };

        struct ShortMode
        {
            TChar Data[kShortCapacity - 1];
            uint8_t Size;
        };

        struct
        {
            union
            {
                LongMode Long;
                ShortMode Short;
                uint64_t Words[3];
            };
        } m_Data;

        [[nodiscard]] bool IsLong() const noexcept
        {
            return m_Data.Short.Size == 0xff;
        }

        [[nodiscard]] uint32_t GetLCap() const noexcept
        {
            return m_Data.Long.Capacity;
        }

        void SetLCap(const uint32_t capacity) noexcept
        {
            m_Data.Long.Capacity = capacity;
        }

        uint32_t GetSSize() const noexcept
        {
            return m_Data.Short.Size ^ (kShortCapacity - 1);
        }

        void SetSSize(const uint32_t size) noexcept
        {
            m_Data.Short.Size = static_cast<uint8_t>(size ^ (kShortCapacity - 1));
        }

        uint32_t GetLSize() const noexcept
        {
            return m_Data.Long.Size;
        }

        void SetLSize(const uint32_t size) noexcept
        {
            m_Data.Long.Size = size;
            m_Data.Long.Indicator = Constants::kMaxU64;
        }

        void SetSize(const uint32_t size) noexcept
        {
            if (IsLong())
                SetLSize(size);
            else
                SetSSize(size);
        }

        void Zero() noexcept
        {
            m_Data.Words[0] = 0;
            m_Data.Words[1] = 0;
            m_Data.Words[2] = 0;
            SetSSize(0);
        }

        static uint32_t Recommend(const uint32_t s) noexcept
        {
            if (s < kShortCapacity)
                return kShortCapacity - 1;

            return AlignUp<Memory::kDefaultAlignment>(s + 1) - 1;
        }

        static TChar* Allocate(const uint32_t s) noexcept
        {
            return static_cast<TChar*>(Memory::DefaultAllocate(s));
        }

        static void Deallocate(TChar* c) noexcept
        {
            Memory::DefaultFree(c);
        }

        static void CopyData(TChar* dest, const TChar* src, const uint32_t size) noexcept
        {
            TCharTraits::copy(dest, src, size);
        }

        static void SetData(TChar* dest, const TChar value, const uint32_t size) noexcept
        {
            TCharTraits::assign(dest, size, value);
        }

        TChar* InitImpl(const uint32_t size) noexcept
        {
            TChar* newPtr;
            if (size < kShortCapacity)
            {
                SetSSize(size);
                newPtr = m_Data.Short.Data;
            }
            else
            {
                uint32_t newCap = Recommend(size);
                newPtr = Allocate(newCap + 1);
                m_Data.Long.Data = newPtr;
                SetLCap(newCap + 1);
                SetLSize(size);
            }

            return newPtr;
        }

        void Init(const TChar* str, const uint32_t size) noexcept
        {
            TChar* ptr = InitImpl(size);
            CopyData(ptr, str, size);
            ptr[size] = '\0';
        }

        void Init(const uint32_t count, const TChar value) noexcept
        {
            TChar* ptr = InitImpl(count);
            SetData(ptr, value, count);
            ptr[count] = '\0';
        }

        void GrowAndReplace(const uint32_t oldCap, const uint32_t deltaCap, uint32_t oldSize, const uint32_t copyCount,
                            const uint32_t delCount, const uint32_t addCount, const TChar* newChars)
        {
            TChar* oldData = Data();
            uint32_t cap = Recommend(std::max(oldCap + deltaCap, 2 * oldCap));
            TChar* newData = Allocate(cap + 1);
            if (copyCount)
                CopyData(newData, oldData, copyCount);
            if (addCount)
                CopyData(newData + copyCount, newChars, addCount);
            uint32_t copySize = oldSize - delCount - copyCount;
            if (copySize)
                CopyData(newData + copyCount + addCount, oldData + delCount, copySize);
            if (oldCap + 1 != kShortCapacity)
                Deallocate(oldData);
            m_Data.Long.Data = (newData);
            SetLCap(cap + 1);
            oldSize = copyCount + addCount + copySize;
            SetLSize(oldSize);
            newData[oldSize] = '\0';
        }

    public:
        using Iterator = Internal::StrIterator;

        String() noexcept
        {
            Zero();
        }

        String(const String& other) noexcept
        {
            if (!other.IsLong())
                m_Data = other.m_Data;
            else
                Init(other.m_Data.Long.Data, other.m_Data.Long.Size);
        }

        String& operator=(const String& other) noexcept
        {
            Clear();
            Shrink();
            if (!other.IsLong())
                m_Data = other.m_Data;
            else
                Init(other.m_Data.Long.Data, other.m_Data.Long.Size);
            return *this;
        }

        String(String&& other) noexcept
            : m_Data(other.m_Data)
        {
            other.Zero();
        }

        String& operator=(String&& other) noexcept
        {
            Clear();
            Shrink();
            m_Data = other.m_Data;
            other.Zero();
            return *this;
        }

        String(const uint32_t length, const TChar value) noexcept
        {
            Init(length, value);
        }

        String(const TChar* str, const uint32_t byteSize) noexcept
        {
            Init(str, byteSize);
        }

        String(const StringSlice slice) noexcept
        {
            Init(slice.Data(), slice.Size());
        }

        String(const TChar* str) noexcept
            : String(str, Str::ByteLength(str))
        {
        }

        ~String() noexcept
        {
            if (IsLong())
            {
                Deallocate(m_Data.Long.Data);
            }
        }

        [[nodiscard]] const TChar* Data() const noexcept
        {
            return IsLong() ? m_Data.Long.Data : m_Data.Short.Data;
        }

        TChar* Data() noexcept
        {
            return IsLong() ? m_Data.Long.Data : m_Data.Short.Data;
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
            if (IsLong())
                return Str::CodepointAt(m_Data.Long.Data, m_Data.Long.Size, index);

            return Str::CodepointAt(m_Data.Short.Data, GetSSize(), index);
        }

        // O(1)
        [[nodiscard]] uint32_t Size() const noexcept
        {
            return IsLong() ? GetLSize() : GetSSize();
        }

        [[nodiscard]] bool Empty() const noexcept
        {
            return Size() == 0;
        }

        // O(1)
        [[nodiscard]] uint32_t Capacity() const noexcept
        {
            return (IsLong() ? GetLCap() : kShortCapacity) - 1;
        }

        // O(N)
        [[nodiscard]] uint32_t Length() const noexcept
        {
            return UTF8::Length(Data(), Size());
        }

        operator StringSlice() const noexcept
        {
            return { Data(), Size() };
        }

        StringSlice Substring(const uint32_t beginIndex, const uint32_t length) const
        {
            auto begin = Data();
            auto end = Data();
            UTF8::Advance(begin, beginIndex);
            UTF8::Advance(end, beginIndex + length);
            return StringSlice(begin, static_cast<uint32_t>(end - begin));
        }

        [[nodiscard]] StringSlice ASCIISubstring(const uint32_t beginIndex, const uint32_t length) const
        {
            auto begin = Data() + beginIndex;
            auto end = Data() + beginIndex + length;
            return StringSlice(begin, static_cast<uint32_t>(end - begin));
        }

        void Reserve(uint32_t reserve) noexcept
        {
            const uint32_t cap = Capacity();
            if (cap >= reserve)
                return;

            reserve = Recommend(reserve);
            TChar* newData = Allocate(reserve + 1);
            TChar* oldData = Data();

            const uint32_t oldSize = Size();
            CopyData(newData, oldData, oldSize + 1);
            if (IsLong())
                Deallocate(oldData);

            SetLCap(reserve + 1);
            SetLSize(oldSize);
            m_Data.Long.Data = newData;
        }

        void Shrink() noexcept
        {
            const uint32_t cap = Capacity();
            const uint32_t size = Size();
            const uint32_t reserve = Recommend(size);
            if (reserve == cap)
                return;

            if (reserve == kShortCapacity - 1)
            {
                TChar* newData = m_Data.Short.Data;
                TChar* oldData = m_Data.Long.Data;

                CopyData(newData, oldData, size + 1);
                Deallocate(oldData);
                SetSSize(size);
            }
            else
            {
                TChar* newData = Allocate(reserve + 1);
                TChar* oldData = m_Data.Long.Data;

                CopyData(newData, oldData, size + 1);
                Deallocate(oldData);

                SetLCap(reserve + 1);
                SetLSize(size);
                m_Data.Long.Data = newData;
            }
        }

        void Clear() noexcept
        {
            *Data() = '\0';
            SetSize(0);
        }

        String& Append(const TChar* str, const uint32_t count)
        {
            FE_CORE_ASSERT(count == 0 || str != nullptr, "Couldn't append more than 0 chars from a null string");
            if (count == 0)
            {
                return *this;
            }

            const uint32_t cap = Capacity();
            uint32_t size = Size();
            if (cap - size >= count)
            {
                TChar* data = Data();
                CopyData(data + size, str, count);
                size += count;
                SetSize(size);
                data[size] = '\0';
            }
            else
            {
                GrowAndReplace(cap, size + count - cap, size, size, 0, count, str);
            }

            return *this;
        }

        String& Append(const StringSlice str)
        {
            return Append(str.Data(), str.Size());
        }

        String& Append(const TChar cp)
        {
            return Append(&cp, 1); // TODO: optimize this for single chars
        }

        String& Append(const TChar* str)
        {
            return Append(str, Str::ByteLength(str));
        }

        String& operator+=(const StringSlice str)
        {
            return Append(str);
        }

        String& operator/=(const StringSlice str)
        {
            if (Data()[Size() - 1] != '/')
                Append('/');
            return Append(str);
        }

        friend String operator+(const String& lhs, const StringSlice rhs)
        {
            String t;
            t.Reserve(lhs.Size() + rhs.Size() + 1);
            t += lhs;
            t += rhs;
            return t;
        }

        friend String operator/(const String& lhs, const StringSlice rhs)
        {
            String t;
            t.Reserve(lhs.Size() + rhs.Size() + 2);
            t += lhs;
            t /= rhs;
            return t;
        }

        [[nodiscard]] Iterator FindFirstOf(const Iterator start, const TCodepoint search) const noexcept
        {
            return StringSlice(Data(), Size()).FindFirstOf(start.m_Iter, search).m_Iter;
        }

        [[nodiscard]] Iterator FindFirstOf(const TCodepoint search) const noexcept
        {
            return StringSlice(Data(), Size()).FindFirstOf(search).m_Iter;
        }

        [[nodiscard]] Iterator FindLastOf(const TCodepoint search) const noexcept
        {
            return StringSlice(Data(), Size()).FindLastOf(search).m_Iter;
        }

        [[nodiscard]] festd::pmr::vector<StringSlice> Split(const TCodepoint c = ' ',
                                                            std::pmr::memory_resource* pAllocator = nullptr) const
        {
            return StringSlice(Data(), Size()).Split(c, pAllocator);
        }

        [[nodiscard]] festd::pmr::vector<StringSlice> SplitLines(std::pmr::memory_resource* pAllocator = nullptr) const
        {
            return StringSlice(Data(), Size()).SplitLines(pAllocator);
        }

        [[nodiscard]] StringSlice StripRight(const StringSlice chars = "\n\r\t ") const noexcept
        {
            return StringSlice(Data(), Size()).StripRight(chars);
        }

        [[nodiscard]] StringSlice StripLeft(const StringSlice chars = "\n\r\t ") const noexcept
        {
            return StringSlice(Data(), Size()).StripLeft(chars);
        }

        [[nodiscard]] StringSlice Strip(const StringSlice chars = "\n\r\t ") const noexcept
        {
            return StringSlice(Data(), Size()).Strip(chars);
        }

        [[nodiscard]] int32_t Compare(const StringSlice other) const noexcept
        {
            return UTF8::Compare(Data(), other.Data(), Size(), other.Size());
        }

        [[nodiscard]] bool IsEqualTo(const StringSlice other, const bool caseSensitive = true) const noexcept
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

        template<class T>
        [[nodiscard]] festd::expected<T, ParseError> Parse()
        {
            return StringSlice(Data(), Size()).Parse<T>();
        }

        [[nodiscard]] static String Join(const StringSlice separator, const festd::span<StringSlice>& strings)
        {
            String result;
            uint32_t capacity = 0;
            for (const StringSlice string : strings)
            {
                capacity += string.Size() + separator.Size();
            }

            result.Reserve(capacity);
            for (uint32_t i = 0; i < strings.size(); ++i)
            {
                result.Append(strings[i]);
                if (i != strings.size() - 1)
                {
                    result.Append(separator);
                }
            }

            return result;
        }

        [[nodiscard]] explicit operator UUID() const noexcept
        {
            return UUID(Data());
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

    template<>
    struct ValueParser<String> : std::true_type
    {
        inline static ParseError TryConvert(StringSlice str, String& result)
        {
            result = str;
            return ParseErrorCode::None;
        }
    };
} // namespace FE
