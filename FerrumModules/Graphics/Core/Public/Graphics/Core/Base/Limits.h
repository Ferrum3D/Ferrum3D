#pragma once

namespace FE::Graphics::Core::Limits
{
    namespace Image
    {
        constexpr uint32_t kMaxMipCount = 13;
        constexpr uint32_t kMaxWidth = 1 << kMaxMipCount;
    } // namespace Image


    namespace Pipeline
    {
        constexpr uint32_t kMaxColorAttachments = 8;
        constexpr uint32_t kMaxShaderResourceGroups = 8;
        constexpr uint32_t kMaxRootConstantsByteSize = 128;
        constexpr uint32_t kMaxVertexStreams = 12;
        constexpr uint32_t kMaxStreamChannels = 15;
    } // namespace Pipeline
} // namespace FE::Graphics::Core::Limits
