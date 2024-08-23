#include <HAL/Vulkan/Buffer.h>
#include <HAL/Vulkan/Image.h>
#include <HAL/Vulkan/MemoryRequirementsCache.h>

namespace FE::Graphics::Vulkan
{
    VkMemoryRequirements MemoryRequirementsCache::GetRenderTargetMemoryRequirements()
    {
        if (m_RenderTargetMemoryRequirements.size == 0)
        {
            Rc pImage = m_pServiceProvider->ResolveRequired<HAL::Image>();

            const HAL::ImageBindFlags bindFlags = HAL::ImageBindFlags::Color | HAL::ImageBindFlags::ShaderRead;
            const HAL::ImageDesc imageDesc = HAL::ImageDesc::Img2D(bindFlags, 1, 1, HAL::Format::R8G8B8A8_UNorm);
            const HAL::ResultCode result = pImage->Init("MemoryRequirementsCache Temp", imageDesc);
            FE_ASSERT(result == HAL::ResultCode::Success);
            m_RenderTargetMemoryRequirements = ImplCast(pImage.Get())->GetMemoryRequirements();
        }

        return m_RenderTargetMemoryRequirements;
    }


    VkMemoryRequirements MemoryRequirementsCache::GetBufferMemoryRequirements()
    {
        if (m_BufferMemoryRequirements.size == 0)
        {
            Rc pBuffer = m_pServiceProvider->ResolveRequired<HAL::Buffer>();

            const HAL::BindFlags bindFlags = HAL::BindFlags::ConstantBuffer | HAL::BindFlags::ShaderResource;
            const HAL::BufferDesc bufferDesc{ 1, bindFlags };
            const HAL::ResultCode result = pBuffer->Init("MemoryRequirementsCache Temp", bufferDesc);
            FE_ASSERT(result == HAL::ResultCode::Success);
            m_BufferMemoryRequirements = ImplCast(pBuffer.Get())->GetMemoryRequirements();
        }

        return m_BufferMemoryRequirements;
    }


    VkMemoryRequirements MemoryRequirementsCache::GetImageMemoryRequirements(const HAL::ImageDesc& desc)
    {
        VkMemoryRequirements requirements{};
        const size_t hash = eastl::hash<HAL::ImageDesc>{}(desc);
        if (m_ImageMemoryRequirementsByDesc.TryGetValue(hash, requirements))
            return requirements;

        Rc pImage = m_pServiceProvider->ResolveRequired<HAL::Image>();

        const HAL::ResultCode result = pImage->Init("MemoryRequirementsCache Temp", desc);
        FE_ASSERT(result == HAL::ResultCode::Success);
        requirements = ImplCast(pImage.Get())->GetMemoryRequirements();
        m_ImageMemoryRequirementsByDesc.Emplace(hash, requirements);
        return requirements;
    }


    VkMemoryRequirements MemoryRequirementsCache::GetImageMemoryRequirements()
    {
        if (m_ImageMemoryRequirements.size == 0)
        {
            Rc pImage = m_pServiceProvider->ResolveRequired<HAL::Image>();

            const HAL::ImageBindFlags bindFlags = HAL::ImageBindFlags::UnorderedAccess | HAL::ImageBindFlags::ShaderRead;
            const HAL::ImageDesc imageDesc = HAL::ImageDesc::Img2D(bindFlags, 1, 1, HAL::Format::R8G8B8A8_UNorm);
            const HAL::ResultCode result = pImage->Init("MemoryRequirementsCache Temp", imageDesc);
            FE_ASSERT(result == HAL::ResultCode::Success);
            m_RenderTargetMemoryRequirements = ImplCast(pImage.Get())->GetMemoryRequirements();
        }

        return m_ImageMemoryRequirements;
    }
} // namespace FE::Graphics::Vulkan
