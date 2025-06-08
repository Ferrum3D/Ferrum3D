#pragma once
#include <Graphics/Core/Common/FrameGraph/FrameGraph.h>

namespace FE::Graphics::Vulkan
{
    struct FrameGraph final : public Common::FrameGraph
    {
        FE_RTTI_Class(FrameGraph, "585305A0-06EB-4B16-8EF1-26FAACEB6AB8");

        FrameGraph(Core::Device* device, Common::FrameGraphResourcePool* resourcePool);

    private:
        void PrepareExecute() override;
        void FinishExecute() override;
        void FinishPassExecute(const PassData& pass) override;
    };
} // namespace FE::Graphics::Vulkan
