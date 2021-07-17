#pragma once
#include <FeCore/Utils/CoreUtils.h>
#include <FeCore/Math/FeVector3.h>

namespace FE
{
    // clang-format off
	FE_ENUM(FeImageFileFormat)
	{
		None,
		PNG,
		JPEG,
		TIFF
	};
    // clang-format on

    struct TextureLoadDesc
    {
        std::string Name{};
        std::string Path{};
        FeImageFileFormat Format;
        bool IsSRGB       = false;
        bool GenerateMips = true;
    };

    class IFeTexture
    {
    public:
        /**
		 * @brief Get vector { Width, Height, Depth } of the texture
		 * @return Dimentions of the texture
		*/
        virtual uint3 GetSize() = 0;
    };
} // namespace FE
