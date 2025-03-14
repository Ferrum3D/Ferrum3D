#include <FeCore/Base/Base.h>
#include <FeCore/Base/Platform.h>
#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Strings/Encoding.h>

namespace FE::Platform
{
    namespace
    {
        union CpuId final
        {
            struct
            {
                [[maybe_unused]] uint32_t eax;
                [[maybe_unused]] uint32_t ebx;
                [[maybe_unused]] uint32_t ecx;
                [[maybe_unused]] uint32_t edx;
            };

            int32_t m_regs[4];

            CpuId(const uint32_t funcId, const uint32_t subFuncId)
            {
                Memory::Zero(m_regs, sizeof(m_regs));
                __cpuidex(m_regs, static_cast<int32_t>(funcId), static_cast<int32_t>(subFuncId));
            }
        };


        bool GetCpuIdInfo(CpuInfo& cpuInfo)
        {
            FE_PROFILER_ZONE();

            {
                const CpuId cpuId0(0, 0);

                memcpy(cpuInfo.m_vendorId, &cpuId0.ebx, 4);
                memcpy(cpuInfo.m_vendorId + 4, &cpuId0.edx, 4);
                memcpy(cpuInfo.m_vendorId + 8, &cpuId0.ecx, 4);
                cpuInfo.m_vendorId[12] = 0;
            }
            {
                const CpuId cpuId1(1, 0);

                cpuInfo.m_flags.m_sse41 = (cpuId1.ecx & 0x00080000) != 0;
                cpuInfo.m_flags.m_sse42 = (cpuId1.ecx & 0x00100000) != 0;
                cpuInfo.m_flags.m_avx = (cpuId1.ecx & 0x10000000) != 0;
            }
            {
                const CpuId cpuId7(7, 0);

                cpuInfo.m_flags.m_avx2 = (cpuId7.ebx & 0x00000020) != 0;
            }

            uint32_t cpuNameLength = 0;
            for (uint32_t funcId = 0x80000002; funcId < 0x80000005; ++funcId)
            {
                const CpuId cpuId(funcId, 0);

                memcpy(&cpuInfo.m_cpuName[cpuNameLength], &cpuId, sizeof(CpuId));
                cpuNameLength += 16;
            }

            // CPUs without AVX support don't meet our minimal requirements, no further checks needed.
            // However, we have to get the CPU name for the error message.
            return cpuInfo.MeetsMinimalRequirements();
        }


        void GetCoreInfo(CpuInfo& cpuInfo)
        {
            FE_PROFILER_ZONE();

            DWORD structureLength = 0;
            BOOL result = GetLogicalProcessorInformationEx(RelationAll, nullptr, &structureLength);
            FE_Assert(result == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

            std::byte* buffer = FE_StackAlloc(std::byte, structureLength);

            result = GetLogicalProcessorInformationEx(
                RelationAll, reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer), &structureLength);
            FE_Assert(result);

            for (const std::byte* ptr = buffer; ptr < buffer + structureLength;)
            {
                const auto* info = reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(ptr);
                if (info == nullptr)
                    break;

                switch (info->Relationship)
                {
                case RelationProcessorCore:
                    ++cpuInfo.m_physicalCores;
                    for (WORD groupIndex = 0; groupIndex < info->Processor.GroupCount; ++groupIndex)
                    {
                        cpuInfo.m_logicalCores += Bit::PopCount(info->Processor.GroupMask[groupIndex].Mask);
                    }
                    break;

                case RelationNumaNode:
                    ++cpuInfo.m_numaNodes;
                    break;

                case RelationGroup:
                    cpuInfo.m_processorGroups = info->Group.ActiveGroupCount;
                    break;

                default:
                    break;
                }

                ptr += info->Size;
            }
        }


        char GMessageTempMemory[1024];
    } // namespace


    CpuInfo GetCpuInfo()
    {
        // Static initialization guarantees thread-safety
        const static CpuInfo kCpuInfo = [] {
            CpuInfo info;
            if (GetCpuIdInfo(info))
                GetCoreInfo(info);
            return info;
        }();

        return kCpuInfo;
    }


    bool IsDebuggerPresent()
    {
        return ::IsDebuggerPresent();
    }


    void FatalInitError(const char* message)
    {
        Memory::FixedBlockAllocator allocator{ GMessageTempMemory, sizeof(GMessageTempMemory) };
        const Str::Utf8ToUtf16 messageUtf16{ message, &allocator };
        MessageBoxW(nullptr, messageUtf16.ToWideString(), L"App initialization error", MB_OK | MB_ICONERROR);
        FE_DebugBreak();
    }
} // namespace FE::Platform
