#pragma once

namespace FE::Graphics::RHI::Limits
{
    namespace Image
    {
        constexpr uint32_t kMaxMipCount = 13;
        constexpr uint32_t kMaxWidth = 1 << kMaxMipCount;
    } // namespace Image

    namespace Pipeline
    {
        constexpr uint32_t kMaxColorAttachments = 8;
    }
} // namespace FE::Graphics::RHI::Limits
