#include <FeCore/Compression/Compression.h>
#include <FeCore/Compression/CompressionInternal.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Modules/EnvironmentPrivate.h>

#include <libdeflate.h>

namespace FE::Compression
{
    namespace
    {
        uint32_t EncodeBlockMagic(const Method method)
        {
            return Math::MakeFourCC('F', 'C', 'B', festd::to_underlying(method));
        }


        void* AllocateCompressorImpl(const int32_t level)
        {
            Env::Internal::SharedState& state = Env::Internal::SharedState::Get();

            std::unique_lock lock{ state.m_lock };
            auto& cache = state.m_compressorCache[level - 1];
            if (cache.empty())
                return libdeflate_alloc_compressor(level);

            void* impl = cache.back();
            cache.pop_back();
            return impl;
        }


        void* AllocateGDeflateCompressorImpl(const int32_t level)
        {
            Env::Internal::SharedState& state = Env::Internal::SharedState::Get();

            std::unique_lock lock{ state.m_lock };
            auto& cache = state.m_gDeflateCompressorCache[level - 1];
            if (cache.empty())
                return libdeflate_alloc_gdeflate_compressor(level);

            void* impl = cache.back();
            cache.pop_back();
            return impl;
        }


        void* AllocateDecompressorImpl()
        {
            Env::Internal::SharedState& state = Env::Internal::SharedState::Get();

            std::unique_lock lock{ state.m_lock };
            auto& cache = state.m_decompressorCache;
            if (cache.empty())
                return libdeflate_alloc_decompressor();

            void* impl = cache.back();
            cache.pop_back();
            return impl;
        }


        void* AllocateGDeflateDecompressorImpl()
        {
            Env::Internal::SharedState& state = Env::Internal::SharedState::Get();

            std::unique_lock lock{ state.m_lock };
            auto& cache = state.m_gDeflateDecompressorCache;
            if (cache.empty())
                return libdeflate_alloc_gdeflate_decompressor();

            void* impl = cache.back();
            cache.pop_back();
            return impl;
        }


        libdeflate_compressor* CastCompressor(void* impl)
        {
            return static_cast<libdeflate_compressor*>(impl);
        }


        libdeflate_gdeflate_compressor* CastGCompressor(void* impl)
        {
            return static_cast<libdeflate_gdeflate_compressor*>(impl);
        }


        libdeflate_decompressor* CastDecompressor(void* impl)
        {
            return static_cast<libdeflate_decompressor*>(impl);
        }


        libdeflate_gdeflate_decompressor* CastGDecompressor(void* impl)
        {
            return static_cast<libdeflate_gdeflate_decompressor*>(impl);
        }
    } // namespace


    void Internal::Init()
    {
        libdeflate_set_memory_allocator(&Memory::DefaultAllocate, &Memory::DefaultFree);
    }


    void Internal::Shutdown()
    {
        Env::Internal::SharedState& state = Env::Internal::SharedState::Get();

        for (auto& cache : state.m_compressorCache)
        {
            for (void* impl : cache)
                libdeflate_free_compressor(static_cast<libdeflate_compressor*>(impl));

            cache.clear();
        }
    }


    Compressor::~Compressor()
    {
        if (!m_impl)
            return;

        Env::Internal::SharedState& state = Env::Internal::SharedState::Get();

        std::unique_lock lock{ state.m_lock };

        if (m_method == Method::kGDeflate)
            state.m_gDeflateCompressorCache[m_level - 1].push_back(m_impl);
        else
            state.m_compressorCache[m_level - 1].push_back(m_impl);

        m_impl = nullptr;
    }


    void Compressor::Reset()
    {
        Compressor empty;
        std::swap(*this, empty);
    }


    size_t Compressor::GetBounds(const size_t uncompressedSize) const
    {
        constexpr uint32_t perBlockMetadataSize = sizeof(BlockHeader) + sizeof(BlockFooter);

        switch (m_method)
        {
        default:
        case Method::kInvalid:
            FE_DebugBreak();
            [[fallthrough]];

        case Method::kNone:
            return uncompressedSize + perBlockMetadataSize + sizeof(PageHeader);

        case Method::kDeflate:
            return libdeflate_deflate_compress_bound(CastCompressor(m_impl), uncompressedSize) + perBlockMetadataSize
                + sizeof(PageHeader);

        case Method::kGDeflate:
            {
                size_t pageCount;
                const size_t bound = libdeflate_gdeflate_compress_bound(CastGCompressor(m_impl), uncompressedSize, &pageCount);
                return bound + perBlockMetadataSize + sizeof(PageHeader) * pageCount;
            }
        }
    }


    bool Compressor::Compress(Crc32& crc, const void* src, const size_t srcSize, void* dst, const size_t dstSize) const
    {
        FE_Assert(srcSize <= kBlockSize);

        if (sizeof(BlockHeader) > dstSize)
            return false;

        switch (m_method)
        {
        default:
        case Method::kInvalid:
            FE_DebugBreak();
            [[fallthrough]];

        case Method::kNone:
            {
                Memory::BlockWriter writer{ dst, dstSize };
                writer.Write(BlockHeader{ EncodeBlockMagic(m_method), static_cast<uint32_t>(srcSize) });

                if (sizeof(PageHeader) > writer.AvailableSpace())
                    return false;

                PageHeader& pageHeader = writer.Write<PageHeader>();
                pageHeader.m_compressedSize = static_cast<uint32_t>(srcSize);
                pageHeader.m_nextPageOffset = kInvalidIndex;

                if (!writer.WriteBytes(src, srcSize))
                    return false;

                if (sizeof(BlockFooter) > writer.AvailableSpace())
                    return false;

                writer.Write(BlockFooter{ static_cast<uint32_t>(srcSize), crc.Update(src, srcSize) });
                return true;
            }

        case Method::kDeflate:
            {
                Memory::BlockWriter writer{ dst, dstSize };
                writer.Write(BlockHeader{ EncodeBlockMagic(m_method), static_cast<uint32_t>(srcSize) });

                if (sizeof(PageHeader) > writer.AvailableSpace())
                    return false;

                PageHeader& pageHeader = writer.Write<PageHeader>();

                const size_t compressedBytes =
                    libdeflate_deflate_compress(CastCompressor(m_impl), src, srcSize, writer.m_ptr, writer.AvailableSpace());
                if (compressedBytes == 0)
                    return false;

                writer.m_ptr += compressedBytes;
                pageHeader.m_compressedSize = static_cast<uint32_t>(compressedBytes);
                pageHeader.m_nextPageOffset = kInvalidIndex;

                if (sizeof(BlockFooter) > writer.AvailableSpace())
                    return false;

                writer.Write(BlockFooter{ static_cast<uint32_t>(srcSize), crc.Update(src, srcSize) });
                return true;
            }

        case Method::kGDeflate:
            {
                Memory::BlockWriter writer{ dst, dstSize };
                writer.Write(BlockHeader{ EncodeBlockMagic(m_method), kGDeflatePageSize });

                size_t pageCount;
                const size_t bound = libdeflate_gdeflate_compress_bound(CastGCompressor(m_impl), srcSize, &pageCount);
                const size_t pageBound = bound / pageCount;

                festd::small_vector<libdeflate_gdeflate_out_page, 16> outPages;
                outPages.resize(static_cast<uint32_t>(pageCount));
                for (libdeflate_gdeflate_out_page& outPage : outPages)
                {
                    if (sizeof(PageHeader) > writer.AvailableSpace())
                        return false;

                    PageHeader& pageHeader = writer.Write<PageHeader>();
                    pageHeader.m_nextPageOffset = static_cast<uint32_t>(pageBound);

                    outPage.data = writer.m_ptr;
                    outPage.nbytes = pageBound;

                    if (pageBound > writer.AvailableSpace())
                        return false;

                    writer.m_ptr += pageBound;
                }

                const size_t compressedBytes =
                    libdeflate_gdeflate_compress(CastGCompressor(m_impl), src, srcSize, outPages.data(), outPages.size());

                if (compressedBytes == 0)
                    return false;

                if (sizeof(BlockFooter) > writer.AvailableSpace())
                    return false;

                for (libdeflate_gdeflate_out_page& outPage : outPages)
                {
                    PageHeader* pageHeader = static_cast<PageHeader*>(outPage.data) - 1;
                    FE_AssertDebug(pageHeader->m_nextPageOffset == pageBound);
                    FE_AssertDebug(pageHeader->m_compressedSize == 0);
                    pageHeader->m_compressedSize = static_cast<uint32_t>(outPage.nbytes);

                    if (&outPage == &outPages.back())
                    {
                        pageHeader->m_nextPageOffset = kInvalidIndex;
                        writer.m_ptr = reinterpret_cast<std::byte*>(pageHeader + 1) + pageHeader->m_compressedSize;

                        BlockFooter& footer = writer.Write<BlockFooter>();
                        footer.m_crc32 = crc.Update(src, srcSize);
                        footer.m_tailPageUncompressedSize =
                            srcSize % kGDeflatePageSize > 0 ? srcSize % kGDeflatePageSize : kGDeflatePageSize;
                    }
                }

                return true;
            }
        }
    }


    Compressor Compressor::Create(const Method method, const int32_t level)
    {
        switch (method)
        {
        default:
        case Method::kInvalid:
            FE_DebugBreak();
            [[fallthrough]];

        case Method::kNone:
            return Compressor{ method, level, nullptr };

        case Method::kDeflate:
            return Compressor{ method, level, AllocateCompressorImpl(level) };

        case Method::kGDeflate:
            return Compressor{ method, level, AllocateGDeflateCompressorImpl(level) };
        }
    }


    Decompressor::~Decompressor()
    {
        if (!m_impl)
            return;

        Env::Internal::SharedState& state = Env::Internal::SharedState::Get();

        std::unique_lock lock{ state.m_lock };

        if (m_method == Method::kGDeflate)
            state.m_gDeflateDecompressorCache.push_back(m_impl);
        else
            state.m_decompressorCache.push_back(m_impl);

        m_impl = nullptr;
    }


    void Decompressor::Reset()
    {
        Decompressor empty;
        std::swap(*this, empty);
    }


    DecompressionResult Decompressor::Decompress(const void* src, const size_t srcSize, void* dst, const size_t dstSize) const
    {
        switch (m_method)
        {
        default:
        case Method::kInvalid:
            FE_DebugBreak();
            [[fallthrough]];

        case Method::kNone:
            {
                if (dstSize < srcSize)
                    return DecompressionResult{ ResultCode::kInsufficientSpace, 0 };

                memcpy(dst, src, srcSize);

                return DecompressionResult{ ResultCode::kSuccess, srcSize };
            }

        case Method::kDeflate:
            {
                size_t decompressedSize;
                const libdeflate_result decompressionResult =
                    libdeflate_deflate_decompress(CastDecompressor(m_impl), src, srcSize, dst, dstSize, &decompressedSize);

                if (decompressedSize > dstSize)
                    return DecompressionResult{ ResultCode::kInsufficientSpace, 0 };

                if (decompressionResult == LIBDEFLATE_BAD_DATA)
                    return DecompressionResult{ ResultCode::kInvalidFormat, 0 };

                if (decompressionResult == LIBDEFLATE_INSUFFICIENT_SPACE)
                    return DecompressionResult{ ResultCode::kInsufficientSpace, 0 };

                if (decompressionResult != LIBDEFLATE_SUCCESS)
                    return DecompressionResult{ ResultCode::kUnknownError, 0 };

                return DecompressionResult{ ResultCode::kSuccess, decompressedSize };
            }

        case Method::kGDeflate:
            {
                size_t decompressedSize;
                libdeflate_gdeflate_in_page inPage;
                inPage.nbytes = srcSize;
                inPage.data = src;

                const libdeflate_result decompressionResult =
                    libdeflate_gdeflate_decompress(CastGDecompressor(m_impl), &inPage, 1, dst, dstSize, &decompressedSize);

                if (decompressionResult == LIBDEFLATE_BAD_DATA)
                    return DecompressionResult{ ResultCode::kInvalidFormat, 0 };

                if (decompressionResult == LIBDEFLATE_INSUFFICIENT_SPACE)
                    return DecompressionResult{ ResultCode::kInsufficientSpace, 0 };

                if (decompressionResult != LIBDEFLATE_SUCCESS)
                    return DecompressionResult{ ResultCode::kUnknownError, 0 };

                return DecompressionResult{ ResultCode::kSuccess, dstSize };
            }
        }
    }


    Decompressor Decompressor::Create(const Method method)
    {
        switch (method)
        {
        default:
        case Method::kInvalid:
            FE_DebugBreak();
            [[fallthrough]];

        case Method::kNone:
            return Decompressor{ method, nullptr };

        case Method::kDeflate:
            return Decompressor{ method, AllocateDecompressorImpl() };

        case Method::kGDeflate:
            return Decompressor{ method, AllocateGDeflateDecompressorImpl() };
        }
    }
} // namespace FE::Compression
