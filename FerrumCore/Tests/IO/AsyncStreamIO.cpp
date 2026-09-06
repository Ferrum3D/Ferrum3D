#include <Core/IO/Assets.h>
#include <Core/IO/Async.h>
#include <Core/IO/Platform/PlatformFile.h>
#include <Core/IO/StreamBase.h>
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


TEST(AsyncStreamIO, ReadAppendUsesExactRangeAndPreservesPrefix)
{
    const festd::vector<std::byte> source = MakeAsyncTestData(256);
    const IO::Path path = MakeTestPath("exact-append");
    WriteTestFile(path, source);

    festd::pmr::vector<std::byte> destination(7, std::byte{ 0x5a });
    CompletionLatch latch;

    IO::Async::Batch batch({ .m_filePath = path, .m_byteOffset = 16, .m_byteSize = 128 });
    batch.ReadAppend(destination, 24, 11);
    batch.InvokeOnCompletion([&] {
        latch.Signal();
    });

    auto controller = SubmitAndWait(batch, latch);
    EXPECT_EQ(controller->GetStatus(), IO::Async::Status::kSucceeded);
    ASSERT_EQ(destination.size(), 31);
    EXPECT_TRUE(std::all_of(destination.begin(), destination.begin() + 7, [](const std::byte value) {
        return value == std::byte{ 0x5a };
    }));
    EXPECT_TRUE(std::equal(destination.begin() + 7, destination.end(), source.begin() + 27));
}


TEST(AsyncStreamIO, MultipleCommandsAppendToOneVector)
{
    const festd::vector<std::byte> source = MakeAsyncTestData(256);
    const IO::Path path = MakeTestPath("multiple-append");
    WriteTestFile(path, source);

    festd::pmr::vector<std::byte> destination(3, std::byte{ 0x2a });
    CompletionLatch latch;

    IO::Async::Batch batch({ .m_filePath = path, .m_byteOffset = 0, .m_byteSize = source.size() });
    batch.ReadAppend(destination, 32, 8);
    batch.ReadAppend(destination, 48, 96);
    batch.InvokeOnCompletion([&] {
        latch.Signal();
    });

    auto controller = SubmitAndWait(batch, latch);
    EXPECT_EQ(controller->GetStatus(), IO::Async::Status::kSucceeded);
    ASSERT_EQ(destination.size(), 83);
    EXPECT_TRUE(std::equal(destination.begin() + 3, destination.begin() + 35, source.begin() + 8));
    EXPECT_TRUE(std::equal(destination.begin() + 35, destination.end(), source.begin() + 96));
}


TEST(AsyncStreamIO, ReadAppendToEndUsesPhysicalFileForUnknownRange)
{
    const festd::vector<std::byte> source = MakeAsyncTestData(256);
    const IO::Path path = MakeTestPath("append-to-end");
    WriteTestFile(path, source);

    festd::pmr::vector<std::byte> destination;
    CompletionLatch latch;

    IO::Async::Batch batch({ .m_filePath = path, .m_byteOffset = 32, .m_byteSize = 0 });
    batch.ReadAppendToEnd(destination, 12);
    batch.InvokeOnCompletion([&] {
        latch.Signal();
    });

    auto controller = SubmitAndWait(batch, latch);
    EXPECT_EQ(controller->GetStatus(), IO::Async::Status::kSucceeded);
    ASSERT_EQ(destination.size(), source.size() - 44);
    EXPECT_TRUE(std::equal(destination.begin(), destination.end(), source.begin() + 44));
}


TEST(AsyncStreamIO, ReadAppendToEndUsesBoundedSourceRange)
{
    const festd::vector<std::byte> source = MakeAsyncTestData(256);
    const IO::Path path = MakeTestPath("bounded-append-to-end");
    WriteTestFile(path, source);

    festd::pmr::vector<std::byte> destination;
    CompletionLatch latch;

    IO::Async::Batch batch({ .m_filePath = path, .m_byteOffset = 40, .m_byteSize = 80 });
    batch.ReadAppendToEnd(destination, 24);
    batch.InvokeOnCompletion([&] {
        latch.Signal();
    });

    auto controller = SubmitAndWait(batch, latch);
    EXPECT_EQ(controller->GetStatus(), IO::Async::Status::kSucceeded);
    ASSERT_EQ(destination.size(), 56);
    EXPECT_TRUE(std::equal(destination.begin(), destination.end(), source.begin() + 64));
}


TEST(AsyncStreamIO, ZeroLengthAppendIsEmpty)
{
    const festd::vector<std::byte> source = MakeAsyncTestData(32);
    const IO::Path path = MakeTestPath("zero-append");
    WriteTestFile(path, source);

    festd::pmr::vector<std::byte> destination(5, std::byte{ 0x7f });
    CompletionLatch latch;

    IO::Async::Batch batch({ .m_filePath = path, .m_byteOffset = 0, .m_byteSize = source.size() });
    batch.ReadAppend(destination, 0);
    batch.InvokeOnCompletion([&] {
        latch.Signal();
    });

    auto controller = SubmitAndWait(batch, latch);
    EXPECT_EQ(controller->GetStatus(), IO::Async::Status::kSucceeded);
    EXPECT_EQ(destination, festd::pmr::vector<std::byte>(5, std::byte{ 0x7f }));
}


TEST(AsyncStreamIO, TruncatedExactReadFails)
{
    const festd::vector<std::byte> source = MakeAsyncTestData(32);
    const IO::Path path = MakeTestPath("truncated-raw");
    WriteTestFile(path, source);

    festd::pmr::vector<std::byte> destination;
    CompletionLatch latch;

    IO::Async::Batch batch({ .m_filePath = path, .m_byteOffset = 0, .m_byteSize = 0 });
    batch.ReadAppend(destination, source.size() + 1);
    batch.InvokeOnCompletion([&] {
        latch.Signal();
    });

    auto controller = SubmitAndWait(batch, latch);
    EXPECT_EQ(controller->GetStatus(), IO::Async::Status::kFailed);
    EXPECT_EQ(controller->GetLastOperationResult(), IO::ResultCode::kIOError);
}


TEST(AsyncStreamIO, OversizedAppendFailsBeforeAllocation)
{
    const festd::vector<std::byte> source = MakeAsyncTestData(1);
    const IO::Path path = MakeTestPath("oversized-append");
    WriteTestFile(path, source);

    festd::pmr::vector<std::byte> firstDestination(2, std::byte{ 0x19 });
    festd::pmr::vector<std::byte> destination;
    CompletionLatch latch;

    IO::Async::Batch batch({ .m_filePath = path, .m_byteOffset = 0, .m_byteSize = 0 });
    batch.ReadAppend(firstDestination, 1);
    batch.ReadAppend(destination, static_cast<size_t>(Constants::kMaxU32) + 1);
    batch.InvokeOnCompletion([&] {
        latch.Signal();
    });

    auto controller = SubmitAndWait(batch, latch);
    EXPECT_EQ(controller->GetStatus(), IO::Async::Status::kFailed);
    EXPECT_EQ(controller->GetLastOperationResult(), IO::ResultCode::kInvalidArgument);
    EXPECT_EQ(firstDestination, festd::pmr::vector<std::byte>(2, std::byte{ 0x19 }));
    EXPECT_TRUE(destination.empty());
}


TEST(AsyncStreamIO, InvalidBoundedAppendLeavesDestinationUnchanged)
{
    const festd::vector<std::byte> source = MakeAsyncTestData(16);
    const IO::Path path = MakeTestPath("invalid-bounded-append");
    WriteTestFile(path, source);

    festd::pmr::vector<std::byte> destination(4, std::byte{ 0x22 });
    CompletionLatch latch;
    IO::Async::Batch batch({ .m_filePath = path, .m_byteOffset = 0, .m_byteSize = 10 });
    batch.ReadAppend(destination, 5, 8);
    batch.InvokeOnCompletion([&] {
        latch.Signal();
    });

    auto controller = SubmitAndWait(batch, latch);
    EXPECT_EQ(controller->GetStatus(), IO::Async::Status::kFailed);
    EXPECT_EQ(controller->GetLastOperationResult(), IO::ResultCode::kInvalidArgument);
    EXPECT_EQ(destination, festd::pmr::vector<std::byte>(4, std::byte{ 0x22 }));
}


TEST(AsyncStreamIO, PhysicalOffsetOverflowLeavesDestinationUnchanged)
{
    const festd::vector<std::byte> source = MakeAsyncTestData(16);
    const IO::Path path = MakeTestPath("offset-overflow");
    WriteTestFile(path, source);

    festd::pmr::vector<std::byte> destination(4, std::byte{ 0x33 });
    CompletionLatch latch;
    IO::Async::Batch batch({ .m_filePath = path, .m_byteOffset = Constants::kMaxValue<size_t>, .m_byteSize = 0 });
    batch.ReadAppend(destination, 1, 1);
    batch.InvokeOnCompletion([&] {
        latch.Signal();
    });

    auto controller = SubmitAndWait(batch, latch);
    EXPECT_EQ(controller->GetStatus(), IO::Async::Status::kFailed);
    EXPECT_EQ(controller->GetLastOperationResult(), IO::ResultCode::kInvalidArgument);
    EXPECT_EQ(destination, festd::pmr::vector<std::byte>(4, std::byte{ 0x33 }));
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


TEST(AsyncStreamIO, CompressedReadAppend)
{
    const festd::vector<std::byte> source(4096, std::byte{ 0x6a });
    auto compressor = Compression::Compressor::Create(Compression::Method::kZstd);
    festd::vector<std::byte> compressed(static_cast<uint32_t>(compressor.GetBounds(source.size())));
    const Compression::CompressionResult compressionResult =
        compressor.Compress(source.data(), source.size(), compressed.data(), compressed.size());
    ASSERT_EQ(compressionResult.m_result, Compression::ResultCode::kSuccess);
    compressed.resize(static_cast<uint32_t>(compressionResult.m_compressedSize));
    ASSERT_LT(compressed.size(), source.size());

    festd::vector<std::byte> compressedFile;
    compressedFile.reserve(compressed.size() * 2);
    compressedFile.insert(compressedFile.end(), compressed.begin(), compressed.end());
    compressedFile.insert(compressedFile.end(), compressed.begin(), compressed.end());

    const IO::Path path = MakeTestPath("compressed-append");
    WriteTestFile(path, compressedFile);

    festd::pmr::vector<std::byte> destination(9, std::byte{ 0x31 });
    CompletionLatch latch;
    IO::Async::Batch batch({ .m_filePath = path, .m_byteOffset = 0, .m_byteSize = compressedFile.size() });
    batch.ReadAppend(destination, Compression::Method::kZstd, compressed.size(), source.size());
    batch.ReadAppend(destination, Compression::Method::kZstd, compressed.size(), source.size(), compressed.size());
    batch.InvokeOnCompletion([&] {
        latch.Signal();
    });

    auto controller = SubmitAndWait(batch, latch);
    EXPECT_EQ(controller->GetStatus(), IO::Async::Status::kSucceeded);
    ASSERT_EQ(destination.size(), source.size() * 2 + 9);
    EXPECT_TRUE(std::equal(destination.begin() + 9, destination.begin() + 9 + source.size(), source.begin()));
    EXPECT_TRUE(std::equal(destination.begin() + 9 + source.size(), destination.end(), source.begin()));
}


TEST(AsyncStreamIO, DecompressedSizeMismatchFails)
{
    const festd::vector<std::byte> source(256, std::byte{ 0x4b });
    auto compressor = Compression::Compressor::Create(Compression::Method::kZstd);
    festd::vector<std::byte> compressed(static_cast<uint32_t>(compressor.GetBounds(source.size())));
    const Compression::CompressionResult compressionResult =
        compressor.Compress(source.data(), source.size(), compressed.data(), compressed.size());
    ASSERT_EQ(compressionResult.m_result, Compression::ResultCode::kSuccess);
    compressed.resize(static_cast<uint32_t>(compressionResult.m_compressedSize));

    const IO::Path path = MakeTestPath("decompressed-size-mismatch");
    WriteTestFile(path, compressed);

    festd::pmr::vector<std::byte> destination;
    CompletionLatch latch;
    IO::Async::Batch batch({ .m_filePath = path, .m_byteOffset = 0, .m_byteSize = compressed.size() });
    batch.ReadAppend(destination, Compression::Method::kZstd, compressed.size(), source.size() + 1);
    batch.InvokeOnCompletion([&] {
        latch.Signal();
    });

    auto controller = SubmitAndWait(batch, latch);
    EXPECT_EQ(controller->GetStatus(), IO::Async::Status::kFailed);
    EXPECT_EQ(controller->GetLastOperationResult(), IO::ResultCode::kDecompressionError);
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


TEST(ReadOnlyMemoryStream, CursorSeekEofAndClose)
{
    const std::array data{ std::byte{ 1 }, std::byte{ 2 }, std::byte{ 3 }, std::byte{ 4 } };
    IO::ReadOnlyMemoryStream stream(data);
    std::array<std::byte, 3> destination{};

    EXPECT_TRUE(stream.IsOpen());
    EXPECT_EQ(stream.ReadToBuffer(destination.data(), 2), 2);
    EXPECT_EQ(stream.Tell(), 2);
    EXPECT_EQ(destination[0], data[0]);
    EXPECT_EQ(destination[1], data[1]);

    EXPECT_EQ(stream.ReadToBuffer(destination.data(), destination.size()), 2);
    EXPECT_EQ(stream.Tell(), data.size());
    EXPECT_EQ(stream.ReadToBuffer(destination.data(), 1), 0);

    EXPECT_EQ(stream.Seek(-2, IO::SeekMode::kCurrent), IO::ResultCode::kSuccess);
    EXPECT_EQ(stream.Tell(), 2);
    EXPECT_EQ(stream.Seek(3, IO::SeekMode::kCurrent), IO::ResultCode::kInvalidSeek);
    EXPECT_EQ(stream.Tell(), 2);
    EXPECT_EQ(stream.Seek(-5, IO::SeekMode::kEnd), IO::ResultCode::kInvalidSeek);
    EXPECT_EQ(stream.Tell(), 2);
    EXPECT_EQ(stream.Seek(Constants::kMinValue<intptr_t>, IO::SeekMode::kCurrent), IO::ResultCode::kInvalidSeek);
    EXPECT_EQ(stream.Tell(), 2);
    EXPECT_EQ(stream.Seek(0, IO::SeekMode::kBegin), IO::ResultCode::kSuccess);

    stream.Close();
    EXPECT_FALSE(stream.IsOpen());
    EXPECT_EQ(stream.Length(), 0);
    EXPECT_EQ(stream.Tell(), 0);
    EXPECT_EQ(stream.ReadToBuffer(destination.data(), 1), 0);
    EXPECT_EQ(stream.Seek(0, IO::SeekMode::kBegin), IO::ResultCode::kInvalidSeek);
}


TEST(ReadOnlyMemoryStream, EmptyOpenStream)
{
    IO::ReadOnlyMemoryStream stream;
    EXPECT_FALSE(stream.IsOpen());

    stream.OpenInPlace(nullptr, 0);
    EXPECT_TRUE(stream.IsOpen());
    EXPECT_EQ(stream.ReadToBuffer(nullptr, 0), 0);
    EXPECT_EQ(stream.Seek(0, IO::SeekMode::kEnd), IO::ResultCode::kSuccess);
    EXPECT_EQ(stream.Seek(1, IO::SeekMode::kBegin), IO::ResultCode::kInvalidSeek);
    EXPECT_EQ(stream.Tell(), 0);
}


TEST(AssetHandles, ReferenceCountsAndLeaseConversion)
{
    IO::AssetSlot slot{};
    int instance = 42;
    slot.m_instance.store(&instance);

    {
        IO::ResidencyTicket ticket(&slot);
        EXPECT_EQ(slot.m_strongRefCount.load(), 1);

        IO::AssetLease<int> lease(ticket);
        EXPECT_EQ(slot.m_strongRefCount.load(), 2);
        EXPECT_EQ(lease.Get(), &instance);

        IO::AssetHandle<int> handle(lease);
        EXPECT_EQ(slot.m_weakRefCount.load(), 1);
        EXPECT_EQ(handle.Get(), &instance);

        IO::AssetHandle<int> handleCopy = handle;
        EXPECT_EQ(slot.m_weakRefCount.load(), 2);
        IO::AssetHandle<int> handleMove = std::move(handleCopy);
        EXPECT_EQ(slot.m_weakRefCount.load(), 2);
        EXPECT_EQ(handleMove.GetAssetSlot(), &slot);

        IO::AssetHandle<int> assigned;
        assigned = handleMove;
        EXPECT_EQ(slot.m_weakRefCount.load(), 3);
        assigned = handleMove;
        EXPECT_EQ(slot.m_weakRefCount.load(), 3);

        IO::AssetHandle<int> moveAssigned;
        moveAssigned = std::move(assigned);
        EXPECT_EQ(slot.m_weakRefCount.load(), 3);
        EXPECT_EQ(assigned.GetAssetSlot(), nullptr);
        EXPECT_EQ(moveAssigned.GetAssetSlot(), &slot);

        handleMove.Invalidate();
        EXPECT_EQ(handleMove.GetAssetSlot(), nullptr);
        EXPECT_EQ(slot.m_weakRefCount.load(), 2);
        handleMove.Reset(&slot);
        handleMove.Reset(&slot);
        EXPECT_EQ(slot.m_weakRefCount.load(), 3);

        IO::AssetHandle<int> adopted;
        adopted.Adopt(moveAssigned.Detach());
        EXPECT_EQ(moveAssigned.GetAssetSlot(), nullptr);
        EXPECT_EQ(adopted.GetAssetSlot(), &slot);
        EXPECT_EQ(slot.m_weakRefCount.load(), 3);
    }

    EXPECT_EQ(slot.m_strongRefCount.load(), 0);
    EXPECT_EQ(slot.m_weakRefCount.load(), 0);
}
