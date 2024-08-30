#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE
{
    enum class JobPriority : uint16_t
    {
        kLow,
        kNormal,
        kHigh,
        kCount,
    };

    class Job;


    class IJobSystem : public Memory::RefCountedObjectBase
    {
        friend class WaitGroup;

        virtual void* AllocateSmallBlock(size_t byteSize) = 0;
        virtual void FreeSmallBlock(void* ptr, size_t byteSize) = 0;

    public:
        FE_RTTI_Class(IJobSystem, "F9FB743A-B543-4B64-A36B-B055434DE90B");

        virtual void AddJob(Job* pJob, JobPriority priority = JobPriority::kNormal) = 0;
        virtual void Start() = 0;
        virtual void Stop() = 0;
    };
} // namespace FE
