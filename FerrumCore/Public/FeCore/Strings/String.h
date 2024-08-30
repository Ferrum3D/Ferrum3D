#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Strings/StringSlice.h>

namespace FE
{
    //! @brief String class with UTF-8 support.
    class String final
    {
        inline static constexpr uint32_t ShortCapacity = 24;

        struct LongMode
        {
            TChar* Data;
            uint32_t Capacity;
            uint32_t Size;
            uint64_t Indicator;
        };

        struct ShortMode
        {
            TChar Data[ShortCapacity - 1];
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

        [[nodiscard]] inline bool IsLong() const noexcept
        {
            return m_Data.Short.Size == 0xff;
        }

        [[nodiscard]] inline uint32_t GetLCap() const noexcept
        {
            return m_Data.Long.Capacity;
        }

        inline void SetLCap(uint32_t capacity) noexcept
        {
            m_Data.Long.Capacity = capacity;
        }

        inline uint32_t GetSSize() const noexcept
        {
            return m_Data.Short.Size ^ (ShortCapacity - 1);
        }

        inline void SetSSize(uint32_t size) noexcept
        {
            m_Data.Short.Size = static_cast<uint8_t>(size ^ (ShortCapacity - 1));
        }

        inline uint32_t GetLSize() const noexcept
        {
            return m_Data.Long.Size;
        }

        inline void SetLSize(uint32_t size) noexcept
        {
            m_Data.Long.Size = size;
            m_Data.Long.Indicator = std::numeric_limits<uint64_t>::max();
        }

        inline void SetSize(uint32_t size) noexcept
        {
            if (IsLong())
                SetLSize(size);
            else
                SetSSize(size);
        }

        inline void Zero() noexcept
        {
            m_Data.Words[0] = 0;
            m_Data.Words[1] = 0;
            m_Data.Words[2] = 0;
            SetSSize(0);
        }

        inline static uint32_t Recommend(uint32_t s) noexcept
        {
            if (s < ShortCapacity)
                return ShortCapacity - 1;

            return AlignUp<Memory::kDefaultAlignment>(s + 1) - 1;
        }

        inline static TChar* Allocate(uint32_t s) noexcept
        {
            return static_cast<TChar*>(Memory::DefaultAllocate(s));
        }

        inline static void Deallocate(TChar* c) noexcept
        {
            Memory::DefaultFree(c);
        }

        inline static void CopyData(TChar* dest, const TChar* src, uint32_t size) noexcept
        {
            TCharTraits::copy(dest, src, size);
        }

        inline static void SetData(TChar* dest, TChar value, uint32_t size) noexcept
        {
            TCharTraits::assign(dest, size, value);
        }

        inline TChar* InitImpl(uint32_t size) noexcept
        {
            TChar* newPtr;
            if (size < ShortCapacity)
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

        inline void Init(const TChar* str, uint32_t size) noexcept
        {
            TChar* ptr = InitImpl(size);
            CopyData(ptr, str, size);
            ptr[size] = '\0';
        }

        inline void Init(uint32_t count, TChar value) noexcept
        {
            TChar* ptr = InitImpl(count);
            SetData(ptr, value, count);
            ptr[count] = '\0';
        }

        inline void GrowAndReplace(uint32_t oldCap, uint32_t deltaCap, uint32_t oldSize, uint32_t copyCount, uint32_t delCount,
                                   uint32_t addCount, const TChar* newChars)
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
            if (oldCap + 1 != ShortCapacity)
                Deallocate(oldData);
            m_Data.Long.Data = (newData);
            SetLCap(cap + 1);
            oldSize = copyCount + addCount + copySize;
            SetLSize(oldSize);
            newData[oldSize] = '\0';
        }

    public:
        using Iterator = Internal::StrIterator;

        inline String() noexcept
        {
            Zero();
        }

        inline String(const String& other) noexcept
        {
            if (!other.IsLong())
                m_Data = other.m_Data;
            else
                Init(other.m_Data.Long.Data, other.m_Data.Long.Size);
        }

        inline String& operator=(const String& other) noexcept
        {
            Clear();
            Shrink();
            if (!other.IsLong())
                m_Data = other.m_Data;
            else
                Init(other.m_Data.Long.Data, other.m_Data.Long.Size);
            return *this;
        }

        inline String(String&& other) noexcept
            : m_Data(other.m_Data)
        {
            other.Zero();
        }

        inline String& operator=(String&& other) noexcept
        {
            Clear();
            Shrink();
            m_Data = other.m_Data;
            other.Zero();
            return *this;
        }

        inline String(uint32_t length, TChar value) noexcept
        {
            Init(length, value);
        }

        inline String(const TChar* str, uint32_t byteSize) noexcept
        {
            Init(str, byteSize);
        }

        inline String(StringSlice slice) noexcept
        {
            Init(slice.Data(), slice.Size());
        }

        inline String(const TChar* str) noexcept
            : String(str, Str::ByteLength(str))
        {
        }

        inline ~String() noexcept
        {
            if (IsLong())
            {
                Deallocate(m_Data.Long.Data);
            }
        }

        [[nodiscard]] inline const TChar* Data() const noexcept
        {
            return IsLong() ? m_Data.Long.Data : m_Data.Short.Data;
        }

        inline TChar* Data() noexcept
        {
            return IsLong() ? m_Data.Long.Data : m_Data.Short.Data;
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
            if (IsLong())
                return Str::CodepointAt(m_Data.Long.Data, m_Data.Long.Size, index);

            return Str::CodepointAt(m_Data.Short.Data, GetSSize(), index);
        }

        // O(1)
        [[nodiscard]] inline uint32_t Size() const noexcept
        {
            return IsLong() ? GetLSize() : GetSSize();
        }

        [[nodiscard]] inline bool Empty() const noexcept
        {
            return Size() == 0;
        }

        // O(1)
        [[nodiscard]] inline uint32_t Capacity() const noexcept
        {
            return (IsLong() ? GetLCap() : ShortCapacity) - 1;
        }

        // O(N)
        [[nodiscard]] inline uint32_t Length() const noexcept
        {
            return UTF8::Length(Data(), Size());
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
            return StringSlice(begin, static_cast<uint32_t>(end - begin));
        }

        [[nodiscard]] inline StringSlice ASCIISubstring(uint32_t beginIndex, uint32_t length) const
        {
            auto begin = Data() + beginIndex;
            auto end = Data() + beginIndex + length;
            return StringSlice(begin, static_cast<uint32_t>(end - begin));
        }

        inline void Reserve(uint32_t reserve) noexcept
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

        inline void Shrink() noexcept
        {
            const uint32_t cap = Capacity();
            const uint32_t size = Size();
            const uint32_t reserve = Recommend(size);
            if (reserve == cap)
                return;

            if (reserve == ShortCapacity - 1)
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

        inline void Clear() noexcept
        {
            *Data() = '\0';
            SetSize(0);
        }

        inline String& Append(const TChar* str, uint32_t count)
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

        inline String& Append(StringSlice str)
        {
            return Append(str.Data(), str.Size());
        }

        inline String& Append(TChar cp)
        {
            return Append(&cp, 1); // TODO: optimize this for single chars
        }

        inline String& Append(const TChar* str)
        {
            return Append(str, Str::ByteLength(str));
        }

        inline String& operator+=(StringSlice str)
        {
            return Append(str);
        }

        inline String& operator/=(StringSlice str)
        {
            if (Data()[Size() - 1] != '/')
                Append('/');
            return Append(str);
        }

        inline friend String operator+(const String& lhs, StringSlice rhs)
        {
            String t;
            t.Reserve(lhs.Size() + rhs.Size() + 1);
            t += lhs;
            t += rhs;
            return t;
        }

        inline friend String operator/(const String& lhs, StringSlice rhs)
        {
            String t;
            t.Reserve(lhs.Size() + rhs.Size() + 2);
            t += lhs;
            t /= rhs;
            return t;
        }

        [[nodiscard]] inline Iterator FindFirstOf(Iterator start, TCodepoint search) const noexcept
        {
            return StringSlice(Data(), Size()).FindFirstOf(start.m_Iter, search).m_Iter;
        }

        [[nodiscard]] inline Iterator FindFirstOf(TCodepoint search) const noexcept
        {
            return StringSlice(Data(), Size()).FindFirstOf(search).m_Iter;
        }

        [[nodiscard]] inline Iterator FindLastOf(TCodepoint search) const noexcept
        {
            return StringSlice(Data(), Size()).FindLastOf(search).m_Iter;
        }

        [[nodiscard]] inline festd::pmr::vector<StringSlice> Split(TCodepoint c = ' ',
                                                                   std::pmr::memory_resource* pAllocator = nullptr) const
        {
            return StringSlice(Data(), Size()).Split(c, pAllocator);
        }

        [[nodiscard]] inline festd::pmr::vector<StringSlice> SplitLines(std::pmr::memory_resource* pAllocator = nullptr) const
        {
            return StringSlice(Data(), Size()).SplitLines(pAllocator);
        }

        [[nodiscard]] inline StringSlice StripRight(StringSlice chars = "\n\r\t ") const noexcept
        {
            return StringSlice(Data(), Size()).StripRight(chars);
        }

        [[nodiscard]] inline StringSlice StripLeft(StringSlice chars = "\n\r\t ") const noexcept
        {
            return StringSlice(Data(), Size()).StripLeft(chars);
        }

        [[nodiscard]] inline StringSlice Strip(StringSlice chars = "\n\r\t ") const noexcept
        {
            return StringSlice(Data(), Size()).Strip(chars);
        }

        [[nodiscard]] inline int32_t Compare(StringSlice other) const noexcept
        {
            return UTF8::Compare(Data(), other.Data(), Size(), other.Size());
        }

        [[nodiscard]] inline bool IsEqualTo(StringSlice other, bool caseSensitive = true) const noexcept
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

        template<class T>
        [[nodiscard]] inline Result<T, ParseError> Parse()
        {
            return StringSlice(Data(), Size()).Parse<T>();
        }

        [[nodiscard]] inline static String Join(StringSlice separator, const festd::span<StringSlice>& strings)
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

        [[nodiscard]] inline explicit operator UUID() const noexcept
        {
            return UUID(Data());
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
