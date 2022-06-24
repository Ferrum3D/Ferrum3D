#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE
{
    class Job;

    class IJobScheduler : public IObject
    {
    public:
        FE_CLASS_RTTI(IJobScheduler, "F9FB743A-B543-4B64-A36B-B055434DE90B");

        [[nodiscard]] virtual UInt32 GetWorkerCount() const = 0;
        [[nodiscard]] virtual UInt32 GetWorkerID() const    = 0;

        [[nodiscard]] virtual void* OneFrameAllocate(USize size, USize alignment) = 0;

        virtual void ScheduleJob(Job* job) = 0;
        virtual void SuspendUntilComplete(Job* job) = 0;
    };
} // namespace FE
