#pragma once
#include <Graphics/Core/GraphicsPipeline.h>
#include <Graphics/Core/ShaderLibrary.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <Graphics/Core/Vulkan/DescriptorAllocator.h>

namespace FE::Graphics::Vulkan
{
    struct ShaderLibrary;


    struct GraphicsPipelineInitContext final
    {
        Core::DefinesStorage m_defines;
        Core::GraphicsPipelineDesc m_desc;
        VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_bindlessSetLayout = VK_NULL_HANDLE;
        ShaderLibrary* m_shaderLibrary = nullptr;
        Logger* m_logger = nullptr;
    };


    struct GraphicsPipeline final : public Core::GraphicsPipeline
    {
        FE_RTTI_Class(GraphicsPipeline, "4524C98F-C971-47EB-A896-6C4EA33CA549");

        GraphicsPipeline(Core::Device* device, DescriptorAllocator* descriptorAllocator);
        ~GraphicsPipeline() override;

        void InitInternal(const GraphicsPipelineInitContext& context);

        void SetCompletionWaitGroup(WaitGroup* waitGroup)
        {
            FE_Assert(m_completionWaitGroup == nullptr, "Already set");
            m_completionWaitGroup = waitGroup;
        }

        [[nodiscard]] VkPipeline GetNative() const
        {
            return m_nativePipeline;
        }

        [[nodiscard]] VkPipelineLayout GetNativeLayout() const
        {
            return m_layout;
        }

    private:
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
        DescriptorAllocator* m_descriptorAllocator = nullptr;

        VkPipelineLayout m_layout = VK_NULL_HANDLE;
        VkPipeline m_nativePipeline = VK_NULL_HANDLE;
    };


    FE_ENABLE_NATIVE_CAST(GraphicsPipeline);
} // namespace FE::Graphics::Vulkan
