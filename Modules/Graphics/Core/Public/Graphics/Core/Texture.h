#pragma once
#include <Graphics/Core/ImageBase.h>
#include <Graphics/Core/Resource.h>

namespace FE::Graphics::Core
{
    enum class TextureUploadStatus : uint32_t
    {
        kEmpty,
        kSucceeded,
        kFailed,
    };


    struct Texture : public Resource
    {
        FE_RTTI_Class(Texture, "816F7FB8-A3C4-4D22-B8F0-A88D8DB78F47");

        virtual const ImageDesc& GetDesc() const = 0;

    private:
        std::atomic<TextureUploadStatus> m_uploadStatus = TextureUploadStatus::kEmpty;
    };
} // namespace FE::Graphics::Core
