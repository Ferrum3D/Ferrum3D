#include <FeCore/Assets/AssetStorage.h>
#include <FeCore/Math/Color.h>

namespace FE::Osmium
{
    class ImageAssetStorage : public Assets::AssetStorage
    {
        friend class ImageAssetLoader;

        UInt8* m_Data  = nullptr;
        Int32 m_Width  = 0;
        Int32 m_Height = 0;

        explicit ImageAssetStorage(ImageAssetLoader* loader);

    protected:
        void Delete() override;

    public:
        [[nodiscard]] inline UInt32 PixelValueAt(USize row, USize column) const
        {
            return m_Data[row * m_Width + column];
        }

        [[nodiscard]] inline Color PixelColorAt(USize row, USize column) const
        {
            return Color::FromUInt32(PixelValueAt(row, column));
        }

        [[nodiscard]] inline USize Size() const
        {
            return m_Width * m_Height * 4;
        }

        [[nodiscard]] inline const UInt8* Data() const
        {
            return m_Data;
        }

        [[nodiscard]] inline Int32 Width() const
        {
            return m_Width;
        }

        [[nodiscard]] inline Int32 Height() const
        {
            return m_Height;
        }

        [[nodiscard]] Assets::AssetType GetAssetType() const override;
    };
} // namespace FE::Osmium