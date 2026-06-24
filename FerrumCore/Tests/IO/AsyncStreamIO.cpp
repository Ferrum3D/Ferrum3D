#include <Core/IO/AsyncStreamIO.h>
#include <Core/IO/FileStream.h>
#include <Core/IO/Path.h>
#include <Core/Jobs/Job.h>
#include <Core/Math/Random.h>
#include <Core/Threading/ConditionVariable.h>
#include <Core/Threading/Mutex.h>
#include <gtest/gtest.h>

using namespace FE;

namespace
{
    struct ImmediateJobSystem final : public IJobSystem
    {
        void Schedule(const JobScheduleInfo& info) override
        {
            ++m_scheduledJobCount;
            info.m_job->Execute();
        }

        void Start() override {}
        void Stop() override {}

        FiberAffinityMask GetAffinityMaskForCurrentThread() const override
        {
            return FiberAffinityMask::kMainThread;
        }

        std::atomic<uint32_t> m_scheduledJobCount = 0;
    };


    struct ReadCallback final : public IO::IAsyncReadCallback
    {
        void AsyncIOCallback(const IO::AsyncReadResult& result) override
        {
            std::lock_guard lock{ m_mutex };
            m_status = result.m_controller->GetStatus();
            m_result = result.m_controller->GetLastOperationResult();
            m_bytesRead = result.m_bytesRead;

            if (result.m_request->m_readBuffer != nullptr)
            {
                m_data.assign(result.m_request->m_readBuffer, result.m_request->m_readBuffer + result.m_bytesRead);
                result.FreeData();
            }

            m_called = true;
            m_condition.NotifyOne();
        }

        void Wait()
        {
            std::unique_lock lock{ m_mutex };
            m_condition.Wait(lock, [this] {
                return m_called;
            });
        }

        Threading::Mutex m_mutex;
        Threading::ConditionVariable m_condition;
        bool m_called = false;
        IO::AsyncOperationStatus m_status = IO::AsyncOperationStatus::kQueued;
        IO::ResultCode m_result = IO::ResultCode::kUnknownError;
        size_t m_bytesRead = 0;
        festd::vector<std::byte> m_data;
    };


    festd::vector<std::byte> MakeAsyncTestData(const uint32_t size)
    {
        DefaultRandom random;

        festd::vector<std::byte> result(size);
        for (uint32_t index = 0; index < size; ++index)
            result[index] = static_cast<std::byte>(random.RandUInt64() & 0xff);

        return result;
    }


    Rc<IO::IStream> OpenTestStream(const festd::span<const std::byte> data)
    {
        const IO::Path path = "async-stream-io-compression.ferrum-test-file.bin";

        auto* streamFactory = Env::GetServiceProvider()->ResolveRequired<IO::IStreamFactory>();

        auto streamResult = streamFactory->OpenFileStream(path, IO::OpenMode::kCreate);
        EXPECT_TRUE(streamResult);

        Rc<IO::IStream> stream = streamResult.value();
        EXPECT_EQ(stream->WriteFromBuffer(data.data(), data.size()), data.size());
        stream->Close();

        streamResult = streamFactory->OpenFileStream(path, IO::OpenMode::kReadOnly);
        EXPECT_TRUE(streamResult);
        return streamResult.value();
    }


    void RunCompressedRead(const Compression::Method method)
    {
        const festd::vector<std::byte> source = MakeAsyncTestData(Compression::kBlockSize + 4096);

        auto compressor = Compression::Compressor::Create(method);
        festd::vector<std::byte> compressed(static_cast<uint32_t>(compressor.GetBounds(source.size())));
        const Compression::CompressionResult compressionResult =
            compressor.Compress(source.data(), source.size(), compressed.data(), compressed.size());
        ASSERT_EQ(compressionResult.m_result, Compression::ResultCode::kSuccess);
        compressed.resize(static_cast<uint32_t>(compressionResult.m_compressedSize));

        Rc<IO::IStream> stream = OpenTestStream(compressed);

        ImmediateJobSystem jobSystem;
        IO::AsyncStreamIO asyncIO{ nullptr, &jobSystem, nullptr };

        ReadCallback callback;
        IO::AsyncReadRequest request;
        request.m_stream = stream;
        request.m_callback = &callback;
        request.m_readBufferSize = source.size();
        request.m_compressedSize = compressed.size();
        request.m_compressionMethod = method;

        asyncIO.ReadAsync(request, IO::Priority::kNormal, nullptr);
        callback.Wait();

        EXPECT_EQ(jobSystem.m_scheduledJobCount, 1);
        EXPECT_EQ(callback.m_status, IO::AsyncOperationStatus::kSucceeded);
        EXPECT_EQ(callback.m_result, IO::ResultCode::kSuccess);
        EXPECT_EQ(callback.m_bytesRead, source.size());
        EXPECT_EQ(callback.m_data, source);
    }


    void RunFailedRead(const uint32_t compressedSize, const Compression::Method method, const IO::ResultCode expectedResult,
                       const uint32_t expectedJobs)
    {
        const festd::vector<std::byte> source = MakeAsyncTestData(32);
        Rc<IO::IStream> stream = OpenTestStream(source);
        ImmediateJobSystem jobSystem;
        IO::AsyncStreamIO asyncIO{ nullptr, &jobSystem, nullptr };
        ReadCallback callback;

        IO::AsyncReadRequest request;
        request.m_stream = stream;
        request.m_callback = &callback;
        request.m_readBufferSize = 128;
        request.m_compressedSize = compressedSize;
        request.m_compressionMethod = method;

        asyncIO.ReadAsync(request, IO::Priority::kNormal, nullptr);
        callback.Wait();

        EXPECT_EQ(jobSystem.m_scheduledJobCount, expectedJobs);
        EXPECT_EQ(callback.m_status, IO::AsyncOperationStatus::kFailed);
        EXPECT_EQ(callback.m_result, expectedResult);
    }
} // namespace


TEST(AsyncStreamIO, RawReadByDefault)
{
    const festd::vector<std::byte> source = MakeAsyncTestData(4096);
    Rc<IO::IStream> stream = OpenTestStream(source);

    ImmediateJobSystem jobSystem;
    IO::AsyncStreamIO asyncIO{ nullptr, &jobSystem, nullptr };

    ReadCallback callback;
    IO::AsyncReadRequest request;
    request.m_stream = stream;
    request.m_callback = &callback;
    request.m_readBufferSize = source.size();

    asyncIO.ReadAsync(request, IO::Priority::kNormal, nullptr);
    callback.Wait();

    EXPECT_EQ(jobSystem.m_scheduledJobCount, 0);
    EXPECT_EQ(callback.m_status, IO::AsyncOperationStatus::kSucceeded);
    EXPECT_EQ(callback.m_result, IO::ResultCode::kSuccess);
    EXPECT_EQ(callback.m_data, source);
}


TEST(AsyncStreamIO, DeflateRead)
{
    RunCompressedRead(Compression::Method::kDeflate);
}


TEST(AsyncStreamIO, ZstdRead)
{
    RunCompressedRead(Compression::Method::kZstd);
}


TEST(AsyncStreamIO, TruncatedCompressedRead)
{
    RunFailedRead(64, Compression::Method::kZstd, IO::ResultCode::kIOError, 0);
}


TEST(AsyncStreamIO, DecompressionFailure)
{
    RunFailedRead(32, Compression::Method::kZstd, IO::ResultCode::kDecompressionError, 1);
}
