#include <OsAssets/Images/ImageLoaderImpl.h>

#define STBI_MALLOC(sz) FE::GlobalAllocator<FE::HeapAllocator>::Get().Allocate(sz, 8, FE_SRCPOS())
#define STBI_REALLOC_SIZED(p, oldsz, newsz)                                                                                      \
    FE::GlobalAllocator<FE::HeapAllocator>::Get().Reallocate(p, FE_SRCPOS(), newsz, 8, oldsz)
#define STBI_FREE(p) FE::GlobalAllocator<FE::HeapAllocator>::Get().Deallocate(p, FE_SRCPOS())

#define STBIW_MALLOC(sz) STBI_MALLOC(sz)
#define STBIW_REALLOC_SIZED(p, oldsz, newsz) STBI_REALLOC_SIZED(p, oldsz, newsz)
#define STBIW_FREE(p) STBI_FREE(p)

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace FE::Osmium::Internal
{
    Result<UInt8*, StringSlice> LoadImageFromMemory(const UInt8* data, USize length, Int32& width, Int32& height, Int32& channels)
    {
        UInt8* result = stbi_load_from_memory(data, static_cast<int>(length), &width, &height, &channels, STBI_rgb_alpha);
        if (result)
        {
            return Result<UInt8*, StringSlice>::Ok(result);
        }

        return Result<UInt8*, StringSlice>::Err(stbi_failure_reason());
    }

    void WriteImageToStream(const UInt8* data, Int32 width, Int32 height, IO::IStream* stream)
    {
        stbi_write_png_to_func(
            [](void* context, void* data, int size) {
                static_cast<IO::IStream*>(context)->WriteFromBuffer(data, static_cast<USize>(size));
            },
            stream,
            width,
            height,
            4,
            data,
            4 * width);
    }

    void FreeImageMemory(UInt8* data)
    {
        stbi_image_free(data);
    }
} // namespace FE::Osmium::Internal
