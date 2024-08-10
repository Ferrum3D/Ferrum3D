#include <OsAssets/Images/ImageLoaderImpl.h>

#define STBI_MALLOC(sz) FE::Memory::DefaultAllocate(sz)
#define STBI_REALLOC_SIZED(p, oldsz, newsz) FE::Memory::DefaultReallocate(p, newsz)
#define STBI_FREE(p) FE::Memory::DefaultFree(p)

#define STBIW_MALLOC(sz) STBI_MALLOC(sz)
#define STBIW_REALLOC_SIZED(p, oldsz, newsz) STBI_REALLOC_SIZED(p, oldsz, newsz)
#define STBIW_FREE(p) STBI_FREE(p)

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace FE::Osmium::Internal
{
    Result<uint8_t*, StringSlice> LoadImageFromMemory(const uint8_t* data, size_t length, int32_t& width, int32_t& height, int32_t& channels)
    {
        uint8_t* result = stbi_load_from_memory(data, static_cast<int>(length), &width, &height, &channels, STBI_rgb_alpha);
        if (result)
        {
            return result;
        }

        return Err(StringSlice(stbi_failure_reason()));
    }

    void WriteImageToStream(const uint8_t* data, int32_t width, int32_t height, IO::IStream* stream)
    {
        stbi_write_png_to_func(
            [](void* context, void* data, int size) {
                static_cast<IO::IStream*>(context)->WriteFromBuffer(data, static_cast<size_t>(size));
            },
            stream,
            width,
            height,
            4,
            data,
            4 * width);
    }

    void FreeImageMemory(uint8_t* data)
    {
        stbi_image_free(data);
    }
} // namespace FE::Osmium::Internal
