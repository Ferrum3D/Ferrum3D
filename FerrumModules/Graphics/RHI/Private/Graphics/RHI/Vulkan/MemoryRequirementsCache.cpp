#include <Graphics/RHI/Vulkan/Buffer.h>
#include <Graphics/RHI/Vulkan/Image.h>
#include <Graphics/RHI/Vulkan/MemoryRequirementsCache.h>

namespace FE::Graphics::Vulkan
{
    VkMemoryRequirements MemoryRequirementsCache::GetRenderTargetMemoryRequirements()
    {
        if (m_renderTargetMemoryRequirements.size == 0)
        {
            const Rc image = m_serviceProvider->ResolveRequired<RHI::Image>();

            const RHI::ImageBindFlags bindFlags = RHI::ImageBindFlags::kColor | RHI::ImageBindFlags::kShaderRead;
            const RHI::ImageDesc imageDesc = RHI::ImageDesc::Img2D(bindFlags, 1, 1, RHI::Format::kR8G8B8A8_UNORM);
            const RHI::ResultCode result = image->Init("MemoryRequirementsCache Temp", imageDesc);
            FE_Assert(result == RHI::ResultCode::kSuccess);
            m_renderTargetMemoryRequirements = ImplCast(image.Get())->GetMemoryRequirements();
        }

        return m_renderTargetMemoryRequirements;
    }


    VkMemoryRequirements MemoryRequirementsCache::GetBufferMemoryRequirements()
    {
        if (m_bufferMemoryRequirements.size == 0)
        {
            Rc pBuffer = m_serviceProvider->ResolveRequired<RHI::Buffer>();

            const RHI::BindFlags bindFlags = RHI::BindFlags::kConstantBuffer | RHI::BindFlags::kShaderResource;
            const RHI::BufferDesc bufferDesc{ 1, bindFlags };
            const RHI::ResultCode result = pBuffer->Init("MemoryRequirementsCache Temp", bufferDesc);
            FE_Assert(result == RHI::ResultCode::kSuccess);
            m_bufferMemoryRequirements = ImplCast(pBuffer.Get())->GetMemoryRequirements();
        }

        return m_bufferMemoryRequirements;
    }


    VkMemoryRequirements MemoryRequirementsCache::GetImageMemoryRequirements(const RHI::ImageDesc& desc)
    {
        VkMemoryRequirements requirements{};
        const size_t hash = eastl::hash<RHI::ImageDesc>{}(desc);
        if (m_imageMemoryRequirementsByDesc.TryGetValue(hash, requirements))
            return requirements;

        Rc image = m_serviceProvider->ResolveRequired<RHI::Image>();

        const RHI::ResultCode result = image->Init("MemoryRequirementsCache Temp", desc);
        FE_Assert(result == RHI::ResultCode::kSuccess);
        requirements = ImplCast(image.Get())->GetMemoryRequirements();
        m_imageMemoryRequirementsByDesc.Emplace(hash, requirements);
        return requirements;
    }


    VkMemoryRequirements MemoryRequirementsCache::GetImageMemoryRequirements()
    {
        if (m_imageMemoryRequirements.size == 0)
        {
            Rc image = m_serviceProvider->ResolveRequired<RHI::Image>();

            const RHI::ImageBindFlags bindFlags = RHI::ImageBindFlags::kUnorderedAccess | RHI::ImageBindFlags::kShaderRead;
            const RHI::ImageDesc imageDesc = RHI::ImageDesc::Img2D(bindFlags, 1, 1, RHI::Format::kR8G8B8A8_UNORM);
            const RHI::ResultCode result = image->Init("MemoryRequirementsCache Temp", imageDesc);
            FE_Assert(result == RHI::ResultCode::kSuccess);
            m_renderTargetMemoryRequirements = ImplCast(image.Get())->GetMemoryRequirements();
        }

        return m_imageMemoryRequirements;
    }
} // namespace FE::Graphics::Vulkan
