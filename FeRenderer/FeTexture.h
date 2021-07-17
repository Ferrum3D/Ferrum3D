#include "FeGraphicsDevice.h"
#include "IFeTexture.h"

namespace FE
{
    class FeTexture : public IFeTexture
    {
        DL::RefCntAutoPtr<Diligent::ITexture> m_Handle;

    public:
        FeTexture(DL::RefCntAutoPtr<Diligent::ITexture> texture);

        DL::ITexture* GetTexture();
        DL::ITextureView* GetDefaultView(Diligent::TEXTURE_VIEW_TYPE type);

        virtual uint3 GetSize() override;
    };
} // namespace FE
