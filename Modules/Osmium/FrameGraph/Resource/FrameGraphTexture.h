#pragma once
#include <FrameGraph/Resource/FrameGraphResourceBase.h>
#include <GPU/Image/IImage.h>

namespace FE::FG
{
    enum class FrameGraphInitialState
    {
        Clear,
        Undefined
    };

    struct FrameGraphTextureDesc
    {
        GPU::Size TextureSize     = { 0, 0, 0 };
        GPU::Format TextureFormat = GPU::Format::None;
        GPU::ImageDim Dimension   = GPU::ImageDim::Image2D;

        UInt16 ArraySize     = 1;
        UInt32 MipLevelCount = 1;
        UInt32 SampleCount   = 1;

        FrameGraphInitialState InitialState = FrameGraphInitialState::Clear;

        FE_STRUCT_RTTI(FrameGraphTextureDesc, "A829AA9C-3B24-4245-9B06-F9CAFCE8CE97");
    };

    class FrameGraphTexture final : public FrameGraphResourceBase
    {
        FrameGraphTextureDesc m_Desc;
        Shared<GPU::IImage> m_ActualResource;

    public:
        FE_CLASS_RTTI(FrameGraphTexture, "F97870EE-6791-4023-A639-8D6C09BB1980");

        ~FrameGraphTexture() override = default;
        explicit FrameGraphTexture(const FrameGraphTextureDesc& desc);

        void Realize(GPU::IDevice* device) override;
        void Dispose() override;
    };
} // namespace FE::FG
