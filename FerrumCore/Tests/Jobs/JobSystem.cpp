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
        IJobSystem* m_jobSystem = nullptr;
        Threading::ConditionVariable* m_cv = nullptr;
        std::atomic<uint32_t>* m_executionCount = nullptr;
        std::atomic<uint32_t>* m_orderCounter = nullptr;
        std::atomic<uint32_t>* m_executionOrder = nullptr;
        std::atomic<uint64_t>* m_observedAffinity = nullptr;

        void Execute() override
        {
            if (m_observedAffinity)
            {
                const FiberAffinityMask affinity = m_jobSystem->GetAffinityMaskForCurrentThread();
                m_observedAffinity->store(festd::to_underlying(affinity), std::memory_order_relaxed);
            }

            if (m_executionOrder)
                m_executionOrder->store(m_orderCounter->fetch_add(1, std::memory_order_relaxed), std::memory_order_relaxed);

            m_executionCount->fetch_add(1, std::memory_order_release);
            m_cv->NotifyAll();
        }
    };


    struct WaitAllJob final : public Job
    {
        WaitGroup* m_waitGroups[2] = {};
        Threading::ConditionVariable* m_cv = nullptr;
        std::atomic<bool>* m_entered = nullptr;
        std::atomic<bool>* m_resumed = nullptr;

        void Execute() override
        {
            m_entered->store(true, std::memory_order_release);
            m_cv->NotifyAll();
            WaitGroup::WaitAll(festd::span<WaitGroup* const>{ m_waitGroups });
            m_resumed->store(true, std::memory_order_release);
            m_cv->NotifyAll();
        }
    };


    TEST(JobSystem, PrerequisitesAndCombinedWaits)
    {
        constexpr uint32_t kRaceJobCount = 64;

        IJobSystem* jobSystem = Env::GetServiceProvider()->ResolveRequired<IJobSystem>();
        Threading::Mutex mutex;
        Threading::ConditionVariable cv;

        std::atomic<uint32_t> immediateExecutionCount = 0;
        NotifyJob immediateJob;
        immediateJob.m_jobSystem = jobSystem;
        immediateJob.m_cv = &cv;
        immediateJob.m_executionCount = &immediateExecutionCount;
        const Rc immediateCompletion = WaitGroup::Create();

        std::atomic<uint32_t> deferredExecutionCount = 0;
        NotifyJob deferredJob;
        deferredJob.m_jobSystem = jobSystem;
        deferredJob.m_cv = &cv;
        deferredJob.m_executionCount = &deferredExecutionCount;
        const Rc deferredPrerequisiteA = WaitGroup::Create();
        const Rc deferredPrerequisiteB = WaitGroup::Create();
        deferredJob.AddPrerequisites({ deferredPrerequisiteA, deferredPrerequisiteB });

        std::atomic<uint32_t> presignaledExecutionCount = 0;
        NotifyJob presignaledJob;
        presignaledJob.m_jobSystem = jobSystem;
        presignaledJob.m_cv = &cv;
        presignaledJob.m_executionCount = &presignaledExecutionCount;
        const Rc presignaledPrerequisite = WaitGroup::Create();
        presignaledJob.AddPrerequisite(presignaledPrerequisite);
        presignaledPrerequisite->Signal();

        std::atomic<uint32_t> priorityExecutionCount = 0;
        std::atomic<uint32_t> orderCounter = 0;
        std::atomic<uint32_t> lowExecutionOrder = Constants::kMaxU32;
        std::atomic<uint32_t> highExecutionOrder = Constants::kMaxU32;
        std::atomic<uint64_t> lowAffinity = 0;
        std::atomic<uint64_t> highAffinity = 0;
        NotifyJob lowPriorityJob;
        lowPriorityJob.m_jobSystem = jobSystem;
        lowPriorityJob.m_cv = &cv;
        lowPriorityJob.m_executionCount = &priorityExecutionCount;
        lowPriorityJob.m_orderCounter = &orderCounter;
        lowPriorityJob.m_executionOrder = &lowExecutionOrder;
        lowPriorityJob.m_observedAffinity = &lowAffinity;
        NotifyJob highPriorityJob;
        highPriorityJob.m_jobSystem = jobSystem;
        highPriorityJob.m_cv = &cv;
        highPriorityJob.m_executionCount = &priorityExecutionCount;
        highPriorityJob.m_orderCounter = &orderCounter;
        highPriorityJob.m_executionOrder = &highExecutionOrder;
        highPriorityJob.m_observedAffinity = &highAffinity;
        const Rc priorityPrerequisite = WaitGroup::Create();
        lowPriorityJob.AddPrerequisite(priorityPrerequisite);
        highPriorityJob.AddPrerequisite(priorityPrerequisite);

        std::atomic<bool> waitAllEntered = false;
        std::atomic<bool> waitAllResumed = false;
        const Rc waitAllGroupA = WaitGroup::Create();
        const Rc waitAllGroupB = WaitGroup::Create();
        WaitAllJob waitAllJob;
        waitAllJob.m_waitGroups[0] = waitAllGroupA.Get();
        waitAllJob.m_waitGroups[1] = waitAllGroupB.Get();
        waitAllJob.m_cv = &cv;
        waitAllJob.m_entered = &waitAllEntered;
        waitAllJob.m_resumed = &waitAllResumed;

        std::atomic<uint32_t> raceExecutionCount = 0;
        NotifyJob raceJobs[kRaceJobCount];
        Rc<WaitGroup> racePrerequisites[kRaceJobCount];
        for (uint32_t index = 0; index < kRaceJobCount; ++index)
        {
            raceJobs[index].m_jobSystem = jobSystem;
            raceJobs[index].m_cv = &cv;
            raceJobs[index].m_executionCount = &raceExecutionCount;
            racePrerequisites[index] = WaitGroup::Create();
            raceJobs[index].AddPrerequisite(racePrerequisites[index]);
        }

        const auto waitFor = [&mutex, &cv](const auto& predicate) {
            std::unique_lock lock(mutex);
            return cv.WaitFor(lock, 2000, predicate);
        };

        Threading::Thread schedulerThread("JobSystemTestScheduler", [&] {
            Threading::Sleep(50);

            immediateJob.ScheduleForeground(jobSystem, immediateCompletion.Get());
            EXPECT_TRUE(waitFor([&] {
                return immediateCompletion->IsSignaled();
            }));
            EXPECT_EQ(immediateExecutionCount.load(std::memory_order_acquire), 1);

            deferredJob.ScheduleForeground(jobSystem);
            Threading::Sleep(20);
            EXPECT_EQ(deferredExecutionCount.load(std::memory_order_acquire), 0);
            deferredPrerequisiteA->Signal();
            Threading::Sleep(20);
            EXPECT_EQ(deferredExecutionCount.load(std::memory_order_acquire), 0);
            deferredPrerequisiteB->Signal();
            EXPECT_TRUE(waitFor([&] {
                return deferredExecutionCount.load(std::memory_order_acquire) == 1;
            }));

            EXPECT_EQ(presignaledExecutionCount.load(std::memory_order_acquire), 0);
            presignaledJob.ScheduleForeground(jobSystem);
            EXPECT_TRUE(waitFor([&] {
                return presignaledExecutionCount.load(std::memory_order_acquire) == 1;
            }));

            lowPriorityJob.Schedule(jobSystem, FiberAffinityMask::kMainThread, nullptr, JobPriority::kLow);
            highPriorityJob.Schedule(jobSystem, FiberAffinityMask::kMainThread, nullptr, JobPriority::kHigh);
            priorityPrerequisite->Signal();
            EXPECT_TRUE(waitFor([&] {
                return priorityExecutionCount.load(std::memory_order_acquire) == 2;
            }));
            EXPECT_EQ(highExecutionOrder.load(std::memory_order_relaxed), 0);
            EXPECT_EQ(lowExecutionOrder.load(std::memory_order_relaxed), 1);
            EXPECT_EQ(highAffinity.load(std::memory_order_relaxed), festd::to_underlying(FiberAffinityMask::kMainThread));
            EXPECT_EQ(lowAffinity.load(std::memory_order_relaxed), festd::to_underlying(FiberAffinityMask::kMainThread));

            waitAllJob.ScheduleForeground(jobSystem);
            EXPECT_TRUE(waitFor([&] {
                return waitAllEntered.load(std::memory_order_acquire);
            }));
            waitAllGroupA->Signal();
            Threading::Sleep(20);
            EXPECT_FALSE(waitAllResumed.load(std::memory_order_acquire));
            waitAllGroupB->Signal();
            EXPECT_TRUE(waitFor([&] {
                return waitAllResumed.load(std::memory_order_acquire);
            }));

            WaitGroup::WaitAll(festd::span<WaitGroup* const>{});
            const Rc alreadySignaled = WaitGroup::Create();
            alreadySignaled->Signal();
            WaitGroup* alreadySignaledGroups[] = { alreadySignaled.Get() };
            WaitGroup::WaitAll(festd::span<WaitGroup* const>{ alreadySignaledGroups });

            Threading::Thread signalThread("JobSystemPrerequisiteSignaler", [&] {
                for (const Rc<WaitGroup>& prerequisite : racePrerequisites)
                    prerequisite->Signal();
            });
            for (NotifyJob& job : raceJobs)
                job.ScheduleForeground(jobSystem);
            signalThread.Join();
            EXPECT_TRUE(waitFor([&] {
                return raceExecutionCount.load(std::memory_order_acquire) == kRaceJobCount;
            }));

            jobSystem->Stop();
        });

        jobSystem->Start();
        schedulerThread.Join();
    }
} // namespace FE::Tests
