#include <FeCore/Logging/Trace.h>
#include <Graphics/Assets/ImageLoaderImpl.h>

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

#include <bc7enc.h>

namespace FE::Graphics
{
    festd::expected<RawImage, StringSlice> RawImage::LoadFromMemory(festd::span<const uint8_t> data)
    {
        RawImage result;
        int32_t w, h, channels;
        result.m_data = stbi_load_from_memory(data.data(), static_cast<int32_t>(data.size()), &w, &h, &channels, STBI_rgb_alpha);
        if (result.m_data)
        {
            if (w >= UINT16_MAX || h >= UINT16_MAX)
                return festd::unexpected("Image too big");

            result.m_width = static_cast<uint16_t>(w);
            result.m_height = static_cast<uint16_t>(h);
            result.m_channelCount = static_cast<uint32_t>(channels);
            return result;
        }

        return festd::unexpected(stbi_failure_reason());
    }


    void RawImage::Free(RawImage& image)
    {
        stbi_image_free(image.m_data);
        memset(&image, 0, sizeof(RawImage));
    }
} // namespace FE::Graphics
