#pragma once
#include <Core/Compression/Compression.h>
#include <Core/IO/IStreamFactory.h>
#include <Core/Jobs/IJobSystem.h>

namespace FE::IO
{
    //! @brief Asynchronous I/O operation status.
    enum class AsyncOperationStatus
    {
        kQueued,    //!< The operation has been queued, the processing hasn't yet started.
        kRunning,   //!< The operation is currently being processed by the I/O thread.
        kCanceled,  //!< The operation has been canceled.
        kSucceeded, //!< The operation has completed successfully.
        kFailed,    //!< The operation has failed.
    };


    //! @brief Returns true if the specified status indicates that an operation is in a final state.
    constexpr bool IsFinalStatus(const AsyncOperationStatus status)
    {
        switch (status)
        {
        case AsyncOperationStatus::kCanceled:
        case AsyncOperationStatus::kSucceeded:
        case AsyncOperationStatus::kFailed:
            return true;
        default:
            return false;
        }
    }


    //! @brief Asynchronous operation controller: can be used to cancel an operation or to query its status.
    struct IAsyncController : public Memory::RefCountedObjectBase
    {
        FE_RTTI("2427B1D9-F1A5-4A1B-A804-EB9ACA502C28");

        ~IAsyncController() override = default;

        virtual void Cancel() = 0;
        virtual AsyncOperationStatus GetStatus() const = 0;
        virtual ResultCode GetLastOperationResult() const = 0;
    };


    //! @brief Asynchronous read operation request.
    struct AsyncReadRequest final
    {
        Rc<IStream> m_stream;  //!< The stream that the operation will be performed on, optional.
        Path m_path;           //!< The path to the file to open the stream for, must be provided if m_stream is null.
        intptr_t m_offset = 0; //!< The starting offset in the source file.

        uintptr_t m_userData0 = 0; //!< Optional user data, ignored by the I/O thread.
        uintptr_t m_userData1 = 0; //!< Optional user data, ignored by the I/O thread.

        IAsyncReadCallback* m_callback = nullptr;         //!< The callback to call when the read is completed.
        std::pmr::memory_resource* m_allocator = nullptr; //!< The allocator used for read and temporary buffers.
                                                          //!< Optional: the default allocator will be used when null.

        std::byte* m_readBuffer = nullptr; //!< The destination buffer. Optional: allocated by the I/O thread when null.
        uint32_t m_readBufferSize = 0;     //!< Raw byte count for kNone, destination capacity for compressed methods.
        uint32_t m_compressedSize = 0;     //!< Compressed data size, ignored if m_compressionMethod is kNone.
        uint32_t m_overallocateBytes = 0;  //!< Extra bytes allocated after an automatically allocated destination buffer.

        Compression::Method m_compressionMethod = Compression::Method::kNone;
        JobPriority m_decompressionPriority = JobPriority::kNormal;
    };


    struct AsyncReadResult final
    {
        AsyncReadRequest* m_request = nullptr;
        IAsyncController* m_controller = nullptr;
        size_t m_bytesRead = 0;

        void FreeData() const
        {
            m_request->m_allocator->deallocate(m_request->m_readBuffer, m_request->m_readBufferSize);
            m_request->m_readBuffer = nullptr;
        }
    };


    //! @brief Asynchronous I/O thread interface.
    struct IAsyncStreamIO : public Memory::RefCountedObjectBase
    {
        FE_RTTI("A44064EC-34E0-4B99-9BC7-A2B27321F617");

        ~IAsyncStreamIO() override = default;

        //! @brief Enqueue an asynchronous read operation.
        //!
        //! This function can be used to read files asynchronously from the file system or from archives.
        //! It uses the registered IStreamFactory to open streams and performs reads on a dedicated I/O thread.
        //! If compression method specified in the request is not kNone, reads are decompressed
        //! by one background job before the callback is invoked.
        //!
        //! @param request      Read operation request specification.
        //! @param priority     The priority of the operation.
        //! @param ppController A pointer to the variable that receives a pointer to IAsyncController.
        virtual void ReadAsync(const AsyncReadRequest& request, Priority priority = Priority::kNormal,
                               IAsyncController** ppController = nullptr) = 0;
    };
} // namespace FE::IO
