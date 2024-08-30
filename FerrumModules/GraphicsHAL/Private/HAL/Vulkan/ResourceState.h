#pragma once
#include <HAL/ResourceState.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    inline VkAccessFlags GetAccessMask(HAL::ResourceState state)
    {
        using HAL::ResourceState;

        static auto Conversions = []() {
            std::array<VkAccessFlags, enum_cast(HAL::ResourceState::kCount)> result{};
            // clang-format off
            result[enum_cast(ResourceState::kUndefined)] = VK_FLAGS_NONE;
            result[enum_cast(ResourceState::kCommon)] = VK_FLAGS_NONE;
            result[enum_cast(ResourceState::kVertexBuffer)] = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
            result[enum_cast(ResourceState::kConstantBuffer)] = VK_ACCESS_UNIFORM_READ_BIT;
            result[enum_cast(ResourceState::kIndexBuffer)] = VK_ACCESS_INDEX_READ_BIT;
            result[enum_cast(ResourceState::kRenderTarget)] = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            result[enum_cast(ResourceState::kUnorderedAccess)] = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            result[enum_cast(ResourceState::kDepthWrite)] = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            result[enum_cast(ResourceState::kDepthRead)] = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            result[enum_cast(ResourceState::kShaderResource)] = VK_ACCESS_SHADER_READ_BIT;
            result[enum_cast(ResourceState::kIndirectArgument)] = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            result[enum_cast(ResourceState::kTransferWrite)] = VK_ACCESS_TRANSFER_WRITE_BIT;
            result[enum_cast(ResourceState::kTransferRead)] = VK_ACCESS_TRANSFER_READ_BIT;
            result[enum_cast(ResourceState::kPresent)] = VK_ACCESS_MEMORY_READ_BIT;
            // clang-format on
            return result;
        }();

        return Conversions[enum_cast(state)];
    }

    inline VkImageLayout VKConvert(HAL::ResourceState state)
    {
        using HAL::ResourceState;

        static auto Conversions = []() {
            std::array<VkImageLayout, enum_cast(HAL::ResourceState::kCount)> result{};
            result[enum_cast(ResourceState::kUndefined)] = VK_IMAGE_LAYOUT_UNDEFINED;
            result[enum_cast(ResourceState::kCommon)] = VK_IMAGE_LAYOUT_GENERAL;
            result[enum_cast(ResourceState::kRenderTarget)] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            result[enum_cast(ResourceState::kUnorderedAccess)] = VK_IMAGE_LAYOUT_GENERAL;
            result[enum_cast(ResourceState::kDepthRead)] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            result[enum_cast(ResourceState::kDepthWrite)] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            result[enum_cast(ResourceState::kShaderResource)] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            result[enum_cast(ResourceState::kTransferRead)] = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            result[enum_cast(ResourceState::kTransferWrite)] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            result[enum_cast(ResourceState::kPresent)] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            return result;
        }();

        return Conversions[enum_cast(state)];
    }
} // namespace FE::Graphics::Vulkan
