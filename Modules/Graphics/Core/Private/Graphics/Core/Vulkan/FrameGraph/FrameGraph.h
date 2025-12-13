#pragma once
#include <Graphics/Core/Common/FrameGraph/FrameGraph.h>
#include <Graphics/Core/Vulkan/Base/BaseTypes.h>

namespace FE::Graphics::Vulkan
{
    struct FrameGraph final : public Common::FrameGraph
    {
        FE_RTTI("585305A0-06EB-4B16-8EF1-26FAACEB6AB8");

        FrameGraph(Core::Device* device, Core::DescriptorManager* descriptorManager, Core::ResourcePool* resourcePool,
                   Core::GraphicsQueue* commandQueue);

        ~FrameGraph() override;

    private:
        void PrepareExecuteInternal() override;
        void FinishExecuteInternal() override;
        void ExecutePassBarriersInternal(PassNode& pass) override;

        Core::GraphicsQueue* m_commandQueue = nullptr;
    };
} // namespace FE::Graphics::Vulkan
