#pragma once
#include <FeCore/Base/CompilerTraits.h>
#include <FeCore/Base/PlatformTraits.h>

namespace FE::Platform
{
    struct CpuInfo final
    {
        uint32_t m_processorGroups = 0;
        uint32_t m_numaNodes = 0;
        uint32_t m_physicalCores = 0;
        uint32_t m_logicalCores = 0;

        char m_vendorId[13] = {};
        char m_cpuName[49] = {};

        struct
        {
            bool m_sse41 : 1;
            bool m_sse42 : 1;
            bool m_avx : 1;
            bool m_avx2 : 1;
        } m_flags = {};

        [[nodiscard]] bool MeetsMinimalRequirements() const
        {
            return m_flags.m_avx;
        }
    };

    CpuInfo GetCpuInfo();

    bool IsDebuggerPresent();

    void FatalInitError(const char* message);
} // namespace FE::Platform
