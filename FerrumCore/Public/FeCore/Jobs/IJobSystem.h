#pragma once
#include <FeCore/Jobs/Base.h>
#include <FeCore/Memory/RefCount.h>

namespace FE
{
    struct JobScheduleInfo final
    {
        Job* m_job = nullptr;
        JobPriority m_priority = JobPriority::kNormal;
        FiberAffinityMask m_affinityMask = FiberAffinityMask::kAll;
    };


    struct IJobSystem : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(IJobSystem, "F9FB743A-B543-4B64-A36B-B055434DE90B");

        virtual void Schedule(const JobScheduleInfo& info) = 0;
        virtual void Start() = 0;
        virtual void Stop() = 0;

        virtual FiberAffinityMask GetAffinityMaskForCurrentThread() const = 0;
    };
} // namespace FE
