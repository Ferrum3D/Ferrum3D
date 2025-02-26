#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE
{
    enum class JobPriority : uint16_t
    {
        kLow = 0,
        kNormal = 1,
        kHigh = 2,

        // TODO
        kBackground = kLow,

        kCount = kHigh + 1,
    };

    struct Job;


    struct IJobSystem : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(IJobSystem, "F9FB743A-B543-4B64-A36B-B055434DE90B");

        virtual void AddJob(Job* pJob, JobPriority priority = JobPriority::kNormal) = 0;
        virtual void Start() = 0;
        virtual void Stop() = 0;

    private:
        friend struct WaitGroup;

        virtual std::pmr::memory_resource* GetWaitGroupAllocator() = 0;
    };
} // namespace FE
