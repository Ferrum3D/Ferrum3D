#pragma once
#include <FeCore/IO/BaseIO.h>
#include <FeCore/Memory/Memory.h>

namespace FE::IO
{
    //! @brief Base interface for I/O streams.
    struct IStream : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(IStream, "FD697DC5-020E-4998-ADF2-9DFAF48E2A75");

        ~IStream() override = default;

        //! @brief True if write operation is allowed on this stream.
        [[nodiscard]] virtual bool WriteAllowed() const = 0;

        //! @brief True if read operation is allowed on this stream.
        [[nodiscard]] virtual bool ReadAllowed() const = 0;

        //! @brief True if seek operation is allowed on this stream.
        [[nodiscard]] virtual bool SeekAllowed() const = 0;

        //! @brief True if stream is open and ready to use.
        [[nodiscard]] virtual bool IsOpen() const = 0;

        //! @brief Add offset to current stream position.
        //!
        //! @param offset   Offset to add to current position.
        //! @param seekMode Seek mode to use.
        //!
        //! @return Success or error code.
        virtual ResultCode Seek(intptr_t offset, SeekMode seekMode) = 0;

        //! @brief Get current stream position.
        //!
        //! @note This can always return zero for certain streams, e.g. stdio stream.
        [[nodiscard]] virtual uintptr_t Tell() const = 0;

        //! @brief Get length of the stream.
        //!
        //! This will return length of streams when size is known, e.g. file streams.
        //! Otherwise return value is always zero.
        [[nodiscard]] virtual size_t Length() const = 0;

        //! @brief Read contents of stream to a pre-allocated buffer.
        //!
        //! @param buffer The buffer to read to.
        //! @param byteSize Size of buffer in bytes.
        //!
        //! @return Number of bytes actually read.
        virtual size_t ReadToBuffer(void* buffer, size_t byteSize) = 0;

        //! @brief Read contents of stream to a pre-allocated buffer.
        //!
        //! @param buffer The buffer to read to.
        //!
        //! @return Number of bytes actually read.
        size_t ReadToBuffer(const festd::span<std::byte> buffer)
        {
            return ReadToBuffer(buffer.data(), buffer.size());
        }

        //! @brief Write contents of buffer to the stream.
        //!
        //! @param buffer The buffer to write from.
        //! @param byteSize Size of buffer in bytes.
        //!
        //! @return Number of bytes actually written.
        virtual size_t WriteFromBuffer(const void* buffer, size_t byteSize) = 0;

        //! @brief Write contents of buffer to the stream.
        //!
        //! @param buffer The buffer to write from.
        //!
        //! @return Number of bytes actually written.
        size_t WriteFromBuffer(const festd::span<const std::byte> buffer)
        {
            return WriteFromBuffer(buffer.data(), buffer.size());
        }

        //! @brief Write to this stream from other stream.
        //!
        //! @param stream Pointer to stream to write from.
        //! @param size   Size in bytes of data to write.
        //!
        //! @return Number of bytes actually written.
        virtual size_t WriteFromStream(IStream* stream, size_t size) = 0;

        //! @brief Get name of the stream.
        [[nodiscard]] virtual festd::string_view GetName() = 0;

        //! @brief Get OpenMode for this stream if supported.
        [[nodiscard]] virtual OpenMode GetOpenMode() const = 0;

        //! @brief Only works for file streams.
        [[nodiscard]] virtual FileStats GetStats() const = 0;

        //! @brief Close this stream.
        virtual void Close() = 0;

        //! @brief Flush all buffered writes if supported.
        virtual void FlushWrites() = 0;
    };
} // namespace FE::IO
