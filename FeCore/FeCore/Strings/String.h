#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Memory/Allocator.h>
#include <FeCore/Memory/HeapAllocator.h>
#include <FeCore/Strings/StringSlice.h>

namespace FE
{
    //! \brief String class that uses \ref HeapAllocator and UTF-8 encoding.
    class String final
    {
        struct LongMode
        {
            size_t Cap;
            size_t Size;
            TChar* Data;
        };

        inline static constexpr size_t MinCapacity = (sizeof(LongMode) - 1) / sizeof(TChar);

        struct ShortMode
        {
            union
            {
                UInt8 Size;
                [[maybe_unused]] TChar Lx;
            };
            TChar Data[MinCapacity];
        };

        union Ulx
        {
            [[maybe_unused]] LongMode Lm;
            [[maybe_unused]] ShortMode Sm;
        };

        inline static constexpr size_t WordCount = sizeof(Ulx) / sizeof(size_t);

        struct Rep
        {
            union
            {
                LongMode L;
                ShortMode S;
                size_t R[WordCount];
            };
        } m_Data;

        [[nodiscard]] inline bool IsLong() const noexcept
        {
            return m_Data.S.Size & 1;
        }

        [[nodiscard]] inline size_t GetLCap() const noexcept
        {
            return m_Data.L.Cap & static_cast<size_t>(~1);
        }

        inline void SetLCap(size_t cap) noexcept
        {
            m_Data.L.Cap = cap | 1;
        }

        inline void SetSSize(size_t size) noexcept
        {
            m_Data.S.Size = static_cast<UInt8>(size << 1);
        }

        inline void SetLSize(size_t size) noexcept
        {
            m_Data.L.Size = size;
        }

        inline void SetSize(size_t size) noexcept
        {
            if (IsLong())
                SetLSize(size);
            else
                SetSSize(size);
        }

        inline void Zero() noexcept
        {
            for (auto& i : m_Data.R)
            {
                i = 0;
            }
        }

        inline static constexpr size_t Alignment = 16;

        inline static size_t Recommend(size_t s) noexcept
        {
            if (s < MinCapacity)
            {
                return MinCapacity - 1;
            }

            size_t guess = FE::AlignUp<Alignment / sizeof(TChar)>(s + 1) - 1;

            if (guess == MinCapacity)
            {
                ++guess;
            }

            return guess;
        }

        inline static TChar* Allocate(size_t s) noexcept
        {
            FE_STATIC_SRCPOS(position);
            return static_cast<TChar*>(GlobalAllocator<HeapAllocator>::Get().Allocate(s, Alignment, position));
        }

        inline static void Deallocate(TChar* c, size_t s) noexcept
        {
            FE_STATIC_SRCPOS(position);
            GlobalAllocator<HeapAllocator>::Get().Deallocate(c, position, s);
        }

        inline static void CopyData(TChar* dest, const TChar* src, size_t size) noexcept
        {
            TCharTraits::copy(dest, src, size);
        }

        inline static void SetData(TChar* dest, TChar value, size_t size) noexcept
        {
            TCharTraits::assign(dest, size, value);
        }

        inline TChar* InitImpl(size_t size) noexcept
        {
            TChar* newPtr;
            if (size < MinCapacity)
            {
                SetSSize(size);
                newPtr = m_Data.S.Data;
            }
            else
            {
                size_t newCap = Recommend(size);
                newPtr        = Allocate(newCap + 1);
                m_Data.L.Data = newPtr;
                SetLCap(newCap + 1);
                SetLSize(size);
            }

            return newPtr;
        }

        inline void Init(const TChar* str, size_t size) noexcept
        {
            TChar* ptr = InitImpl(size);
            CopyData(ptr, str, size);
            ptr[size] = '\0';
        }

        inline void Init(size_t count, TChar value) noexcept
        {
            TChar* ptr = InitImpl(count);
            SetData(ptr, value, count);
            ptr[count] = '\0';
        }

        inline void GrowAndReplace(
            size_t oldCap, size_t deltaCap, size_t oldSize, size_t copyCount, size_t delCount, size_t addCount,
            const TChar* newChars)
        {
            TChar* oldData = Data();
            size_t cap     = Recommend(std::max(oldCap + deltaCap, 2 * oldCap));
            TChar* newData = Allocate(cap + 1);
            if (copyCount)
                CopyData(newData, oldData, copyCount);
            if (addCount)
                CopyData(newData + copyCount, newChars, addCount);
            size_t copySize = oldSize - delCount - copyCount;
            if (copySize)
                CopyData(newData + copyCount + addCount, oldData + delCount, copySize);
            if (oldCap + 1 != MinCapacity)
                Deallocate(oldData, oldCap);
            m_Data.L.Data = (newData);
            SetLCap(cap + 1);
            oldSize = copyCount + addCount + copySize;
            SetLSize(oldSize);
            newData[oldSize] = '\0';
        }

    public:
        FE_STRUCT_RTTI(String, "340DE9B8-EC27-4248-9E0D-7D40330E8EBA");

        class Iterator
        {
            const TChar* m_Iter;

        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = TCodepoint;
            using pointer           = const TCodepoint*;
            using reference         = const TCodepoint&;

            inline Iterator(const TChar* iter)
                : m_Iter(iter)
            {
            }

            inline value_type operator*() const
            {
                return UTF8::PeekDecode(m_Iter);
            }

            inline Iterator& operator++()
            {
                UTF8::Decode(m_Iter);
                return *this;
            }

            inline Iterator operator++(int)
            {
                Iterator t = *this;
                ++(*this);
                return t;
            }

            inline Iterator& operator--()
            {
                UTF8::DecodePrior(m_Iter);
                return *this;
            }

            inline Iterator operator--(int)
            {
                Iterator t = *this;
                UTF8::DecodePrior(m_Iter);
                return t;
            }

            inline friend bool operator==(const Iterator& a, const Iterator& b)
            {
                return a.m_Iter == b.m_Iter;
            };

            inline friend bool operator!=(const Iterator& a, const Iterator& b)
            {
                return a.m_Iter != b.m_Iter;
            };
        };

        inline String() noexcept
        {
            Zero();
        }

        inline String(const String& other) noexcept
        {
            if (!other.IsLong())
                m_Data = other.m_Data;
            else
                Init(other.m_Data.L.Data, other.m_Data.L.Size);
        }

        inline String& operator=(const String& other) noexcept
        {
            if (!other.IsLong())
                m_Data = other.m_Data;
            else
                Init(other.m_Data.L.Data, other.m_Data.L.Size);
            return *this;
        }

        inline String(String&& other) noexcept
            : m_Data(other.m_Data)
        {
            other.Zero();
        }

        inline String& operator=(String&& other) noexcept
        {
            m_Data = other.m_Data;
            other.Zero();
            return *this;
        }

        inline String(size_t length, TChar value) noexcept
        {
            Init(length, value);
        }

        inline String(const TChar* str, size_t byteSize) noexcept
        {
            Init(str, byteSize);
        }

        inline String(StringSlice slice) noexcept
        {
            Init(slice.Data(), slice.Size());
        }

        inline String(const TChar* str) noexcept
            : String(str, TCharTraits::length(str))
        {
        }

        inline ~String() noexcept
        {
            if (IsLong())
            {
                Deallocate(m_Data.L.Data, GetLCap());
            }
        }

        inline const TChar* Data() const noexcept
        {
            return IsLong() ? m_Data.L.Data : m_Data.S.Data;
        }

        inline TChar* Data() noexcept
        {
            return IsLong() ? m_Data.L.Data : m_Data.S.Data;
        }

        // O(1)
        [[nodiscard]] inline TChar ByteAt(size_t index) const
        {
            FE_CORE_ASSERT(index < Size(), "Invalid index");
            return Data()[index];
        }

        // O(N)
        [[nodiscard]] inline TCodepoint CodePointAt(size_t index) const
        {
            auto begin     = Data();
            auto end       = begin + Size() + 1;
            size_t cpIndex = 0;
            for (auto iter = begin; iter != end; UTF8::Decode(iter), ++cpIndex)
            {
                if (cpIndex == index)
                {
                    return UTF8::PeekDecode(iter);
                }
            }

            FE_CORE_ASSERT(false, "Invalid index");
            return 0;
        }

        // O(1)
        [[nodiscard]] inline size_t Size() const noexcept
        {
            return IsLong() ? m_Data.L.Size : m_Data.S.Size >> 1;
        }

        [[nodiscard]] inline bool Empty() const noexcept
        {
            return Size() == 0;
        }

        // O(1)
        [[nodiscard]] inline size_t Capacity() const noexcept
        {
            return (IsLong() ? GetLCap() : MinCapacity) - 1;
        }

        // O(N)
        [[nodiscard]] inline size_t Length() const noexcept
        {
            auto ptr = Data();
            return UTF8::Length(ptr, Size());
        }

        inline operator StringSlice() const noexcept
        {
            return StringSlice(Data(), Size());
        }

        inline StringSlice operator()(size_t beginIndex, size_t endIndex) const
        {
            auto begin = Data();
            auto end   = Data();
            UTF8::Advance(begin, beginIndex);
            UTF8::Advance(end, endIndex);
            return StringSlice(begin, end - begin);
        }

        inline void Reserve(size_t reserve) noexcept
        {
            size_t cap = Capacity();
            if (cap >= reserve)
                return;

            reserve        = Recommend(reserve);
            TChar* newData = Allocate(reserve + 1);
            TChar* oldData = Data();
            CopyData(newData, oldData, Size() + 1);
            if (IsLong())
                Deallocate(oldData, cap);

            SetLCap(reserve + 1);
            SetLSize(Size());
            m_Data.L.Data = newData;
        }

        inline void Shrink() noexcept
        {
            size_t cap     = Capacity();
            size_t size    = Size();
            size_t reserve = Recommend(size);
            if (reserve == cap)
                return;

            if (reserve == MinCapacity - 1)
            {
                TChar* newData = m_Data.S.Data;
                TChar* oldData = m_Data.L.Data;

                CopyData(newData, oldData, size + 1);
                Deallocate(oldData, cap);
                SetSSize(size);
            }
            else
            {
                TChar* newData = Allocate(reserve + 1);
                TChar* oldData = m_Data.L.Data;

                CopyData(newData, oldData, size + 1);
                Deallocate(oldData, cap);

                SetLCap(reserve + 1);
                SetLSize(size);
                m_Data.L.Data = newData;
            }
        }

        inline void Clear() noexcept
        {
            *Data() = '\0';
            SetSize(0);
        }

        inline String& Append(const TChar* str, size_t count)
        {
            assert(count == 0 || str != nullptr);
            if (count == 0)
                return *this;
            size_t size = Size();
            size_t cap  = Capacity();
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
            return Append(str, std::char_traits<TChar>::length(str));
        }

        inline String& Append(const String& other)
        {
            return Append(other.Data(), other.Size());
        }

        inline String& operator+=(const TChar* str)
        {
            return Append(str);
        }

        inline String& operator+=(const String& str)
        {
            return Append(str);
        }

        inline friend String operator+(const String& lhs, const TChar* rhs)
        {
            String t;
            t.Reserve(lhs.Size() + std::char_traits<TChar>::length(rhs));
            t += lhs;
            t += rhs;
            return t;
        }

        inline friend String operator+(const String& lhs, const String& rhs)
        {
            String t;
            t.Reserve(lhs.Size() + rhs.Size());
            t += lhs;
            t += rhs;
            return t;
        }

        [[nodiscard]] inline Int32 Compare(const StringSlice& other) const noexcept
        {
            return UTF8::Compare(Data(), other.Data(), Size(), other.Size());
        }

        [[nodiscard]] inline Iterator begin() const noexcept
        {
            auto ptr = Data();
            return Iterator(ptr);
        }

        [[nodiscard]] inline Iterator end() const noexcept
        {
            auto ptr  = Data();
            auto size = Size();
            return Iterator(ptr + size);
        }

        [[nodiscard]] inline Iterator cbegin() const noexcept
        {
            auto ptr = Data();
            return Iterator(ptr);
        }

        [[nodiscard]] inline Iterator cend() const noexcept
        {
            auto ptr  = Data();
            auto size = Size();
            return Iterator(ptr + size);
        }
    };
} // namespace FE
