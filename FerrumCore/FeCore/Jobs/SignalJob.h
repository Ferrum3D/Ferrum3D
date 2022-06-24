#pragma once
#include <FeCore/Jobs/Job.h>

namespace FE
{
    class SignalJob : public Job
    {
    protected:
        void Execute(const JobExecutionContext&) override {}

    public:
        inline explicit SignalJob()
            : Job(JobPriority::Normal, true)
        {
        }
    };
} // namespace FE
