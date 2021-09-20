#pragma once
#include <FeCore/RTTI/RTTI.h>
#include <GPU/Image/IImage.h>
#include <GPU/Image/ImageFormat.h>
#include <GPU/Image/ImageSubresource.h>

namespace FE::GPU
{
    struct ImageViewDesc
    {
        FE_STRUCT_RTTI(ImageViewDesc, "F018B216-D830-4856-8BF1-E1C082BBBBB1");
        
        ImageSubresourceRange SubresourceRange;
        Format Format = Format::None;
        Shared<IImage> Image;
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
