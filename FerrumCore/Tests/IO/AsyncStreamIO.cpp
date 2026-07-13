#include <Core/IO/Async.h>
#include <Core/IO/Platform/PlatformFile.h>
#include <Core/Math/Random.h>
#include <Core/Threading/ConditionVariable.h>
#include <Core/Threading/Mutex.h>
#include <gtest/gtest.h>

using namespace FE;

namespace
{
    struct CompletionLatch final
    {
        void Signal()
        {
            std::lock_guard lock{ m_mutex };
            m_called = true;
            m_condition.NotifyAll();
        }

        bool Wait()
        {
            std::unique_lock lock{ m_mutex };
            m_condition.Wait(lock, [this] {
                return m_called;
            });

            return true;
        }

        Threading::Mutex m_mutex;
        Threading::ConditionVariable m_condition;
        bool m_called = false;
    };


    festd::vector<std::byte> MakeAsyncTestData(const uint32_t size)
    {
        DefaultRandom random;

        festd::vector<std::byte> result(size);
        for (uint32_t index = 0; index < size; ++index)
            result[index] = static_cast<std::byte>(random.RandUInt64() & 0xff);

        return result;
    }


    IO::Path MakeTestPath(const festd::string_view suffix)
    {
        return IO::Path{ Fmt::FixedFormat("async-stream-io-{}.ferrum-test-file.bin", suffix) };
    }


    void WriteTestFile(const IO::Path& path, const festd::span<const std::byte> data)
    {
        Platform::FileHandle file;
        ASSERT_EQ(Platform::OpenFile(path, IO::OpenMode::kCreate, file), IO::ResultCode::kSuccess);

        size_t bytesWritten = 0;
        EXPECT_EQ(Platform::WriteFile(file, data.data(), data.size(), bytesWritten), IO::ResultCode::kSuccess);
        EXPECT_EQ(bytesWritten, data.size());
        Platform::CloseFile(file);
    }


    Rc<IO::Async::IController> SubmitAndWait(const IO::Async::Batch& batch, CompletionLatch& latch,
                                             const IO::Priority priority = IO::Priority::kNormal)
    {
        Rc<IO::Async::IController> controller = IO::Async::Read(batch, priority);
        latch.Wait();
        return controller;
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

        const IO::Path path = MakeTestPath(method == Compression::Method::kDeflate ? "deflate" : "zstd");
        WriteTestFile(path, compressed);

        festd::vector<std::byte> destination(source.size());
        CompletionLatch latch;

        IO::Async::Batch batch;
        batch.SetSource({ .m_filePath = path, .m_byteOffset = 0, .m_byteSize = compressed.size() });
        batch.Read(destination, compressed.size(), method);
        batch.InvokeOnCompletion([&] {
            latch.Signal();
        });

        auto controller = SubmitAndWait(batch, latch);
        EXPECT_EQ(controller->GetStatus(), IO::Async::Status::kSucceeded);
        EXPECT_EQ(controller->GetLastOperationResult(), IO::ResultCode::kSuccess);
        EXPECT_EQ(destination, source);
    }


    void RunFailedCompressedRead(const uint32_t compressedSize, const Compression::Method method,
                                 const IO::ResultCode expectedResult)
    {
        const festd::vector<std::byte> source = MakeAsyncTestData(32);
        const IO::Path path = MakeTestPath(compressedSize == 32 ? "truncated" : "bad-compression");
        WriteTestFile(path, source);

        festd::vector<std::byte> destination(128);
        CompletionLatch latch;

        IO::Async::Batch batch;
        batch.SetSource({ .m_filePath = path, .m_byteOffset = 0, .m_byteSize = compressedSize });
        batch.Read(destination, compressedSize, method);
        batch.InvokeOnCompletion([&] {
            latch.Signal();
        });

        auto controller = SubmitAndWait(batch, latch);
        EXPECT_EQ(controller->GetStatus(), IO::Async::Status::kFailed);
        EXPECT_EQ(controller->GetLastOperationResult(), expectedResult);
    }
} // namespace


TEST(AsyncStreamIO, RawPathRead)
{
    const festd::vector<std::byte> source = MakeAsyncTestData(4096);
    const IO::Path path = MakeTestPath("raw");
    WriteTestFile(path, source);

    festd::vector<std::byte> destination(source.size());
    CompletionLatch latch;

    IO::Async::Batch batch;
    batch.SetSource({ .m_filePath = path, .m_byteOffset = 0, .m_byteSize = source.size() });
    batch.Read(destination.data(), destination.size());
    batch.InvokeOnCompletion([&] {
        latch.Signal();
    });

    auto controller = SubmitAndWait(batch, latch);
    EXPECT_EQ(controller->GetStatus(), IO::Async::Status::kSucceeded);
    EXPECT_EQ(controller->GetLastOperationResult(), IO::ResultCode::kSuccess);
    EXPECT_EQ(destination, source);
}


TEST(AsyncStreamIO, RawPathReadAppend)
{
    const festd::vector<std::byte> source = MakeAsyncTestData(4096);
    const IO::Path path = MakeTestPath("raw-append");
    WriteTestFile(path, source);

    festd::pmr::vector<std::byte> destination;
    CompletionLatch latch;

    IO::Async::Batch batch;
    batch.SetSource({ .m_filePath = path, .m_byteOffset = 0, .m_byteSize = source.size() });
    batch.ReadAppend(destination, source.size());
    batch.InvokeOnCompletion([&] {
        latch.Signal();
    });

    auto controller = SubmitAndWait(batch, latch);
    EXPECT_EQ(controller->GetStatus(), IO::Async::Status::kSucceeded);
    EXPECT_EQ(controller->GetLastOperationResult(), IO::ResultCode::kSuccess);
    EXPECT_EQ(festd::span(destination), festd::span(source));
}


TEST(AsyncStreamIO, MultipleReadsAndSourceOffset)
{
    const festd::vector<std::byte> source = MakeAsyncTestData(512);
    const IO::Path path = MakeTestPath("multiple");
    WriteTestFile(path, source);

    festd::vector<std::byte> first(64);
    festd::vector<std::byte> second(96);
    CompletionLatch latch;

    IO::Async::Batch batch({ .m_filePath = path, .m_byteOffset = 32, .m_byteSize = source.size() - 32 });
    batch.Read(first, 0);
    batch.Read(second.data(), second.size(), 128);
    batch.InvokeOnCompletion([&] {
        latch.Signal();
    });

    auto controller = SubmitAndWait(batch, latch);
    EXPECT_EQ(controller->GetStatus(), IO::Async::Status::kSucceeded);
    EXPECT_TRUE(std::equal(first.begin(), first.end(), source.begin() + 32));
    EXPECT_TRUE(std::equal(second.begin(), second.end(), source.begin() + 160));
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
    RunFailedCompressedRead(64, Compression::Method::kZstd, IO::ResultCode::kDecompressionError);
}


TEST(AsyncStreamIO, DecompressionFailure)
{
    RunFailedCompressedRead(32, Compression::Method::kZstd, IO::ResultCode::kDecompressionError);
}


TEST(AsyncStreamIO, QueuedCancellation)
{
    const festd::vector<std::byte> source = MakeAsyncTestData(128);
    const IO::Path path = MakeTestPath("cancel");
    WriteTestFile(path, source);

    festd::vector<std::byte> destination(source.size());
    CompletionLatch latch;

    IO::Async::Batch batch({ .m_filePath = path, .m_byteOffset = 0, .m_byteSize = source.size() });
    batch.Read(destination.data(), destination.size());
    batch.InvokeOnCompletion([&] {
        latch.Signal();
    });

    auto controller = IO::Async::Read(batch, IO::Priority::kNormal);
    controller->Cancel();
    ASSERT_TRUE(latch.Wait());

    EXPECT_EQ(controller->GetStatus(), IO::Async::Status::kCanceled);
    EXPECT_EQ(controller->GetLastOperationResult(), IO::ResultCode::kCanceled);
}
