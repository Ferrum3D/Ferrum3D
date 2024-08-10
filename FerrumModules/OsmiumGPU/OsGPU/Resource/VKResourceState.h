#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Resource/ResourceState.h>

namespace FE::Osmium
{
    inline VkAccessFlags GetAccessMask(ResourceState state)
    {
        static auto Conversions = []() {
            std::array<VkAccessFlags, static_cast<size_t>(Format::Count)> result{};
            // clang-format off
            result[static_cast<size_t>(ResourceState::Undefined)]        = VK_FLAGS_NONE;
            result[static_cast<size_t>(ResourceState::Common)]           = VK_FLAGS_NONE;
            result[static_cast<size_t>(ResourceState::VertexBuffer)]     = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
            result[static_cast<size_t>(ResourceState::ConstantBuffer)]   = VK_ACCESS_UNIFORM_READ_BIT;
            result[static_cast<size_t>(ResourceState::IndexBuffer)]      = VK_ACCESS_INDEX_READ_BIT;
            result[static_cast<size_t>(ResourceState::RenderTarget)]     = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            result[static_cast<size_t>(ResourceState::UnorderedAccess)]  = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            result[static_cast<size_t>(ResourceState::DepthWrite)]       = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            result[static_cast<size_t>(ResourceState::DepthRead)]        = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            result[static_cast<size_t>(ResourceState::ShaderResource)]   = VK_ACCESS_SHADER_READ_BIT;
            result[static_cast<size_t>(ResourceState::IndirectArgument)] = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            result[static_cast<size_t>(ResourceState::TransferWrite)]    = VK_ACCESS_TRANSFER_WRITE_BIT;
            result[static_cast<size_t>(ResourceState::TransferRead)]     = VK_ACCESS_TRANSFER_READ_BIT;
            result[static_cast<size_t>(ResourceState::Present)]          = VK_ACCESS_MEMORY_READ_BIT;
            // clang-format on
            return result;
        }();

        return Conversions[static_cast<size_t>(state)];
    }

    inline VkImageLayout VKConvert(ResourceState state)
    {
        static auto Conversions = []() {
            std::array<VkImageLayout, static_cast<size_t>(Format::Count)> result{};
            result[static_cast<size_t>(ResourceState::Undefined)]       = VK_IMAGE_LAYOUT_UNDEFINED;
            result[static_cast<size_t>(ResourceState::Common)]          = VK_IMAGE_LAYOUT_GENERAL;
            result[static_cast<size_t>(ResourceState::RenderTarget)]    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            result[static_cast<size_t>(ResourceState::UnorderedAccess)] = VK_IMAGE_LAYOUT_GENERAL;
            result[static_cast<size_t>(ResourceState::DepthRead)]       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            result[static_cast<size_t>(ResourceState::DepthWrite)]      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            result[static_cast<size_t>(ResourceState::ShaderResource)]  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            result[static_cast<size_t>(ResourceState::TransferRead)]    = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            result[static_cast<size_t>(ResourceState::TransferWrite)]   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            result[static_cast<size_t>(ResourceState::Present)]         = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            return result;
        }();

        return Conversions[static_cast<size_t>(state)];
    }
} // namespace FE::Osmium
