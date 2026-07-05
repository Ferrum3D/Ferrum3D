#include <Core/Compression/Compression.h>
#include <Core/Compression/CompressionPrivate.h>
#include <Core/Memory/Memory.h>
#include <festd/vector.h>

#include <libdeflate.h>

#define ZSTD_STATIC_LINKING_ONLY 1
#include <zstd.h>
#include <zstd_errors.h>

namespace FE::Compression
{
    namespace
    {
        constexpr int32_t kDeflateMaxLevel = 12;
        constexpr int32_t kZstdMaxLevel = 22;

        constexpr ZSTD_customMem kZstdCustomMem = {
            .customAlloc =
                [](void*, const size_t size) {
                    return Memory::DefaultAllocate(size);
                },
            .customFree =
                [](void*, void* ptr) {
                    Memory::DefaultFree(ptr);
                },
            .opaque = nullptr,
        };


        struct CompressionState final
        {
            Threading::SpinLock m_lock;
            festd::array<festd::vector<void*>, kDeflateMaxLevel> m_deflateCompressorCache;
            festd::array<festd::vector<void*>, kZstdMaxLevel> m_zstdCompressorCache;
            festd::vector<void*> m_deflateDecompressorCache;
            festd::vector<void*> m_zstdDecompressorCache;
        };

        CompressionState* GCompressionState;


        void* AllocateDeflateCompressor(const int32_t level)
        {
            FE_PROFILER_ZONE();

            FE_Assert(level >= 1 && level <= kDeflateMaxLevel);

            std::unique_lock lock{ GCompressionState->m_lock };
            auto& cache = GCompressionState->m_deflateCompressorCache[level - 1];
            if (cache.empty())
                return libdeflate_alloc_compressor(level);

            void* impl = cache.back();
            cache.pop_back();
            return impl;
        }


        void* AllocateZstdCompressor(const int32_t level)
        {
            FE_PROFILER_ZONE();

            FE_Assert(level >= 1 && level <= kZstdMaxLevel);

            std::unique_lock lock{ GCompressionState->m_lock };
            auto& cache = GCompressionState->m_zstdCompressorCache[level - 1];
            if (cache.empty())
                return ZSTD_createCCtx_advanced(kZstdCustomMem);

            void* impl = cache.back();
            cache.pop_back();
            return impl;
        }


        void* AllocateDeflateDecompressor()
        {
            FE_PROFILER_ZONE();

            std::unique_lock lock{ GCompressionState->m_lock };
            auto& cache = GCompressionState->m_deflateDecompressorCache;
            if (cache.empty())
                return libdeflate_alloc_decompressor();

            void* impl = cache.back();
            cache.pop_back();
            return impl;
        }


        void* AllocateZstdDecompressor()
        {
            FE_PROFILER_ZONE();

            std::unique_lock lock{ GCompressionState->m_lock };
            auto& cache = GCompressionState->m_zstdDecompressorCache;
            if (cache.empty())
                return ZSTD_createDCtx_advanced(kZstdCustomMem);

            void* impl = cache.back();
            cache.pop_back();
            return impl;
        }


        libdeflate_compressor* CastDeflateCompressor(void* impl)
        {
            return static_cast<libdeflate_compressor*>(impl);
        }


        ZSTD_CCtx* CastZstdCompressor(void* impl)
        {
            return static_cast<ZSTD_CCtx*>(impl);
        }


        libdeflate_decompressor* CastDeflateDecompressor(void* impl)
        {
            return static_cast<libdeflate_decompressor*>(impl);
        }


        ZSTD_DCtx* CastZstdDecompressor(void* impl)
        {
            return static_cast<ZSTD_DCtx*>(impl);
        }
    } // namespace


    void Internal::Init(std::pmr::memory_resource* allocator)
    {
        FE_Assert(GCompressionState == nullptr, "Compression already initialized");
        GCompressionState = Memory::New<CompressionState>(allocator);

        libdeflate_set_memory_allocator(&Memory::DefaultAllocate, &Memory::DefaultFree);
    }


    void Internal::Shutdown()
    {
        for (auto& cache : GCompressionState->m_deflateCompressorCache)
        {
            for (void* impl : cache)
                libdeflate_free_compressor(static_cast<libdeflate_compressor*>(impl));
        }

        for (auto& cache : GCompressionState->m_zstdCompressorCache)
        {
            for (void* impl : cache)
                ZSTD_freeCCtx(static_cast<ZSTD_CCtx*>(impl));
        }

        for (void* impl : GCompressionState->m_deflateDecompressorCache)
            libdeflate_free_decompressor(static_cast<libdeflate_decompressor*>(impl));

        for (void* impl : GCompressionState->m_zstdDecompressorCache)
            ZSTD_freeDCtx(static_cast<ZSTD_DCtx*>(impl));

        GCompressionState->~CompressionState();
        GCompressionState = nullptr;
    }


    Compressor::~Compressor()
    {
        if (!m_impl)
            return;

        std::unique_lock lock{ GCompressionState->m_lock };

        switch (m_method)
        {
        case Method::kDeflate:
            GCompressionState->m_deflateCompressorCache[m_level - 1].push_back(m_impl);
            break;
        case Method::kZstd:
            GCompressionState->m_zstdCompressorCache[m_level - 1].push_back(m_impl);
            break;
        default:
            FE_DebugBreak();
            break;
        }

        m_impl = nullptr;
    }


    void Compressor::Reset()
    {
        Compressor empty;
        std::swap(*this, empty);
    }


    size_t Compressor::GetBounds(const size_t uncompressedSize) const
    {
        switch (m_method)
        {
        case Method::kNone:
            return uncompressedSize;
        case Method::kDeflate:
            return libdeflate_deflate_compress_bound(CastDeflateCompressor(m_impl), uncompressedSize);
        case Method::kZstd:
            return ZSTD_compressBound(uncompressedSize);
        default:
            FE_DebugBreak();
            return 0;
        }
    }


    CompressionResult Compressor::Compress(const void* src, const size_t srcSize, void* dst, const size_t dstSize) const
    {
        switch (m_method)
        {
        default:
        case Method::kInvalid:
            FE_DebugBreak();
            return { ResultCode::kUnknownError, 0 };

        case Method::kNone:
            {
                FE_PROFILER_ZONE_NAMED("Compress None (memcpy)");

                if (dstSize < srcSize)
                    return { ResultCode::kInsufficientSpace, 0 };

                memcpy(dst, src, srcSize);
                return { ResultCode::kSuccess, srcSize };
            }

        case Method::kDeflate:
            {
                FE_PROFILER_ZONE_NAMED("Compress Deflate");

                const size_t compressedSize =
                    libdeflate_deflate_compress(CastDeflateCompressor(m_impl), src, srcSize, dst, dstSize);
                if (compressedSize == 0)
                    return { ResultCode::kInsufficientSpace, 0 };

                return { ResultCode::kSuccess, compressedSize };
            }

        case Method::kZstd:
            {
                FE_PROFILER_ZONE_NAMED("Compress Zstd");

                const size_t compressedSize = ZSTD_compressCCtx(CastZstdCompressor(m_impl), dst, dstSize, src, srcSize, m_level);
                if (ZSTD_isError(compressedSize))
                {
                    if (ZSTD_getErrorCode(compressedSize) == ZSTD_error_dstSize_tooSmall)
                        return { ResultCode::kInsufficientSpace, 0 };

                    return { ResultCode::kUnknownError, 0 };
                }

                return { ResultCode::kSuccess, compressedSize };
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
            return Compressor{ Method::kInvalid, level, nullptr };
        case Method::kNone:
            return Compressor{ method, level, nullptr };
        case Method::kDeflate:
            return Compressor{ method, level, AllocateDeflateCompressor(level) };
        case Method::kZstd:
            return Compressor{ method, level, AllocateZstdCompressor(level) };
        }
    }


    Decompressor::~Decompressor()
    {
        if (!m_impl)
            return;

        std::unique_lock lock{ GCompressionState->m_lock };

        switch (m_method)
        {
        default:
        case Method::kNone: // Should not have gotten past if (!m_impl).
        case Method::kInvalid:
            FE_DebugBreak();
            break;
        case Method::kDeflate:
            GCompressionState->m_deflateDecompressorCache.push_back(m_impl);
            break;
        case Method::kZstd:
            GCompressionState->m_zstdDecompressorCache.push_back(m_impl);
            break;
        }

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
            return { ResultCode::kUnknownError, 0 };

        case Method::kNone:
            {
                FE_PROFILER_ZONE_NAMED("Decompress None (memcpy)");

                if (dstSize < srcSize)
                    return { ResultCode::kInsufficientSpace, 0 };

                memcpy(dst, src, srcSize);
                return { ResultCode::kSuccess, srcSize };
            }

        case Method::kDeflate:
            {
                FE_PROFILER_ZONE_NAMED("Decompress Deflate");

                size_t decompressedSize;
                const libdeflate_result result =
                    libdeflate_deflate_decompress(CastDeflateDecompressor(m_impl), src, srcSize, dst, dstSize, &decompressedSize);

                if (result == LIBDEFLATE_BAD_DATA)
                    return { ResultCode::kInvalidFormat, 0 };

                if (result == LIBDEFLATE_INSUFFICIENT_SPACE)
                    return { ResultCode::kInsufficientSpace, 0 };

                if (result != LIBDEFLATE_SUCCESS)
                    return { ResultCode::kUnknownError, 0 };

                return { ResultCode::kSuccess, decompressedSize };
            }

        case Method::kZstd:
            {
                FE_PROFILER_ZONE_NAMED("Decompress Zstd");

                const size_t decompressedSize = ZSTD_decompressDCtx(CastZstdDecompressor(m_impl), dst, dstSize, src, srcSize);
                if (ZSTD_isError(decompressedSize))
                {
                    if (ZSTD_getErrorCode(decompressedSize) == ZSTD_error_dstSize_tooSmall)
                        return { ResultCode::kInsufficientSpace, 0 };

                    return { ResultCode::kInvalidFormat, 0 };
                }

                return { ResultCode::kSuccess, decompressedSize };
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
            return Decompressor{ Method::kInvalid, nullptr };
        case Method::kNone:
            return Decompressor{ method, nullptr };
        case Method::kDeflate:
            return Decompressor{ method, AllocateDeflateDecompressor() };
        case Method::kZstd:
            return Decompressor{ method, AllocateZstdDecompressor() };
        }
    }
} // namespace FE::Compression
