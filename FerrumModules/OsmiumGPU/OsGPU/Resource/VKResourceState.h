#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Resource/ResourceState.h>

namespace FE::Osmium
{
    inline vk::AccessFlags GetAccessMask(ResourceState state)
    {
        static auto Conversions = []() {
            std::array<vk::AccessFlags, static_cast<size_t>(Format::Count)> result{};
            using Bits = vk::AccessFlagBits;
            // clang-format off
            result[static_cast<USize>(ResourceState::Undefined)]        = static_cast<Bits>(0);
            result[static_cast<USize>(ResourceState::Common)]           = static_cast<Bits>(0);
            result[static_cast<USize>(ResourceState::VertexBuffer)]     = Bits::eVertexAttributeRead;
            result[static_cast<USize>(ResourceState::ConstantBuffer)]   = Bits::eUniformRead;
            result[static_cast<USize>(ResourceState::IndexBuffer)]      = Bits::eIndexRead;
            result[static_cast<USize>(ResourceState::RenderTarget)]     = Bits::eColorAttachmentRead | Bits::eColorAttachmentWrite;
            result[static_cast<USize>(ResourceState::UnorderedAccess)]  = Bits::eShaderRead | Bits::eShaderWrite;
            result[static_cast<USize>(ResourceState::DepthWrite)]       = Bits::eDepthStencilAttachmentRead | Bits::eDepthStencilAttachmentWrite;
            result[static_cast<USize>(ResourceState::DepthRead)]        = Bits::eDepthStencilAttachmentRead;
            result[static_cast<USize>(ResourceState::ShaderResource)]   = Bits::eShaderRead;
            result[static_cast<USize>(ResourceState::IndirectArgument)] = Bits::eIndirectCommandRead;
            result[static_cast<USize>(ResourceState::TransferWrite)]    = Bits::eTransferWrite;
            result[static_cast<USize>(ResourceState::TransferRead)]     = Bits::eTransferRead;
            result[static_cast<USize>(ResourceState::Present)]          = Bits::eMemoryRead;
            // clang-format on
            return result;
        }();

        return Conversions[static_cast<USize>(state)];
    }

    inline vk::ImageLayout VKConvert(ResourceState state)
    {
        static auto Conversions = []() {
            std::array<vk::ImageLayout, static_cast<size_t>(Format::Count)> result{};
            result[static_cast<USize>(ResourceState::Undefined)]       = vk::ImageLayout::eUndefined;
            result[static_cast<USize>(ResourceState::Common)]          = vk::ImageLayout::eGeneral;
            result[static_cast<USize>(ResourceState::RenderTarget)]    = vk::ImageLayout::eColorAttachmentOptimal;
            result[static_cast<USize>(ResourceState::UnorderedAccess)] = vk::ImageLayout::eGeneral;
            result[static_cast<USize>(ResourceState::DepthRead)]       = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
            result[static_cast<USize>(ResourceState::DepthWrite)]      = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            result[static_cast<USize>(ResourceState::ShaderResource)]  = vk::ImageLayout::eShaderReadOnlyOptimal;
            result[static_cast<USize>(ResourceState::TransferRead)]    = vk::ImageLayout::eTransferSrcOptimal;
            result[static_cast<USize>(ResourceState::TransferWrite)]   = vk::ImageLayout::eTransferDstOptimal;
            result[static_cast<USize>(ResourceState::Present)]         = vk::ImageLayout::ePresentSrcKHR;
            return result;
        }();

        return Conversions[static_cast<USize>(state)];
    }
} // namespace FE::Osmium
