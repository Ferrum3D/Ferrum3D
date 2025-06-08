#pragma once
#include <Graphics/Core/Common/FrameGraph/FrameGraph.h>
#include <Graphics/Core/Vulkan/Base/BaseTypes.h>

namespace FE::Graphics::Vulkan
{
    struct BindlessManager;

    struct FrameGraph final : public Common::FrameGraph
    {
        FE_RTTI_Class(FrameGraph, "585305A0-06EB-4B16-8EF1-26FAACEB6AB8");

        FrameGraph(Core::Device* device, Common::FrameGraphResourcePool* resourcePool, BindlessManager* bindlessManager);

        Core::ImageSRVDescriptor GetSRV(Core::Texture* texture, Core::ImageSubresource subresource) override;
        Core::SamplerDescriptor GetSampler(Core::SamplerState sampler) override;

    private:
        void PrepareExecute() override;
        void FinishExecute() override;
        void FinishPassExecute(const PassData& pass) override;

        BindlessManager* m_bindlessManager = nullptr;
    };
} // namespace FE::Graphics::Vulkan
