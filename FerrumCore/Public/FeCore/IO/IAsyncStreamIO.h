#pragma once
#include <FeCore/Compression/Compression.h>
#include <FeCore/IO/IStreamFactory.h>
#include <FeCore/Jobs/IJobSystem.h>

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
        FE_RTTI_Class(IAsyncController, "2427B1D9-F1A5-4A1B-A804-EB9ACA502C28");

        ~IAsyncController() override = default;

        virtual void Cancel() = 0;
        virtual AsyncOperationStatus GetStatus() const = 0;
        virtual ResultCode GetLastOperationResult() const = 0;
    };


    //! @brief Basic asynchronous operation request.
    struct AsyncOperationRequest
    {
        Rc<IStream> m_stream;  //!< The stream that the operation will be performed on, optional.
        Path m_path;           //!< The path to the file to open the stream for, must be provided if pStream is null.
        intptr_t m_offset = 0; //!< The starting offset in the source file. Will be moved by the I/O thread after each operation.

        uintptr_t m_userData0 = 0; //!< Optional user data, ignored by the I/O thread.
        uintptr_t m_userData1 = 0; //!< Optional user data, ignored by the I/O thread.
    };


    //! @brief Asynchronous read operation request.
    struct AsyncReadRequest : public AsyncOperationRequest
    {
        IAsyncReadCallback* m_callback = nullptr;         //!< The callback to call when the read is completed.
        std::pmr::memory_resource* m_allocator = nullptr; //!< The allocator to use to allocate memory to read to.
                                                          //!< Optional: will be assigned by the I/O thread by default.

        std::byte* m_readBuffer = nullptr; //!< The buffer to read to. Optional, will be allocated by the I/O thread by default.
        uint32_t m_readBufferSize = 0;     //!< The size of the buffer to read to.
                                           //!< Optional: the size of the file will be used by default.
        uint32_t m_overallocateBytes = 0; //!< Optional: if the buffer is allocated by the I/O thread, this option allows the user
                                          //!< to additionally allocate some extra memory at the end of the allocated buffer.
                                          //!< For instance, this can be useful for text files to reserve
                                          //!< one byte for the terminating zero.
                                          //!< Note: this overallocated memory is not guaranteed to be zeroed.
    };


    //! @brief Asynchronous block read operation request. The requested file must contain a stream of compressed blocks.
    //!
    //! The I/O thread will read Compression::BlockHeader to determine block's compressed size. Then it will read and decompress
    //! the block data and call the callback. The cycle repeats for m_blockCount blocks.
    struct AsyncBlockReadRequest : public AsyncOperationRequest
    {
        IAsyncReadCallback* m_callback = nullptr;         //!< The callback to call when the read is completed.
        std::pmr::memory_resource* m_allocator = nullptr; //!< The allocator to use to allocate memory to read to.
                                                          //!< Optional: will be assigned by the I/O thread by default.

        std::byte* m_readBuffer = nullptr; //!< The buffer to read to. Optional if m_decompress is true, will be allocated by
                                           //!< the I/O thread by default.
                                           //!< The size of the buffer must be equal to Compression::kBlockSize when
                                           //!< m_decompress is true. Otherwise, it must be calculated using
                                           //!< Compressor::GetBound() for the corresponding compression method.
        uint32_t m_readBufferSize = 0;     //!< The size of the buffer to read to if the buffer is provided by the caller.
        uint32_t m_blockCount = 1;         //!< The number of blocks to read.
        Crc32 m_crc32;                     //!< The initial CRC32 value to be used for integrity checks when decompressing.
                                           //!< The I/O thread will update this value as it reads blocks.
        bool m_decompress = true;          //!< If true, the data read from blocks will be decompressed.

        JobPriority m_decompressionPriority = JobPriority::kNormal; //!< If m_decompress is true, the priority of the job that
                                                                    //!< the I/O thread will schedule for decompression.
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


    struct AsyncBlockReadResult final
    {
        AsyncBlockReadRequest* m_request = nullptr;
        IAsyncController* m_controller = nullptr;
        size_t m_bytesRead = 0;
        uint32_t m_blockIndex = 0;

        void FreeData() const
        {
            m_request->m_allocator->deallocate(m_request->m_readBuffer, Compression::kBlockSize);
            m_request->m_readBuffer = nullptr;
        }
    };


    //! @brief Asynchronous I/O thread interface.
    struct IAsyncStreamIO : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(IAsyncStreamIO, "A44064EC-34E0-4B99-9BC7-A2B27321F617");

        ~IAsyncStreamIO() override = default;

        //! @brief Enqueue an asynchronous read operation.
        //!
        //! This function can be used to read files asynchronously from the file system or from archives.
        //! It uses currently registered IStreamFactory to open streams and then calls read methods of
        //! the IStream object in a dedicated I/O thread.
        //!
        //! When the operation is completed the I/O thread will call the provided callback function.
        //! Note that the callback blocks the I/O thread, so the callback must return as soon as possible.
        //! Preferably it should kick off a data processing job.
        //!
        //! The I/O thread can read files either entirely (default) or by blocks. If you need to read
        //! a file block by block, specify m_readBufferSize in the request.
        //!
        //! The memory can be either allocated by the caller or by the I/O thread itself.
        //! However, the caller is always responsible for deallocating the storage
        //! after the operation is completed.
        //!
        //! @param request      Read operation request specification.
        //! @param priority     The priority of the operation.
        //! @param ppController A pointer to the variable that receives a pointer to IAsyncController.
        virtual void ReadAsync(const AsyncReadRequest& request, Priority priority = Priority::kNormal,
                               IAsyncController** ppController = nullptr) = 0;

        //! @brief Enqueue an asynchronous block read operation.
        //!
        //! In general, this function works the same way as the one that takes AsyncReadRequest.
        //! The difference is that it reads compressed block streams and provides the callback with
        //! decompressed data.
        //!
        //! @param request      Read operation request specification.
        //! @param priority     The priority of the operation.
        //! @param ppController A pointer to the variable that receives a pointer to IAsyncController.
        virtual void ReadAsync(const AsyncBlockReadRequest& request, Priority priority = Priority::kNormal,
                               IAsyncController** ppController = nullptr) = 0;
    };
} // namespace FE::IO
