#pragma once
#include <HAL/Vulkan/Common/Config.h>
#include <HAL/ResourceState.h>

namespace FE::Graphics::Vulkan
{
    inline VkAccessFlags GetAccessMask(HAL::ResourceState state)
    {
        using HAL::ResourceState;

        static auto Conversions = []() {
            std::array<VkAccessFlags, enum_cast(HAL::Format::Count)> result{};
            // clang-format off
            result[enum_cast(ResourceState::Undefined)] = VK_FLAGS_NONE;
            result[enum_cast(ResourceState::Common)] = VK_FLAGS_NONE;
            result[enum_cast(ResourceState::VertexBuffer)] = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
            result[enum_cast(ResourceState::ConstantBuffer)] = VK_ACCESS_UNIFORM_READ_BIT;
            result[enum_cast(ResourceState::IndexBuffer)] = VK_ACCESS_INDEX_READ_BIT;
            result[enum_cast(ResourceState::RenderTarget)] = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            result[enum_cast(ResourceState::UnorderedAccess)] = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            result[enum_cast(ResourceState::DepthWrite)] = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            result[enum_cast(ResourceState::DepthRead)] = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            result[enum_cast(ResourceState::ShaderResource)] = VK_ACCESS_SHADER_READ_BIT;
            result[enum_cast(ResourceState::IndirectArgument)] = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            result[enum_cast(ResourceState::TransferWrite)] = VK_ACCESS_TRANSFER_WRITE_BIT;
            result[enum_cast(ResourceState::TransferRead)] = VK_ACCESS_TRANSFER_READ_BIT;
            result[enum_cast(ResourceState::Present)] = VK_ACCESS_MEMORY_READ_BIT;
            // clang-format on
            return result;
        }();

        return Conversions[enum_cast(state)];
    }

    inline VkImageLayout VKConvert(HAL::ResourceState state)
    {
        using HAL::ResourceState;

        static auto Conversions = []() {
            std::array<VkImageLayout, enum_cast(HAL::Format::Count)> result{};
            result[enum_cast(ResourceState::Undefined)] = VK_IMAGE_LAYOUT_UNDEFINED;
            result[enum_cast(ResourceState::Common)] = VK_IMAGE_LAYOUT_GENERAL;
            result[enum_cast(ResourceState::RenderTarget)] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            result[enum_cast(ResourceState::UnorderedAccess)] = VK_IMAGE_LAYOUT_GENERAL;
            result[enum_cast(ResourceState::DepthRead)] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            result[enum_cast(ResourceState::DepthWrite)] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            result[enum_cast(ResourceState::ShaderResource)] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            result[enum_cast(ResourceState::TransferRead)] = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            result[enum_cast(ResourceState::TransferWrite)] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            result[enum_cast(ResourceState::Present)] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            return result;
        }();

        return Conversions[enum_cast(state)];
    }
} // namespace FE::Graphics::Vulkan
