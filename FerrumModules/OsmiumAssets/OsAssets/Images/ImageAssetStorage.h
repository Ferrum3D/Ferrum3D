#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <FeCore/Math/Color.h>

namespace FE::Osmium
{
    class ImageAssetStorage : public Assets::AssetStorage
    {
        friend class ImageAssetLoader;

        uint8_t* m_Data = nullptr;
        int32_t m_Width = 0;
        int32_t m_Height = 0;

    protected:
        void Delete() override;

    public:
        explicit ImageAssetStorage(ImageAssetLoader* loader);

        FE_RTTI_Class(ImageAssetStorage, "0C9406E1-44CF-49E1-8B3B-D9E116E10C91");

        [[nodiscard]] inline uint32_t PixelValueAt(size_t row, size_t column) const
        {
            return reinterpret_cast<uint32_t*>(m_Data)[row * m_Width + column];
        }

        [[nodiscard]] inline Color PixelColorAt(size_t row, size_t column) const
        {
            return Color::FromUInt32(PixelValueAt(row, column));
        }

        [[nodiscard]] inline size_t Size() const
        {
            return m_Width * m_Height * 4;
        }

        [[nodiscard]] inline const uint8_t* Data() const
        {
            return m_Data;
        }

        [[nodiscard]] inline int32_t Width() const
        {
            return m_Width;
        }

        [[nodiscard]] inline int32_t Height() const
        {
            return m_Height;
        }
    };
} // namespace FE::Osmium
