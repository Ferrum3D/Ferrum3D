#pragma once
#include <FeCore/Modules/Environment.h>
#include <festd/Internal/StringStorageImpl.h>
#include <initializer_list>
#include <type_traits>

namespace FE::Internal
{
    [[nodiscard]] constexpr bool IsConstantEvaluated()
    {
#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
        return __builtin_is_constant_evaluated();
#else
        return false;
#endif
    }


    FE_FORCE_INLINE void ValidateStringBytes(const char* str, const uint32_t byteSize)
    {
#if FE_DEBUG
        FE_AssertDebug(str != nullptr || byteSize == 0, "String data must not be null");
        FE_AssertDebug(UTF8::IsValid(str, byteSize), "String data must be valid UTF-8");
        FE_AssertDebug(byteSize == 0 || memchr(str, '\0', byteSize) == nullptr, "String data must not contain embedded zeros");
#else
        (void)str;
        (void)byteSize;
#endif
    }


    FE_FORCE_INLINE void ValidateStringByte(const char byte)
    {
#if FE_DEBUG
        FE_AssertDebug(byte != '\0', "String data must not contain embedded zeros");
        FE_AssertDebug(static_cast<uint8_t>(byte) < 0x80, "Single-byte string input must be ASCII");
#else
        (void)byte;
#endif
    }


    FE_FORCE_INLINE void ValidateStringCodepoint(const int32_t codepoint)
    {
#if FE_DEBUG
        char bytes[4];
        FE_AssertDebug(codepoint != 0, "String data must not contain embedded zeros");
        FE_AssertDebug(UTF8::Encode(codepoint, bytes) > 0, "String codepoint must be valid UTF-8");
#else
        (void)codepoint;
#endif
    }


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
            if (!IsConstantEvaluated())
                ValidateStringBytes(str, byteSize);
        }

        constexpr BasicStringViewImpl(const char* str)
            : m_data(str)
            , m_size(ASCII::Length(str))
        {
            if (!IsConstantEvaluated())
                ValidateStringBytes(str, m_size);
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
        using size_type = uint32_t;
        using difference_type = std::ptrdiff_t;
        using value_type = char;
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

        template<class = std::enable_if_t<TStorage::kHasAllocator>>
        BasicStringImpl(const uint32_t length, const char value, std::pmr::memory_resource* allocator)
            : TStorage(allocator)
        {
            if (length != 0)
                ValidateStringByte(value);
            char* data = TStorage::InitializeImpl(length, TStorage::GetAllocator());
            memset(data, value, length);
            data[length] = '\0';
        }

        template<class = std::enable_if_t<TStorage::kHasAllocator>>
        BasicStringImpl(const char* str, const uint32_t byteSize, std::pmr::memory_resource* allocator)
            : TStorage(allocator)
        {
            ValidateStringBytes(str, byteSize);
            char* data = TStorage::InitializeImpl(byteSize, TStorage::GetAllocator());
            memcpy(data, str, byteSize);
            data[byteSize] = '\0';
        }

        template<class = std::enable_if_t<TStorage::kHasAllocator>>
        BasicStringImpl(const char* str, std::pmr::memory_resource* allocator)
            : BasicStringImpl(str, ASCII::Length(str), allocator)
        {
        }

        template<class = std::enable_if_t<TStorage::kHasAllocator>>
        BasicStringImpl(std::initializer_list<char> chars, std::pmr::memory_resource* allocator)
            : BasicStringImpl(chars.begin(), static_cast<uint32_t>(chars.size()), allocator)
        {
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

        BasicStringImpl& operator=(const char* str)
        {
            assign(str);
            return *this;
        }

        BasicStringImpl& operator=(const char value)
        {
            assign(1, value);
            return *this;
        }

        BasicStringImpl& operator=(const std::initializer_list<char> chars)
        {
            assign(chars);
            return *this;
        }

        BasicStringImpl(const uint32_t length, const char value)
        {
            if (length != 0)
                ValidateStringByte(value);
            char* data = TStorage::InitializeImpl(length, TStorage::GetAllocator());
            memset(data, value, length);
            data[length] = '\0';
        }

        BasicStringImpl(const char* str, uint32_t byteSize)
        {
            ValidateStringBytes(str, byteSize);
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

        BasicStringImpl(std::initializer_list<char> chars)
            : BasicStringImpl(chars.begin(), static_cast<uint32_t>(chars.size()))
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
            if (byteSize > size())
                ValidateStringByte(value);
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
            ValidateStringBytes(str, byteSize);
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

        void assign(const char* str)
        {
            assign(str, ASCII::Length(str));
        }

        void assign(const BasicStringViewImpl& str)
        {
            assign(str.data(), str.size());
        }

        void assign(const uint32_t length, const char value)
        {
            if (length != 0)
                ValidateStringByte(value);
            resize_uninitialized(length);
            char* bytes = data();
            memset(bytes, value, length);
            bytes[length] = '\0';
        }

        void assign(const std::initializer_list<char> chars)
        {
            assign(chars.begin(), static_cast<uint32_t>(chars.size()));
        }

        void append(const char* str, const uint32_t byteSize)
        {
            ValidateStringBytes(str, byteSize);
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

        void append(const BasicStringViewImpl& str)
        {
            append(str.data(), str.size());
        }

        void append(const Iter begin, const Iter end)
        {
            append(begin.m_iter, static_cast<uint32_t>(end.m_iter - begin.m_iter));
        }

        void append(const uint32_t length, const char value)
        {
            if (length != 0)
                ValidateStringByte(value);
            const uint32_t oldSize = size();
            resize_uninitialized(oldSize + length);
            char* bytes = data();
            memset(bytes + oldSize, value, length);
            bytes[oldSize + length] = '\0';
        }

        void append(const std::initializer_list<char> chars)
        {
            append(chars.begin(), static_cast<uint32_t>(chars.size()));
        }

        void append(const int32_t codepoint)
        {
            ValidateStringCodepoint(codepoint);
            char bytes[4];
            const int32_t bytesWritten = UTF8::Encode(codepoint, bytes);
            FE_AssertDebug(bytesWritten > 0, "String codepoint must be valid UTF-8");
            append(bytes, bytesWritten);
        }

        void push_back(const char byte)
        {
            ValidateStringByte(byte);
            const uint32_t oldSize = size();
            resize_uninitialized(oldSize + 1);
            char* bytes = data();
            bytes[oldSize] = byte;
            bytes[oldSize + 1] = '\0';
        }

        uint32_t copy(char* destination, const uint32_t byteSize, const uint32_t byteOffset = 0) const
        {
            FE_AssertDebug(destination != nullptr || byteSize == 0, "Copy destination must not be null");
            const uint32_t currentSize = size();
            FE_AssertDebug(byteOffset <= currentSize, "String copy offset is out of range");
            if (byteOffset > currentSize)
                return 0;

            const uint32_t bytesToCopy = Math::Min(byteSize, currentSize - byteOffset);
            if (bytesToCopy != 0)
                memcpy(destination, data() + byteOffset, bytesToCopy);

            return bytesToCopy;
        }

        void insert(const Iter position, const char* str, const uint32_t byteSize)
        {
            ValidateStringBytes(str, byteSize);
            const uint32_t oldSize = size();
            const char* oldData = data();
            const uintptr_t oldDataAddress = reinterpret_cast<uintptr_t>(oldData);
            const uintptr_t positionAddress = reinterpret_cast<uintptr_t>(position.m_iter);
            FE_AssertDebug(positionAddress >= oldDataAddress && positionAddress <= oldDataAddress + oldSize,
                           "String insert position is out of range");
            if (positionAddress < oldDataAddress || positionAddress > oldDataAddress + oldSize)
                return;

            const uint32_t positionOffset = static_cast<uint32_t>(positionAddress - oldDataAddress);
            const uintptr_t sourceAddress = reinterpret_cast<uintptr_t>(str);
            const bool overlaps = byteSize != 0 && sourceAddress >= oldDataAddress && sourceAddress < oldDataAddress + oldSize;

            std::pmr::memory_resource* allocator = TStorage::GetAllocator();
            char* temporary = nullptr;
            if (overlaps)
            {
                temporary = Memory::AllocateArray<char>(allocator, byteSize);
                memcpy(temporary, str, byteSize);
                str = temporary;
            }

            resize_uninitialized(oldSize + byteSize);
            char* bytes = data();
            memmove(bytes + positionOffset + byteSize, bytes + positionOffset, oldSize - positionOffset);
            if (byteSize != 0)
                memcpy(bytes + positionOffset, str, byteSize);
            bytes[oldSize + byteSize] = '\0';

            if (temporary != nullptr)
                allocator->deallocate(temporary, byteSize, alignof(char));
        }

        void insert(const Iter position, const BasicStringViewImpl& str)
        {
            insert(position, str.data(), str.size());
        }

        Iter insert(const Iter position, const char byte)
        {
            const uintptr_t dataAddress = reinterpret_cast<uintptr_t>(data());
            const uintptr_t positionAddress = reinterpret_cast<uintptr_t>(position.m_iter);
            const uint32_t positionOffset = static_cast<uint32_t>(positionAddress - dataAddress);
            insert(position, &byte, 1);
            return Iter{ data() + positionOffset };
        }

        void insert(const Iter position, const uint32_t length, const char value)
        {
            if (length == 0)
                return;

            ValidateStringByte(value);
            const uint32_t oldSize = size();
            const char* oldData = data();
            const uintptr_t oldDataAddress = reinterpret_cast<uintptr_t>(oldData);
            const uintptr_t positionAddress = reinterpret_cast<uintptr_t>(position.m_iter);
            FE_AssertDebug(positionAddress >= oldDataAddress && positionAddress <= oldDataAddress + oldSize,
                           "String insert position is out of range");
            if (positionAddress < oldDataAddress || positionAddress > oldDataAddress + oldSize)
                return;

            const uint32_t positionOffset = static_cast<uint32_t>(positionAddress - oldDataAddress);
            resize_uninitialized(oldSize + length);
            char* bytes = data();
            memmove(bytes + positionOffset + length, bytes + positionOffset, oldSize - positionOffset);
            memset(bytes + positionOffset, value, length);
            bytes[oldSize + length] = '\0';
        }

        void insert(const Iter position, const std::initializer_list<char> chars)
        {
            insert(position, chars.begin(), static_cast<uint32_t>(chars.size()));
        }

        Iter erase(const Iter first, const Iter last)
        {
            const uint32_t oldSize = size();
            char* bytes = data();
            const uintptr_t dataAddress = reinterpret_cast<uintptr_t>(bytes);
            const uintptr_t firstAddress = reinterpret_cast<uintptr_t>(first.m_iter);
            const uintptr_t lastAddress = reinterpret_cast<uintptr_t>(last.m_iter);
            FE_AssertDebug(firstAddress >= dataAddress && firstAddress <= lastAddress && lastAddress <= dataAddress + oldSize,
                           "String erase range is out of range");
            if (firstAddress < dataAddress || firstAddress > lastAddress || lastAddress > dataAddress + oldSize)
                return Iter{ bytes + oldSize };

            const uint32_t firstOffset = static_cast<uint32_t>(firstAddress - dataAddress);
            const uint32_t lastOffset = static_cast<uint32_t>(lastAddress - dataAddress);
            memmove(bytes + firstOffset, bytes + lastOffset, oldSize - lastOffset);
            resize_uninitialized(oldSize - (lastOffset - firstOffset));
            return Iter{ data() + firstOffset };
        }

        Iter erase(const Iter position)
        {
            Iter next = position;
            ++next;
            return erase(position, next);
        }

        void replace(const Iter first, const Iter last, const char* str, const uint32_t byteSize)
        {
            ValidateStringBytes(str, byteSize);
            const char* oldData = data();
            const uint32_t oldSize = size();
            const uintptr_t oldDataAddress = reinterpret_cast<uintptr_t>(oldData);
            const uintptr_t sourceAddress = reinterpret_cast<uintptr_t>(str);
            const bool overlaps = byteSize != 0 && sourceAddress >= oldDataAddress && sourceAddress < oldDataAddress + oldSize;

            std::pmr::memory_resource* allocator = TStorage::GetAllocator();
            char* temporary = nullptr;
            if (overlaps)
            {
                temporary = Memory::AllocateArray<char>(allocator, byteSize);
                memcpy(temporary, str, byteSize);
                str = temporary;
            }

            const Iter insertionPosition = erase(first, last);
            insert(insertionPosition, str, byteSize);

            if (temporary != nullptr)
                allocator->deallocate(temporary, byteSize, alignof(char));
        }

        void replace(const Iter first, const Iter last, const BasicStringViewImpl& str)
        {
            replace(first, last, str.data(), str.size());
        }

        void replace(const Iter first, const Iter last, const uint32_t length, const char value)
        {
            const Iter insertionPosition = erase(first, last);
            insert(insertionPosition, length, value);
        }

        void replace(const Iter first, const Iter last, const std::initializer_list<char> chars)
        {
            replace(first, last, chars.begin(), static_cast<uint32_t>(chars.size()));
        }

        void pop_back()
        {
            FE_AssertDebug(size() != 0, "Cannot pop from an empty string");

            Iter last{ data() + size() };
            --last;
            erase(last, Iter{ data() + size() });
        }

        void swap(BasicStringImpl& other) noexcept
        {
            if (this == &other)
                return;

            BasicStringImpl temporary = std::move(other);
            other = std::move(*this);
            *this = std::move(temporary);
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

        [[nodiscard]] uint32_t max_size() const
        {
            return Constants::kMaxU32 - 1;
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
        using TBase::operator=;

        using size_type = uint32_t;
        using difference_type = std::ptrdiff_t;
        using value_type = char;
        static constexpr size_type npos = kInvalidIndex;

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

        template<class TOtherBase, class T = TBase,
                 class = decltype(std::declval<T&>().assign(std::declval<const char*>(), uint32_t{}))>
        StringImpl& operator=(const StringImpl<TOtherBase>& other)
        {
            TBase::assign(other.data(), other.size());
            return *this;
        }

        template<class T = TBase, class = decltype(std::declval<T&>().append(std::declval<const char*>()))>
        StringImpl& operator+=(const char* str)
        {
            TBase::append(str);
            return *this;
        }

        template<class T = TBase, class = decltype(std::declval<T&>().push_back(char{}))>
        StringImpl& operator+=(const char byte)
        {
            TBase::push_back(byte);
            return *this;
        }

        template<class T = TBase, class = decltype(std::declval<T&>().append(std::initializer_list<char>{}))>
        StringImpl& operator+=(std::initializer_list<char> chars)
        {
            TBase::append(chars);
            return *this;
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

        [[nodiscard]] uint32_t length() const
        {
            return UTF8::Length(TBase::data(), TBase::size());
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
            return find(begin(), str);
        }

        [[nodiscard]] Iter find(const Iter position, const StringImpl<BasicStringViewImpl> str) const
        {
            const uint32_t byteSize = TBase::size();
            const uint32_t otherByteSize = str.size();
            if (otherByteSize == 0)
                return position;

            if (otherByteSize > byteSize)
                return end();

            const char* data = TBase::data();
            const uintptr_t dataAddress = reinterpret_cast<uintptr_t>(data);
            const uintptr_t positionAddress = reinterpret_cast<uintptr_t>(position.m_iter);
            FE_AssertDebug(positionAddress >= dataAddress && positionAddress <= dataAddress + byteSize,
                           "String find position is out of range");
            if (positionAddress < dataAddress || positionAddress > dataAddress + byteSize)
                return end();

            const uint32_t startOffset = static_cast<uint32_t>(positionAddress - dataAddress);
            if (otherByteSize > byteSize - startOffset)
                return end();

            const char* otherData = str.data();
            for (uint32_t i = startOffset; i < byteSize - otherByteSize + 1; ++i)
            {
                if (memcmp(data + i, otherData, otherByteSize) == 0)
                    return Iter{ data + i };
            }

            return end();
        }

        [[nodiscard]] Iter rfind(const StringImpl<BasicStringViewImpl> str) const
        {
            return rfind(end(), str);
        }

        [[nodiscard]] Iter rfind(const Iter position, const StringImpl<BasicStringViewImpl> str) const
        {
            const uint32_t byteSize = TBase::size();
            const uint32_t otherByteSize = str.size();
            if (otherByteSize == 0)
                return position;

            if (otherByteSize > byteSize)
                return end();

            const char* data = TBase::data();
            const uintptr_t dataAddress = reinterpret_cast<uintptr_t>(data);
            const uintptr_t positionAddress = reinterpret_cast<uintptr_t>(position.m_iter);
            FE_AssertDebug(positionAddress >= dataAddress && positionAddress <= dataAddress + byteSize,
                           "String rfind position is out of range");
            if (positionAddress < dataAddress || positionAddress > dataAddress + byteSize)
                return end();

            uint32_t offset = Math::Min(static_cast<uint32_t>(positionAddress - dataAddress), byteSize - otherByteSize);
            const char* otherData = str.data();
            for (;;)
            {
                if (memcmp(data + offset, otherData, otherByteSize) == 0)
                    return Iter{ data + offset };

                if (offset == 0)
                    break;

                --offset;
            }

            return end();
        }

        [[nodiscard]] Iter find_first_not_of(Iter position, const int32_t codepoint) const
        {
            while (position != end())
            {
                if (*position != codepoint)
                    break;
                ++position;
            }

            return position;
        }

        [[nodiscard]] Iter find_first_not_of(const int32_t codepoint) const
        {
            return find_first_not_of(begin(), codepoint);
        }

        [[nodiscard]] Iter find_last_not_of(Iter position, const int32_t codepoint) const
        {
            const Iter first = begin();
            while (position != first)
            {
                --position;
                if (*position != codepoint)
                    return position;
            }

            return end();
        }

        [[nodiscard]] Iter find_last_not_of(const int32_t codepoint) const
        {
            return find_last_not_of(end(), codepoint);
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
    } // namespace pmr

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
