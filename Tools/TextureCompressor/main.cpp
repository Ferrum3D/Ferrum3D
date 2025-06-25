#include <FeCore/Compression/Compression.h>
#include <FeCore/Console/Console.h>
#include <FeCore/IO/IStreamFactory.h>
#include <FeCore/IO/Path.h>
#include <FeCore/Modules/Configuration.h>
#include <Framework/Application/Application.h>
#include <Framework/Module.h>
#include <Graphics/Assets/TextureAssetFormat.h>
#include <Graphics/Core/Module.h>
#include <Graphics/Module.h>

#include <bc7enc/bc7enc.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace FE;
using namespace FE::Graphics;


namespace
{
    void WriteCompactedPages(const festd::span<const std::byte> compressedBuffer, IO::IStream* out)
    {
        // We need to compact pages for some compression methods.

        Memory::BlockReader reader{ compressedBuffer };

        const auto& blockHeader = reader.Read<Compression::BlockHeader>();
        FE_Verify(out->Write(blockHeader));

        auto pageHeader = reader.Read<Compression::PageHeader>();
        for (;;)
        {
            Compression::PageHeader newHeader = pageHeader;
            if (newHeader.m_nextPageOffset != kInvalidIndex)
                newHeader.m_nextPageOffset = pageHeader.m_compressedSize;

            FE_Verify(out->Write(newHeader));

            FE_Verify(out->WriteFromBuffer(reader.m_ptr, pageHeader.m_compressedSize) == pageHeader.m_compressedSize);
            if (pageHeader.m_nextPageOffset == kInvalidIndex)
            {
                reader.m_ptr += pageHeader.m_compressedSize;
                break;
            }

            reader.m_ptr += pageHeader.m_nextPageOffset;
            pageHeader = reader.Read<Compression::PageHeader>();
        }

        FE_Verify(out->Write(reader.Read<Compression::BlockFooter>()));
    }
} // namespace


struct App final : public Framework::Application
{
    App(const int32_t argc, const char** argv)
        : Application(argc, argv)
    {
    }

    void InitializeApp()
    {
        FE_PROFILER_ZONE();

        DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();

        m_logger = serviceProvider->ResolveRequired<Logger>();
        m_logSink = festd::make_unique<Framework::StdoutLogSink>(m_logger.Get());

        m_streamFactory = serviceProvider->ResolveRequired<IO::IStreamFactory>();
    }

private:
    Rc<WaitGroup> ScheduleUpdate() override
    {
        const auto file = m_streamFactory->OpenFileStream("Images/test1.png", IO::OpenMode::kReadOnly).value();

        festd::vector<std::byte> pngData;
        pngData.resize(static_cast<uint32_t>(file->Length()));
        FE_Verify(file->ReadToBuffer(pngData) == pngData.size());

        int32_t width, height, channels;
        auto* imageData = stbi_load_from_memory(
            reinterpret_cast<stbi_uc*>(pngData.data()), static_cast<int32_t>(pngData.size()), &width, &height, &channels, 4);

        auto deferFree = festd::defer([imageData] {
            stbi_image_free(imageData);
        });

        bc7enc_compress_block_params params;
        bc7enc_compress_block_params_init(&params);

        bc7enc_compress_block_init();

        const int32_t blockCountX = Math::CeilDivide(width, 4);
        const int32_t blockCountY = Math::CeilDivide(height, 4);

        festd::vector<__m128> compressedData;
        compressedData.resize(blockCountX * blockCountY);

        m_logger->LogInfo("Compression started");

        for (int32_t yb = 0; yb < blockCountY; ++yb)
        {
            for (int32_t xb = 0; xb < blockCountX; ++xb)
            {
                __m128 inputBlock[4];
                inputBlock[0] = _mm_loadu_ps(reinterpret_cast<const float*>(imageData + (yb * 4 + 0) * width * 4 + xb * 4 * 4));
                inputBlock[1] = _mm_loadu_ps(reinterpret_cast<const float*>(imageData + (yb * 4 + 1) * width * 4 + xb * 4 * 4));
                inputBlock[2] = _mm_loadu_ps(reinterpret_cast<const float*>(imageData + (yb * 4 + 2) * width * 4 + xb * 4 * 4));
                inputBlock[3] = _mm_loadu_ps(reinterpret_cast<const float*>(imageData + (yb * 4 + 3) * width * 4 + xb * 4 * 4));

                __m128* outputBlock = &compressedData[yb * blockCountX + xb];

                bc7enc_compress_block(outputBlock, inputBlock, &params);
            }

            m_logger->LogInfo("Progress: {}/{} blocks", yb, blockCountY);
        }

        constexpr uint32_t fileBlockByteSize = Compression::kBlockSize;
        festd::vector<std::byte> tempUncompressedBuffer{ fileBlockByteSize };
        festd::vector<std::byte> tempCompressedBuffer;

        const IO::Path fileName{ "Images/test1.ftx" };

        m_logger->LogInfo("Compressing to {}", fileName);

        auto cpuContext = Compression::Compressor::Create(Compression::Method::kDeflate);
        auto gpuContext = Compression::Compressor::Create(Compression::Method::kGDeflate);

        auto out = m_streamFactory->OpenFileStream(fileName, IO::OpenMode::kCreate).value();

        tempCompressedBuffer.resize(static_cast<uint32_t>(cpuContext.GetBounds(fileBlockByteSize)));

        Memory::BlockWriter writer{ tempUncompressedBuffer };

        Data::TextureHeader header;
        header.m_magic = Data::kTextureMagic;
        header.m_desc.m_width = width;
        header.m_desc.m_height = height;
        header.m_desc.m_sampleCount = 1;
        header.m_desc.m_depth = 1;
        header.m_desc.m_arraySize = 1;
        header.m_desc.m_mipSliceCount = 1;
        header.m_desc.m_dimension = Core::ImageDimension::k2D;
        header.m_desc.m_imageFormat = Core::Format::kBC7_UNORM;
        writer.Write(header);

        const uint32_t compressedDataByteSize = festd::size_bytes(compressedData);

        Data::MipChainInfo mipChainInfo;
        mipChainInfo.m_mostDetailedMipSlice = 0;
        mipChainInfo.m_mipSliceCount = 1;
        mipChainInfo.m_arraySlice = 0;
        mipChainInfo.m_blockCount = Math::CeilDivide(compressedDataByteSize, fileBlockByteSize);
        mipChainInfo.m_reserved = 0;
        writer.Write(mipChainInfo);

        Crc32 crc32;

        FE_Verify(cpuContext.Compress(crc32,
                                      tempUncompressedBuffer.data(),
                                      writer.m_ptr - tempUncompressedBuffer.data(),
                                      tempCompressedBuffer.data(),
                                      tempCompressedBuffer.size()));

        WriteCompactedPages(tempCompressedBuffer, out.Get());

        tempCompressedBuffer.resize(static_cast<uint32_t>(gpuContext.GetBounds(fileBlockByteSize)));

        for (uint32_t blockByteOffset = 0; blockByteOffset < compressedDataByteSize; blockByteOffset += fileBlockByteSize)
        {
            const uint32_t blockBytesEnd = Math::Min(blockByteOffset + fileBlockByteSize, compressedDataByteSize);
            const uint32_t blockByteSize = blockBytesEnd - blockByteOffset;

            FE_Verify(gpuContext.Compress(crc32,
                                          reinterpret_cast<const std::byte*>(compressedData.data()) + blockByteOffset,
                                          blockByteSize,
                                          tempCompressedBuffer.data(),
                                          tempCompressedBuffer.size()));

            WriteCompactedPages(tempCompressedBuffer, out.Get());
        }

        out.Reset();

        m_logger->LogInfo("Done");

        return nullptr;
    }

    festd::unique_ptr<Framework::StdoutLogSink> m_logSink;
    Rc<Logger> m_logger;
    Rc<IO::IStreamFactory> m_streamFactory;
};


int main(const int32_t argc, const char** argv)
{
    Env::ApplicationInfo applicationInfo;
    applicationInfo.m_name = "TextureCompressor";

    Framework::Module::Init();
    Core::Module::Init();
    Module::Init();
    Env::Init(applicationInfo);

    std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear);

    auto* application = Memory::New<App>(allocator, argc, argv);
    application->InitializeCore();

    IJobSystem* jobSystem = Env::GetServiceProvider()->ResolveRequired<IJobSystem>();

    int32_t exitCode = 0;
    FunctorJob mainJob([application, jobSystem, &exitCode] {
        application->InitializeApp();
        exitCode = application->Run();
        jobSystem->Stop();
    });

    mainJob.Schedule(jobSystem, FiberAffinityMask::kMainThread);
    jobSystem->Start();

    Memory::Delete(allocator, application);
    Env::Module::ShutdownModules();
    return exitCode;
}
