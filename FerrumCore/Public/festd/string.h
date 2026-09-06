#pragma once
#include <Core/Env/Environment.h>
#include <concepts>
#include <festd/Internal/StringStorageImpl.h>
#include <initializer_list>
#include <type_traits>

namespace FE::Internal
{
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


    //! @brief Non-owning view over a UTF-8 encoded string.
    //!
    //! The view stores a byte pointer and a byte count. It does not own the referenced memory and does not require the
    //! referenced range to be null-terminated. Public aliases expose this type as `festd::string_view`.
    //!
    //! The range must contain valid UTF-8 and must not contain embedded zero bytes. These preconditions are checked by debug
    //! assertions for runtime construction. Unlike `std::string_view`, iteration is over decoded Unicode codepoints rather
    //! than raw bytes, and substring helpers that do not contain `ascii` in their name use codepoint positions.
    struct BasicStringViewImpl
    {
        //! @brief Create an empty view.
        constexpr BasicStringViewImpl()
            : m_data(nullptr)
            , m_size(0)
        {
        }

        //! @brief Create a view over a UTF-8 byte range.
        //!
        //! @param str      The first byte of the string, or `nullptr` when `byteSize` is zero.
        //! @param byteSize The number of bytes in the viewed range, excluding any null terminator.
        constexpr BasicStringViewImpl(const char* str, const uint32_t byteSize)
            : m_data(str)
            , m_size(byteSize)
        {
            if (!std::is_constant_evaluated())
                ValidateStringBytes(str, byteSize);
        }

        //! @brief Create a view over a null-terminated UTF-8 string.
        constexpr BasicStringViewImpl(const char* str)
            : m_data(str)
            , m_size(ASCII::Length(str))
        {
            if (!std::is_constant_evaluated())
                ValidateStringBytes(str, m_size);
        }

        //! @brief Create a view from a pair of codepoint iterators.
        BasicStringViewImpl(const StrIterator begin, const StrIterator end)
            : BasicStringViewImpl(reinterpret_cast<const char*>(begin.m_iter), static_cast<uint32_t>(end.m_iter - begin.m_iter))
        {
        }

        //! @brief Create a view from a pair of byte pointers.
        BasicStringViewImpl(const char* begin, const char* end)
            : BasicStringViewImpl(begin, static_cast<uint32_t>(end - begin))
        {
        }

        //! @brief Get the size of the string in bytes.
        [[nodiscard]] constexpr uint32_t size() const
        {
            return m_size;
        }

        //! @brief Get the first byte of the viewed string.
        [[nodiscard]] constexpr const char* data() const
        {
            return m_data;
        }

    private:
        const char* m_data;
        uint32_t m_size;
    };


    //! @brief Owning UTF-8 string implementation used by public string aliases.
    //!
    //! Public aliases such as `festd::string`, `festd::fixed_string`, `festd::inline_string`, and their pmr variants expose
    //! this API. `size()`, `capacity()`, `reserve()`, and raw range overloads are measured in bytes. `length()`, `substr()`,
    //! and `StrIterator` traversal are measured in Unicode codepoints.
    //!
    //! Safe constructors and mutating functions expect valid UTF-8 and no embedded zero bytes; debug builds assert these
    //! preconditions. `reinitialize()` and `resize_uninitialized()` are raw APIs and leave it to the caller to write valid,
    //! null-terminated UTF-8 before the string is observed again.
    //!
    //! This is intentionally not a drop-in `std::string` replacement. It has no `operator[]`, `at`, `front`, `back`, standard
    //! byte iterators, or `resize(count)` overload. Use `byte_at()` for byte access and `codepoint_at()` or iterators for
    //! decoded codepoints.
    template<class TStorage>
    struct BasicStringImpl : private TStorage
    {
        using size_type = uint32_t;
        using difference_type = std::ptrdiff_t;
        using value_type = char;
        using Iter = StrIterator;

        //! @brief Create an empty string.
        BasicStringImpl()
        {
            if (char* data = TStorage::InitializeImpl(0, TStorage::GetAllocator()))
                data[0] = '\0';
        }

        //! @brief Create an empty string using a polymorphic allocator.
        template<class T = TStorage>
            requires T::kHasAllocator
        BasicStringImpl(std::pmr::memory_resource* allocator)
            : TStorage(allocator)
        {
            if (char* data = TStorage::InitializeImpl(0, TStorage::GetAllocator()))
                data[0] = '\0';
        }

        //! @brief Create a string containing `length` copies of an ASCII byte using a polymorphic allocator.
        template<class T = TStorage>
            requires T::kHasAllocator
        BasicStringImpl(const uint32_t length, const char value, std::pmr::memory_resource* allocator)
            : TStorage(allocator)
        {
            if (length != 0)
                ValidateStringByte(value);
            char* data = TStorage::InitializeImpl(length, TStorage::GetAllocator());
            memset(data, value, length);
            data[length] = '\0';
        }

        //! @brief Create a string from a UTF-8 byte range using a polymorphic allocator.
        template<class T = TStorage>
            requires T::kHasAllocator
        BasicStringImpl(const char* str, const uint32_t byteSize, std::pmr::memory_resource* allocator)
            : TStorage(allocator)
        {
            ValidateStringBytes(str, byteSize);
            char* data = TStorage::InitializeImpl(byteSize, TStorage::GetAllocator());
            memcpy(data, str, byteSize);
            data[byteSize] = '\0';
        }

        //! @brief Create a string from a null-terminated UTF-8 string using a polymorphic allocator.
        template<class T = TStorage>
            requires T::kHasAllocator
        BasicStringImpl(const char* str, std::pmr::memory_resource* allocator)
            : BasicStringImpl(str, ASCII::Length(str), allocator)
        {
        }

        //! @brief Create a string from UTF-8 bytes using a polymorphic allocator.
        template<class T = TStorage>
            requires T::kHasAllocator
        BasicStringImpl(std::initializer_list<char> chars, std::pmr::memory_resource* allocator)
            : BasicStringImpl(chars.begin(), static_cast<uint32_t>(chars.size()), allocator)
        {
        }

        ~BasicStringImpl()
        {
            TStorage::DestroyImpl(TStorage::GetAllocator());
        }

        //! @brief Copy another string.
        BasicStringImpl(const BasicStringImpl& other)
        {
            if constexpr (TStorage::kHasAllocator)
                TStorage::SetAllocator(other.GetAllocator());

            const uint32_t size = other.size();
            char* data = TStorage::InitializeImpl(size, TStorage::GetAllocator());
            memcpy(data, other.data(), size);
            data[size] = '\0';
        }

        //! @brief Move another string.
        BasicStringImpl(BasicStringImpl&& other)
        {
            MoveStorageFrom(other);
        }

        //! @brief Copy-assign another string.
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

        //! @brief Move-assign another string.
        BasicStringImpl& operator=(BasicStringImpl&& other)
        {
            if (this == &other)
                return *this;

            TStorage::DestroyImpl(TStorage::GetAllocator());
            MoveStorageFrom(other);
            return *this;
        }

        //! @brief Assign from a null-terminated UTF-8 string.
        BasicStringImpl& operator=(const char* str)
        {
            assign(str);
            return *this;
        }

        //! @brief Assign a single ASCII byte.
        BasicStringImpl& operator=(const char value)
        {
            assign(1, value);
            return *this;
        }

        //! @brief Assign from UTF-8 bytes.
        BasicStringImpl& operator=(const std::initializer_list<char> chars)
        {
            assign(chars);
            return *this;
        }

        //! @brief Create a string containing `length` copies of an ASCII byte.
        BasicStringImpl(const uint32_t length, const char value)
        {
            if (length != 0)
                ValidateStringByte(value);
            char* data = TStorage::InitializeImpl(length, TStorage::GetAllocator());
            memset(data, value, length);
            data[length] = '\0';
        }

        //! @brief Create a string from a UTF-8 byte range.
        BasicStringImpl(const char* str, uint32_t byteSize)
        {
            ValidateStringBytes(str, byteSize);
            char* data = TStorage::InitializeImpl(byteSize, TStorage::GetAllocator());
            memcpy(data, str, byteSize);
            data[byteSize] = '\0';
        }

        //! @brief Create a string from a null-terminated UTF-8 string.
        BasicStringImpl(const char* str)
            : BasicStringImpl(str, ASCII::Length(str))
        {
        }

        //! @brief Create a string from a pair of codepoint iterators.
        BasicStringImpl(const StrIterator begin, const StrIterator end)
            : BasicStringImpl(begin.m_iter, static_cast<uint32_t>(end.m_iter - begin.m_iter))
        {
        }

        //! @brief Create a string from a pair of byte pointers.
        BasicStringImpl(const char* begin, const char* end)
            : BasicStringImpl(begin, static_cast<uint32_t>(end - begin))
        {
        }

        //! @brief Create a string from UTF-8 bytes.
        BasicStringImpl(std::initializer_list<char> chars)
            : BasicStringImpl(chars.begin(), static_cast<uint32_t>(chars.size()))
        {
        }

        //! @brief Recreate the string storage for a raw byte size and return writable storage.
        //!
        //! This is an unsafe low-level API. The caller must write valid UTF-8 bytes followed by a null terminator before any
        //! normal string operation observes the object.
        char* reinitialize(const uint32_t byteSize)
        {
            return TStorage::Reinitialize(byteSize, TStorage::GetAllocator());
        }

        //! @brief Reserve storage for at least `byteSize` UTF-8 bytes, excluding the null terminator.
        void reserve(const uint32_t byteSize)
        {
            TStorage::ReserveImpl(byteSize, TStorage::GetAllocator());
        }

        //! @brief Resize to `byteSize` bytes without initializing newly exposed bytes.
        //!
        //! This is an unsafe low-level API. The caller must fill the bytes with valid UTF-8 and no embedded zero bytes.
        void resize_uninitialized(const uint32_t byteSize)
        {
            char* bytes = TStorage::ResizeImpl(byteSize, TStorage::GetAllocator());
            bytes[byteSize] = '\0';
        }

        //! @brief Resize to `byteSize` bytes, filling new bytes with an ASCII value.
        //!
        //! There is intentionally no `resize(count)` overload because this string disallows embedded zero bytes.
        void resize(const uint32_t byteSize, const char value)
        {
            const uint32_t initialSize = size();
            char* data = TStorage::ResizeImpl(byteSize, TStorage::GetAllocator());

            if (byteSize > initialSize)
                memset(data + initialSize, value, byteSize - initialSize);

            data[byteSize] = '\0';
        }

        //! @brief Remove all bytes from the string.
        void clear()
        {
            resize_uninitialized(0);
        }

        //! @brief Reduce capacity to the current byte size where supported by the storage policy.
        void shrink_to_fit()
        {
            TStorage::ShrinkImpl(TStorage::GetAllocator());
        }

        //! @brief Replace the contents with a UTF-8 byte range.
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

        //! @brief Replace the contents with a codepoint iterator range.
        void assign(const Iter begin, const Iter end)
        {
            assign(begin.m_iter, static_cast<uint32_t>(end.m_iter - begin.m_iter));
        }

        //! @brief Replace the contents with a null-terminated UTF-8 string.
        void assign(const char* str)
        {
            assign(str, ASCII::Length(str));
        }

        //! @brief Replace the contents with a UTF-8 string view.
        void assign(const BasicStringViewImpl& str)
        {
            assign(str.data(), str.size());
        }

        //! @brief Replace the contents with `length` copies of an ASCII byte.
        void assign(const uint32_t length, const char value)
        {
            if (length != 0)
                ValidateStringByte(value);
            resize_uninitialized(length);
            char* bytes = data();
            memset(bytes, value, length);
            bytes[length] = '\0';
        }

        //! @brief Replace the contents with UTF-8 bytes.
        void assign(const std::initializer_list<char> chars)
        {
            assign(chars.begin(), static_cast<uint32_t>(chars.size()));
        }

        //! @brief Append a UTF-8 byte range.
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

        //! @brief Append a null-terminated UTF-8 string.
        void append(const char* str)
        {
            append(str, ASCII::Length(str));
        }

        //! @brief Append a UTF-8 string view.
        void append(const BasicStringViewImpl& str)
        {
            append(str.data(), str.size());
        }

        //! @brief Append a codepoint iterator range.
        void append(const Iter begin, const Iter end)
        {
            append(begin.m_iter, static_cast<uint32_t>(end.m_iter - begin.m_iter));
        }

        //! @brief Append `length` copies of an ASCII byte.
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

        //! @brief Append UTF-8 bytes.
        void append(const std::initializer_list<char> chars)
        {
            append(chars.begin(), static_cast<uint32_t>(chars.size()));
        }

        //! @brief Append a Unicode codepoint encoded as UTF-8.
        //!
        //! The zero codepoint is rejected because embedded zero bytes are not supported.
        void append(const int32_t codepoint)
        {
            ValidateStringCodepoint(codepoint);
            char bytes[4];
            const int32_t bytesWritten = UTF8::Encode(codepoint, bytes);
            FE_AssertDebug(bytesWritten > 0, "String codepoint must be valid UTF-8");
            append(bytes, bytesWritten);
        }

        //! @brief Append a single ASCII byte.
        void push_back(const char byte)
        {
            ValidateStringByte(byte);
            const uint32_t oldSize = size();
            resize_uninitialized(oldSize + 1);
            char* bytes = data();
            bytes[oldSize] = byte;
            bytes[oldSize + 1] = '\0';
        }

        //! @brief Copy bytes into a caller-provided buffer.
        //!
        //! Unlike `std::string::copy`, `byteOffset` is always a byte offset and no null terminator is written.
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

        //! @brief Insert a UTF-8 byte range at a codepoint iterator position.
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

        //! @brief Insert a UTF-8 string view at a codepoint iterator position.
        void insert(const Iter position, const BasicStringViewImpl& str)
        {
            insert(position, str.data(), str.size());
        }

        //! @brief Insert a single ASCII byte at a codepoint iterator position.
        Iter insert(const Iter position, const char byte)
        {
            const uintptr_t dataAddress = reinterpret_cast<uintptr_t>(data());
            const uintptr_t positionAddress = reinterpret_cast<uintptr_t>(position.m_iter);
            const uint32_t positionOffset = static_cast<uint32_t>(positionAddress - dataAddress);
            insert(position, &byte, 1);
            return Iter{ data() + positionOffset };
        }

        //! @brief Insert `length` copies of an ASCII byte at a codepoint iterator position.
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

        //! @brief Insert UTF-8 bytes at a codepoint iterator position.
        void insert(const Iter position, const std::initializer_list<char> chars)
        {
            insert(position, chars.begin(), static_cast<uint32_t>(chars.size()));
        }

        //! @brief Erase a codepoint iterator range and return the iterator at the erased position.
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

        //! @brief Erase one codepoint and return the iterator at the erased position.
        Iter erase(const Iter position)
        {
            Iter next = position;
            ++next;
            return erase(position, next);
        }

        //! @brief Replace a codepoint iterator range with a UTF-8 byte range.
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

        //! @brief Replace a codepoint iterator range with a UTF-8 string view.
        void replace(const Iter first, const Iter last, const BasicStringViewImpl& str)
        {
            replace(first, last, str.data(), str.size());
        }

        //! @brief Replace a codepoint iterator range with `length` copies of an ASCII byte.
        void replace(const Iter first, const Iter last, const uint32_t length, const char value)
        {
            const Iter insertionPosition = erase(first, last);
            insert(insertionPosition, length, value);
        }

        //! @brief Replace a codepoint iterator range with UTF-8 bytes.
        void replace(const Iter first, const Iter last, const std::initializer_list<char> chars)
        {
            replace(first, last, chars.begin(), static_cast<uint32_t>(chars.size()));
        }

        //! @brief Remove the final Unicode codepoint.
        void pop_back()
        {
            FE_AssertDebug(size() != 0, "Cannot pop from an empty string");

            Iter last{ data() + size() };
            --last;
            erase(last, Iter{ data() + size() });
        }

        //! @brief Swap contents with another string of the same storage type.
        void swap(BasicStringImpl& other) noexcept
        {
            if (this == &other)
                return;

            BasicStringImpl temporary = std::move(other);
            other = std::move(*this);
            *this = std::move(temporary);
        }

        //! @brief Get the allocator used by this string.
        [[nodiscard]] std::pmr::memory_resource* get_allocator() const
        {
            return TStorage::GetAllocator();
        }

        //! @brief Move the string contents to a different polymorphic allocator.
        void set_allocator(std::pmr::memory_resource* allocator)
            requires TStorage::kHasAllocator
        {
            if (allocator == nullptr)
                allocator = std::pmr::get_default_resource();

            if (allocator == TStorage::GetAllocator())
                return;

            BasicStringImpl temporary{ allocator };
            temporary.assign(data(), size());
            *this = std::move(temporary);
        }

        //! @brief Get the size of the string in bytes.
        [[nodiscard]] uint32_t size() const
        {
            return TStorage::SizeImpl();
        }

        //! @brief Get the number of Unicode codepoints in the string.
        [[nodiscard]] uint32_t length() const
        {
            return UTF8::Length(data(), size());
        }

        //! @brief Get the byte capacity of the string, excluding the null terminator.
        [[nodiscard]] uint32_t capacity() const
        {
            return TStorage::CapacityImpl();
        }

        //! @brief Get the maximum representable byte size.
        [[nodiscard]] uint32_t max_size() const
        {
            return Constants::kMaxU32 - 1;
        }

        //! @brief Get writable access to the string bytes.
        //!
        //! The caller must preserve valid UTF-8, no embedded zero bytes, and the final null terminator.
        [[nodiscard]] char* data()
        {
            return TStorage::DataImpl();
        }

        //! @brief Get read-only access to the string bytes.
        [[nodiscard]] const char* data() const
        {
            return TStorage::DataImpl();
        }

        //! @brief Get a null-terminated UTF-8 string.
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


    //! @brief Public UTF-8 string facade shared by owning strings and string views.
    //!
    //! This type adds Unicode-aware operations on top of either owning storage or view storage. `begin()`, `end()`,
    //! `rbegin()`, and `rend()` iterate decoded Unicode codepoints, not raw bytes. Functions that accept or return `Iter`
    //! use codepoint positions. Functions whose names contain `byte` or `ascii` operate on byte offsets.
    //!
    //! Compared to `std::string`, substring and search APIs deliberately prefer codepoint iterators or codepoint indices.
    //! `substr()` returns a string view rather than an owning string.
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

        //! @brief Create from an ASCII byte view.
        StringImpl(festd::ascii_view str)
            : TBase(str.data(), static_cast<uint32_t>(str.length()))
        {
        }

        //! @brief Create from another string or string view.
        template<class TOtherBase>
        StringImpl(const StringImpl<TOtherBase>& other)
            : TBase(other.data(), other.size())
        {
        }

        //! @brief Create from an environment name.
        explicit StringImpl(const Env::Name name)
            : StringImpl(festd::ascii_view{ name })
        {
        }

        //! @brief Assign from another string or string view.
        template<class TOtherBase>
            requires requires(TBase& base, const char* data) { base.assign(data, uint32_t{}); }
        StringImpl& operator=(const StringImpl<TOtherBase>& other)
        {
            TBase::assign(other.data(), other.size());
            return *this;
        }

        //! @brief Point a string view at a null-terminated UTF-8 string.
        StringImpl& operator=(const char* str)
            requires std::same_as<TBase, BasicStringViewImpl>
        {
            TBase::operator=(BasicStringViewImpl{ str });
            return *this;
        }

        //! @brief Append a null-terminated UTF-8 string.
        StringImpl& operator+=(const char* str)
            requires requires(TBase& base, const char* data) { base.append(data); }
        {
            TBase::append(str);
            return *this;
        }

        //! @brief Append a single ASCII byte.
        StringImpl& operator+=(const char byte)
            requires requires(TBase& base) { base.push_back(char{}); }
        {
            TBase::push_back(byte);
            return *this;
        }

        //! @brief Append UTF-8 bytes.
        StringImpl& operator+=(std::initializer_list<char> chars)
            requires requires(TBase& base) { base.append(std::initializer_list<char>{}); }
        {
            TBase::append(chars);
            return *this;
        }

        //! @brief Read a raw byte by byte index.
        [[nodiscard]] char byte_at(uint32_t byteIndex) const
        {
            return TBase::data()[byteIndex];
        }

        //! @brief Decode and return a Unicode codepoint by codepoint index.
        [[nodiscard]] int32_t codepoint_at(const uint32_t codepointIndex) const
        {
            return *(begin() + codepointIndex);
        }

        //! @brief Get a byte-indexed ASCII view of a substring.
        //!
        //! This is byte-indexed and is the closest equivalent to `std::string_view::substr`.
        [[nodiscard]] festd::ascii_view substr_ascii(const uint32_t startIndex, uint32_t length = kInvalidIndex) const
        {
            const char* str = TBase::data();
            if (startIndex > TBase::size())
                return {};

            return festd::ascii_view{ str + startIndex, Math::Min(length, TBase::size() - startIndex) };
        }

        //! @brief Get a codepoint-indexed UTF-8 substring view.
        //!
        //! Unlike `std::string::substr`, this function uses codepoint positions and returns a non-owning view.
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

        //! @brief Get a UTF-8 substring view from a codepoint iterator range.
        [[nodiscard]] StringImpl<BasicStringViewImpl> substr(const Iter startIter, const Iter endIter) const
        {
            return StringImpl<BasicStringViewImpl>{ startIter, endIter };
        }

        //! @brief Get a UTF-8 substring view from a codepoint iterator to the end.
        [[nodiscard]] StringImpl<BasicStringViewImpl> substr(const Iter startIter) const
        {
            return StringImpl<BasicStringViewImpl>{ startIter, end() };
        }

        //! @brief Compare with a null-terminated UTF-8 string by Unicode codepoint values.
        [[nodiscard]] int32_t compare(const char* other) const
        {
            return UTF8::Compare(TBase::data(), other, TBase::size(), ASCII::Length(other));
        }

        //! @brief Compare with another string or string view by Unicode codepoint values.
        template<class TOtherStorage>
        [[nodiscard]] int32_t compare(const StringImpl<TOtherStorage>& other) const
        {
            return UTF8::Compare(TBase::data(), other.data(), TBase::size(), other.size());
        }

        //! @brief Find the first matching codepoint at or after a codepoint iterator.
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

        //! @brief Find the first matching codepoint.
        [[nodiscard]] Iter find_first_of(const int32_t codepoint) const
        {
            return find_first_of(begin(), codepoint);
        }

        //! @brief Find the last matching codepoint before a codepoint iterator.
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

        //! @brief Find the last matching codepoint.
        [[nodiscard]] Iter find_last_of(const int32_t codepoint) const
        {
            return find_last_of(end(), codepoint);
        }

        //! @brief Return a view with leading Unicode space codepoints removed.
        [[nodiscard]] StringImpl<BasicStringViewImpl> strip_left() const
        {
            const Iter nonSpace = eastl::find_if(begin(), end(), [](const int32_t codepoint) {
                return !UTF8::IsSpace(codepoint);
            });

            return StringImpl<BasicStringViewImpl>{ nonSpace, end() };
        }

        //! @brief Return a view with trailing Unicode space codepoints removed.
        [[nodiscard]] StringImpl<BasicStringViewImpl> strip_right() const
        {
            const eastl::reverse_iterator<Iter> nonSpace = eastl::find_if(rbegin(), rend(), [](const int32_t codepoint) {
                return !UTF8::IsSpace(codepoint);
            });

            return StringImpl<BasicStringViewImpl>{ begin(), nonSpace.base() };
        }

        //! @brief Return a view with leading and trailing Unicode space codepoints removed.
        [[nodiscard]] StringImpl<BasicStringViewImpl> strip() const
        {
            return strip_left().strip_right();
        }

        //! @brief Check if the string has zero bytes.
        [[nodiscard]] bool empty() const
        {
            return TBase::size() == 0;
        }

        //! @brief Get the number of Unicode codepoints in the string.
        [[nodiscard]] uint32_t length() const
        {
            return UTF8::Length(TBase::data(), TBase::size());
        }

        //! @brief Check if the string starts with a UTF-8 byte sequence.
        [[nodiscard]] bool starts_with(const StringImpl<BasicStringViewImpl> str) const
        {
            if (str.size() == 0)
                return true;

            const uint32_t size = TBase::size();
            const char* data = TBase::data();
            return str.size() <= size && memcmp(data, str.data(), str.size()) == 0;
        }

        //! @brief Check if the string ends with a UTF-8 byte sequence.
        [[nodiscard]] bool ends_with(const StringImpl<BasicStringViewImpl> str) const
        {
            if (str.size() == 0)
                return true;

            const uint32_t size = TBase::size();
            const char* data = TBase::data();
            return str.size() <= size && memcmp(data + size - str.size(), str.data(), str.size()) == 0;
        }

        //! @brief Find the first occurrence of a UTF-8 byte sequence.
        [[nodiscard]] Iter find(const StringImpl<BasicStringViewImpl> str) const
        {
            return find(begin(), str);
        }

        //! @brief Find the first occurrence of a UTF-8 byte sequence at or after a codepoint iterator.
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

        //! @brief Find the last occurrence of a UTF-8 byte sequence.
        [[nodiscard]] Iter rfind(const StringImpl<BasicStringViewImpl> str) const
        {
            return rfind(end(), str);
        }

        //! @brief Find the last occurrence of a UTF-8 byte sequence at or before a codepoint iterator.
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

        //! @brief Find the first codepoint that does not match `codepoint` at or after a codepoint iterator.
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

        //! @brief Find the first codepoint that does not match `codepoint`.
        [[nodiscard]] Iter find_first_not_of(const int32_t codepoint) const
        {
            return find_first_not_of(begin(), codepoint);
        }

        //! @brief Find the last codepoint that does not match `codepoint` before a codepoint iterator.
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

        //! @brief Find the last codepoint that does not match `codepoint`.
        [[nodiscard]] Iter find_last_not_of(const int32_t codepoint) const
        {
            return find_last_not_of(end(), codepoint);
        }

        //! @brief Convert to an environment name.
        [[nodiscard]] explicit operator Env::Name() const noexcept
        {
            return Env::Name{ TBase::data(), TBase::size() };
        }

        //! @brief Get a codepoint iterator to the first codepoint.
        [[nodiscard]] Iter begin() const
        {
            return Iter{ TBase::data() };
        }

        //! @brief Get a codepoint iterator to the end.
        [[nodiscard]] Iter end() const
        {
            return Iter{ TBase::data() + TBase::size() };
        }

        //! @brief Get a reverse codepoint iterator to the final codepoint.
        [[nodiscard]] eastl::reverse_iterator<Iter> rbegin() const
        {
            return eastl::reverse_iterator<Iter>(end());
        }

        //! @brief Get a reverse codepoint iterator to the beginning.
        [[nodiscard]] eastl::reverse_iterator<Iter> rend() const
        {
            return eastl::reverse_iterator<Iter>(begin());
        }
    };


    //! @brief Append a UTF-8 string view to an owning string.
    template<class TStorage>
    StringImpl<BasicStringImpl<TStorage>>& operator+=(StringImpl<BasicStringImpl<TStorage>>& lhs,
                                                      const StringImpl<BasicStringViewImpl> rhs)
    {
        lhs.append(rhs.data(), rhs.size());
        return lhs;
    }


    //! @brief Concatenate an owning string and a UTF-8 string view.
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

    inline bool operator==(const StringImpl<BasicStringViewImpl> lhs, const char* rhs)
    {
        return lhs.size() == ASCII::Length(rhs) && (lhs.size() == 0 || memcmp(lhs.data(), rhs, lhs.size()) == 0);
    }

    inline bool operator==(const char* lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return ASCII::Length(lhs) == rhs.size() && (rhs.size() == 0 || memcmp(lhs, rhs.data(), rhs.size()) == 0);
    }

    inline bool operator==(const Env::Name lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return lhs.size() == rhs.size() && (lhs.size() == 0 || memcmp(lhs.c_str(), rhs.data(), lhs.size()) == 0);
    }

    inline bool operator==(const StringImpl<BasicStringViewImpl> lhs, const Env::Name rhs)
    {
        return lhs.size() == rhs.size() && (lhs.size() == 0 || memcmp(lhs.data(), rhs.c_str(), lhs.size()) == 0);
    }

    inline std::strong_ordering operator<=>(const StringImpl<BasicStringViewImpl> lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return lhs.compare(rhs) <=> 0;
    }

    inline std::strong_ordering operator<=>(const StringImpl<BasicStringViewImpl> lhs, const char* rhs)
    {
        return lhs.compare(rhs) <=> 0;
    }

    inline std::strong_ordering operator<=>(const char* lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return StringImpl<BasicStringViewImpl>(lhs).compare(rhs) <=> 0;
    }

    inline std::strong_ordering operator<=>(const StringImpl<BasicStringViewImpl> lhs, const Env::Name rhs)
    {
        return lhs.compare(StringImpl<BasicStringViewImpl>(rhs)) <=> 0;
    }

    inline std::strong_ordering operator<=>(const Env::Name lhs, const StringImpl<BasicStringViewImpl> rhs)
    {
        return StringImpl<BasicStringViewImpl>(lhs).compare(rhs) <=> 0;
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
        //! @brief Dynamically allocated UTF-8 string that stores a polymorphic allocator.
        using string = FE::Internal::StringImpl<FE::Internal::BasicStringImpl<FE::Internal::PolymorphicStringStorage>>;

        //! @brief UTF-8 string with inline storage and polymorphic heap fallback.
        template<uint32_t TCapacity>
        using basic_inline_string =
            FE::Internal::StringImpl<FE::Internal::BasicStringImpl<FE::Internal::PolymorphicInlineStringStorage<TCapacity>>>;

        //! @brief UTF-8 string with 256 bytes of inline storage and polymorphic heap fallback.
        using inline_string = basic_inline_string<256>;
    } // namespace pmr

    //! @brief Dynamically allocated UTF-8 string using the default memory resource.
    using string = FE::Internal::StringImpl<FE::Internal::BasicStringImpl<FE::Internal::DefaultDynamicStringStorage>>;

    //! @brief Non-owning UTF-8 string view.
    using string_view = FE::Internal::StringImpl<FE::Internal::BasicStringViewImpl>;

    //! @brief Fixed-capacity owning UTF-8 string.
    //!
    //! The string can store at most `TCapacity` bytes, excluding the null terminator.
    template<uint32_t TCapacity>
    using basic_fixed_string =
        FE::Internal::StringImpl<FE::Internal::BasicStringImpl<FE::Internal::DefaultFixedStringStorage<TCapacity>>>;

    //! @brief Fixed-capacity owning UTF-8 string with 256 bytes of storage.
    using fixed_string = basic_fixed_string<256>;

    //! @brief Owning UTF-8 string with inline storage and dynamic heap fallback.
    //!
    //! The string stores up to `TCapacity` bytes inline before allocating heap storage.
    template<uint32_t TCapacity>
    using basic_inline_string =
        FE::Internal::StringImpl<FE::Internal::BasicStringImpl<FE::Internal::DefaultInlineStringStorage<TCapacity>>>;

    //! @brief Owning UTF-8 string with 256 bytes of inline storage and dynamic heap fallback.
    using inline_string = basic_inline_string<256>;

    static_assert(sizeof(string) == sizeof(uintptr_t) * 3);
    static_assert(sizeof(pmr::string) == sizeof(string) + sizeof(uintptr_t));
    static_assert(sizeof(string_view) == sizeof(uintptr_t) * 2);
} // namespace FE::festd


namespace FE
{
    template<class T>
    concept AppendableString = requires(T& t, const uint32_t size, const int32_t codepoint) {
        { t.reserve(size) } -> std::same_as<void>;
        { t.append(codepoint) } -> std::same_as<void>;
    };


    //! @brief Compute a compile-time hash for a UTF-8 string view.
    constexpr uint64_t CompileTimeHash(const festd::string_view str)
    {
        return CompileTimeHash(str.data(), str.size());
    }


    //! @brief Compute a runtime hash for a UTF-8 string view.
    inline uint64_t DefaultHash(const festd::string_view str)
    {
        return DefaultHash(str.data(), str.size());
    }


    namespace Str
    {
        //! @brief Duplicate a UTF-8 string view into memory allocated by `allocator`.
        inline festd::string_view Duplicate(const festd::string_view str, std::pmr::memory_resource* allocator)
        {
            void* memory = allocator->allocate(str.size() + 1);
            memcpy(memory, str.data(), str.size());
            static_cast<char*>(memory)[str.size()] = '\0';
            return { static_cast<char*>(memory), str.size() };
        }


        template<AppendableString TOutput>
        TOutput ToLower(const festd::string_view input)
        {
            TOutput output;
            output.reserve(input.size());
            for (auto it = input.begin(); it != input.end(); ++it)
                output.append(UTF8::ToLower(*it));

            return output;
        }


        template<AppendableString TOutput>
        TOutput ToUpper(const festd::string_view input)
        {
            TOutput output;
            output.reserve(input.size());
            for (auto it = input.begin(); it != input.end(); ++it)
                output.append(UTF8::ToUpper(*it));

            return output;
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
