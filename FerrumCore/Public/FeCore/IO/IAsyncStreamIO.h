#pragma once
#include <FeCore/IO/IStreamFactory.h>

namespace FE::IO
{
    //! @brief Asynchronous I/O operation status.
    enum class AsyncOperationStatus
    {
        kQueued,    //!< The operation has been queued, the processing hasn't yet started.
        kRunning,   //!< The operation is currently being processed by the I/O thread.
        kCanceled,  //!< The operation has been canceled.
        kSucceeded, //!< The operation has completed successfully.
        kFailed,    //!< The operation failed.
    };


    //! @brief Returns true if the specified status indicates that an operation is in a final state.
    inline constexpr bool IsFinalStatus(AsyncOperationStatus status)
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


    struct IAsyncReadCallback;

    //! @brief Basic asynchronous operation request. Don't use it directly, see e.g. AsyncReadRequest.
    struct AsyncOperationRequest
    {
        Rc<IStream> pStream;       //!< The stream that the operation will be performed on, optional.
        FixedPath Path;            //!< The path to the file to open the stream for, must be provided if pStream is null.
        uintptr_t Offset = 0;      //!< The starting offset to read from or write to.
        void* pUserData = nullptr; //!< An optional pointer to user data.
        Priority Priority = Priority::kNormal; //!< Request priority.
    };


    //! @brief Asynchronous read operation request.
    struct AsyncReadRequest : public AsyncOperationRequest
    {
        IAsyncReadCallback* pCallback = nullptr;         //!< The callback to call when the read is completed.
        std::pmr::memory_resource* pAllocator = nullptr; //!< The allocator to use to allocate memory to read to.
                                                         //!< Optional: will be assigned by the I/O thread by default.

        std::byte* pReadBuffer = nullptr; //!< The buffer to read to. Optional will be allocated by the I/O thread by default.
        uint32_t ReadBufferSize = 0;      //!< The size of the buffer to read to.
                                          //!< Optional: the size of the file will be used by default.
    };


    struct AsyncReadResult final
    {
        const AsyncReadRequest* pRequest = nullptr;
        IAsyncController* pController = nullptr;
        size_t BytesRead = 0;
    };


    //! @brief Asynchronous read operation callback.
    struct IAsyncReadCallback
    {
        FE_RTTI_Class(IAsyncReadCallback, "E1E0BD22-543A-4036-B918-134DB9C99D4F");

        virtual ~IAsyncReadCallback() = default;

        //! @brief Called when an operation associated with this callback completes.
        virtual void AsyncIOCallback(const AsyncReadResult& result) = 0;
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
        //! a file block by block, specify ReadBufferSize in the request.
        //!
        //! The memory can be either allocated by the caller or by the I/O thread itself.
        //! However the caller is always responsible for deallocating the storage
        //! after the operation is completed.
        //!
        //! @param request      - Read operation request specification.
        //! @param ppController - A pointer to the variable that receives a pointer to IAsyncController.
        virtual void ReadAsync(const AsyncReadRequest& request, IAsyncController** ppController = nullptr) = 0;
    };
} // namespace FE::IO
