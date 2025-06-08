#pragma once
#include <Graphics/Core/ResourceState.h>
#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    inline VkAccessFlags GetAccessMask(Core::ResourceState state)
    {
        using Core::ResourceState;

        static auto Conversions = []() {
            std::array<VkAccessFlags, festd::to_underlying(Core::ResourceState::kCount)> result{};
            // clang-format off
            result[festd::to_underlying(ResourceState::kUndefined)] = VK_FLAGS_NONE;
            result[festd::to_underlying(ResourceState::kCommon)] = VK_FLAGS_NONE;
            result[festd::to_underlying(ResourceState::kVertexBuffer)] = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
            result[festd::to_underlying(ResourceState::kConstantBuffer)] = VK_ACCESS_UNIFORM_READ_BIT;
            result[festd::to_underlying(ResourceState::kIndexBuffer)] = VK_ACCESS_INDEX_READ_BIT;
            result[festd::to_underlying(ResourceState::kRenderTarget)] = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            result[festd::to_underlying(ResourceState::kUnorderedAccess)] = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            result[festd::to_underlying(ResourceState::kDepthWrite)] = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            result[festd::to_underlying(ResourceState::kDepthRead)] = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            result[festd::to_underlying(ResourceState::kShaderResource)] = VK_ACCESS_SHADER_READ_BIT;
            result[festd::to_underlying(ResourceState::kIndirectArgument)] = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            result[festd::to_underlying(ResourceState::kTransferWrite)] = VK_ACCESS_TRANSFER_WRITE_BIT;
            result[festd::to_underlying(ResourceState::kTransferRead)] = VK_ACCESS_TRANSFER_READ_BIT;
            result[festd::to_underlying(ResourceState::kPresent)] = VK_ACCESS_MEMORY_READ_BIT;
            // clang-format on
            return result;
        }();

        return Conversions[festd::to_underlying(state)];
    }


    inline VkImageLayout Translate(Core::ResourceState state)
    {
        using Core::ResourceState;

        static auto Conversions = []() {
            std::array<VkImageLayout, festd::to_underlying(Core::ResourceState::kCount)> result{};
            result[festd::to_underlying(ResourceState::kUndefined)] = VK_IMAGE_LAYOUT_UNDEFINED;
            result[festd::to_underlying(ResourceState::kCommon)] = VK_IMAGE_LAYOUT_GENERAL;
            result[festd::to_underlying(ResourceState::kRenderTarget)] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            result[festd::to_underlying(ResourceState::kUnorderedAccess)] = VK_IMAGE_LAYOUT_GENERAL;
            result[festd::to_underlying(ResourceState::kDepthRead)] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            result[festd::to_underlying(ResourceState::kDepthWrite)] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            result[festd::to_underlying(ResourceState::kShaderResource)] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            result[festd::to_underlying(ResourceState::kTransferRead)] = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            result[festd::to_underlying(ResourceState::kTransferWrite)] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            result[festd::to_underlying(ResourceState::kPresent)] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            return result;
        }();

        return Conversions[festd::to_underlying(state)];
    }
} // namespace FE::Graphics::Vulkan
