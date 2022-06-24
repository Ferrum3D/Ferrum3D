#include <OsAssets/Images/StbImage.h>

// TODO: use custom allocator for stb
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace FE::Osmium::Internal
{
    Result<UInt8*, StringSlice> LoadImageFromMemory(const UInt8* data, USize length, Int32& width, Int32& height)
    {
        int channels;
        UInt8* result = stbi_load_from_memory(data, static_cast<int>(length), &width, &height, &channels, STBI_rgb_alpha);
        if (result)
        {
            return Result<UInt8*, StringSlice>::Ok(result);
        }

        return Result<UInt8*, StringSlice>::Err(stbi_failure_reason());
    }

    void FreeImageData(UInt8* data)
    {
        stbi_image_free(data);
    }
} // namespace FE::Osmium::Internal
