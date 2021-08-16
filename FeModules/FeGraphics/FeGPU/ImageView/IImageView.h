#pragma once
#include <FeCore/RTTI/RTTI.h>
#include <FeGPU/Image/IImage.h>
#include <FeGPU/Image/ImageFormat.h>
#include <FeGPU/Image/ImageSubresource.h>

namespace FE::GPU
{
    struct ImageViewDesc
    {
        ImageSubresourceRange SubresourceRange;
        Format Format = Format::None;
        RefCountPtr<IImage> Image;
        ImageDim Dimension = ImageDim::Image2D;
    };

    class IImageView : public IObject
    {
    public:
        FE_CLASS_RTTI(IImageView, "16C72764-BC1D-4745-A83E-51D021ACA35D");

        ~IImageView() override = default;

        [[nodiscard]] virtual const ImageViewDesc& GetDesc() const = 0;
    };
} // namespace FE::GPU
