#pragma once
#include <Graphics/Core/Device.h>

namespace FE::Graphics
{
    struct VisibilitySupport final
    {
        bool m_supported = false;
        Core::Format m_hdrFormat = Core::Format::kUndefined;
        uint32_t m_hdrBytesPerPixel = 0;
        uint32_t m_logicalWaveWidth = 32;
        uint32_t m_waveLaneCount = 0;

        static VisibilitySupport Query(const Core::Device& device)
        {
            VisibilitySupport result;
            const Core::FormatCapabilities visibility = device.GetFormatCapabilities(Core::Format::kR32G32_UINT);
            const Core::FormatCapabilities surface = device.GetFormatCapabilities(Core::Format::kR32_UINT);
            const Core::FormatCapabilities depth = device.GetFormatCapabilities(Core::Format::kD32_SFLOAT);
            if (!visibility.m_colorAttachment || !visibility.m_sampled || !surface.m_storage || !surface.m_sampled
                || !surface.m_storageWriteWithoutFormat || !depth.m_depthAttachment || !depth.m_sampled)
            {
                return result;
            }

            const Core::FormatCapabilities packedHdr = device.GetFormatCapabilities(Core::Format::kB10G11R11_UFLOAT);
            if (packedHdr.m_storage && packedHdr.m_sampled && packedHdr.m_storageWriteWithoutFormat)
            {
                result.m_hdrFormat = Core::Format::kB10G11R11_UFLOAT;
                result.m_hdrBytesPerPixel = 4;
            }
            else
            {
                const Core::FormatCapabilities wideHdr = device.GetFormatCapabilities(Core::Format::kR16G16B16A16_SFLOAT);
                if (!wideHdr.m_storage || !wideHdr.m_sampled || !wideHdr.m_storageWriteWithoutFormat)
                    return result;

                result.m_hdrFormat = Core::Format::kR16G16B16A16_SFLOAT;
                result.m_hdrBytesPerPixel = 8;
            }

            const Core::ComputeCapabilities compute = device.GetComputeCapabilities();
            if (compute.m_waveLaneCountControl)
            {
                const uint32_t width = compute.m_waveLaneCount == 64 ? 64 : 32;
                if (width >= compute.m_minWaveLaneCount && width <= compute.m_maxWaveLaneCount)
                {
                    result.m_logicalWaveWidth = width;
                    result.m_waveLaneCount = width;
                }
            }

            result.m_supported = true;
            return result;
        }
    };
} // namespace FE::Graphics
