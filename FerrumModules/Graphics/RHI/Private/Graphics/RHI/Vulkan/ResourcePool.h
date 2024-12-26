#pragma once
#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/RHI/ResourcePool.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    struct ResourcePool final : public RHI::ResourcePool
    {
        ResourcePool(RHI::Device* device);
        ~ResourcePool() override;

        FE_RTTI_Class(ResourcePool, "32B0D24A-62EB-47D5-869D-897424FD3439");

        festd::expected<RHI::Image*, RHI::ResultCode> CreateImage(Env::Name name, const RHI::ImageDesc& desc) override;
        festd::expected<RHI::Buffer*, RHI::ResultCode> CreateBuffer(Env::Name name, const RHI::BufferDesc& desc) override;

        VmaAllocator m_allocator;
        Memory::PoolAllocator m_imagePool;
        Memory::PoolAllocator m_bufferPool;
    };
} // namespace FE::Graphics::Vulkan
