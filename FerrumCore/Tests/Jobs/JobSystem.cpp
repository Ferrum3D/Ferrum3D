#include <Core/Jobs/IJobSystem.h>
#include <Core/Jobs/Job.h>
#include <Core/Modules/Environment.h>
#include <Core/Threading/ConditionVariable.h>
#include <Core/Threading/Mutex.h>
#include <Core/Threading/Thread.h>
#include <gtest/gtest.h>

namespace FE::Tests
{
    struct NotifyJob final : public Job
    {
        Threading::Mutex* m_mutex = nullptr;
        Threading::ConditionVariable* m_cv = nullptr;
        bool* m_done = nullptr;

        void Execute() override
        {
            m_mutex->lock();
            *m_done = true;
            m_mutex->unlock();
            m_cv->NotifyOne();
        }
    };

    TEST(JobSystem, WakesFromIdleOnNewJob)
    {
        IJobSystem* jobSystem = Env::GetServiceProvider()->ResolveRequired<IJobSystem>();

        Threading::Mutex mutex;
        Threading::ConditionVariable cv;
        bool done = false;

        NotifyJob job;
        job.m_mutex = &mutex;
        job.m_cv = &cv;
        job.m_done = &done;

        JobScheduleInfo info;
        info.m_job = &job;
        info.m_priority = JobPriority::kNormal;
        info.m_affinityMask = FiberAffinityMask::kAll;
        Threading::Thread schedulerThread("JobSystemTestScheduler", [jobSystem, &cv, &mutex, &done, info]() mutable {
            Threading::Sleep(50);
            jobSystem->Schedule(info);

            std::unique_lock lock(mutex);
            const bool signaled = cv.WaitFor(lock, 2000, [&done] {
                return done;
            });

            EXPECT_TRUE(signaled);
            jobSystem->Stop();
        });

        jobSystem->Start();
        schedulerThread.Join();
    }
} // namespace FE::Tests
