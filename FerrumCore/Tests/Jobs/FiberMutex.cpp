#include <Core/Jobs/Jobs.h>
#include <Core/Threading/FiberMutex.h>
#include <Core/Threading/Thread.h>
#include <gtest/gtest.h>

using namespace FE;

TEST(FiberMutex, QueuedFibersReleaseTheWorkerAndReceiveFifoOwnership)
{
    Threading::FiberMutex mutex;
    EXPECT_TRUE(mutex.try_lock());
    EXPECT_FALSE(mutex.try_lock());
    const Rc<WaitGroup> done = WaitGroup::Create(3);
    uint32_t order[3]{};
    uint32_t index = 0;
    for (uint32_t i = 0; i < 3; ++i)
    {
        const Rc<WaitGroup> entered = WaitGroup::Create();
        Jobs::DispatchMainThread(
            [&, entered, i] {
                entered->Signal();
                std::lock_guard guard(mutex);
                order[index++] = i;
            },
            done.Get());
        entered->Wait();
        EXPECT_EQ(mutex.GetWaiterCount(), i + 1);
    }
    bool otherWork = false;
    const Rc<WaitGroup> probe = WaitGroup::Create();
    Jobs::DispatchMainThread(
        [&] {
            otherWork = true;
        },
        probe.Get());
    probe->Wait();
    EXPECT_TRUE(otherWork);
    EXPECT_EQ(index, 0);
    mutex.unlock();
    done->Wait();
    EXPECT_EQ(index, 3);
    for (uint32_t i = 0; i < 3; ++i)
        EXPECT_EQ(order[i], i);
    EXPECT_EQ(mutex.GetWaiterCount(), 0);
    EXPECT_TRUE(mutex.try_lock());
    mutex.unlock();
}


TEST(FiberMutex, RepeatedContendedHandoffsDoNotLoseWakeups)
{
    Threading::FiberMutex mutex;
    uint32_t value = 0;
    constexpr uint32_t kJobs = 64;
    const Rc<WaitGroup> done = WaitGroup::Create(kJobs);
    mutex.lock();
    for (uint32_t i = 0; i < kJobs; ++i)
    {
        Jobs::DispatchForeground(
            [&] {
                for (uint32_t iteration = 0; iteration < 32; ++iteration)
                {
                    std::lock_guard guard(mutex);
                    ++value;
                }
            },
            done.Get());
    }
    mutex.unlock();
    done->Wait();
    EXPECT_EQ(value, kJobs * 32);
}


TEST(FiberMutex, ExternalUnlockRacingWithSuspensionPublishesOwnership)
{
    constexpr uint32_t kIterations = 256;
    Threading::FiberMutex mutex;
    std::atomic<uint32_t> requested = 0;
    uint32_t value = 0;
    Threading::Thread signaler("FiberMutexSignaler", [&] {
        for (uint32_t iteration = 1; iteration <= kIterations; ++iteration)
        {
            while (requested.load(std::memory_order_acquire) != iteration)
                _mm_pause();

            while (mutex.GetWaiterCount() == 0)
                _mm_pause();

            value = iteration;
            mutex.unlock();
        }
    });

    for (uint32_t iteration = 1; iteration <= kIterations; ++iteration)
    {
        mutex.lock();
        requested.store(iteration, std::memory_order_release);

        // The signaler releases the initial ownership as soon as this waiter is published.
        mutex.lock();
        EXPECT_EQ(value, iteration);
        mutex.unlock();
    }

    signaler.Join();
    EXPECT_EQ(mutex.GetWaiterCount(), 0);
}


TEST(FiberMutex, SuspendedOwnersAndWaitersPreserveAffinityAndProtectedWrites)
{
    constexpr uint32_t kJobs = 12;
    constexpr uint32_t kIterations = 16;
    const Jobs::FiberAffinityMask affinities[] = { Jobs::FiberAffinityMask::kMainThread,
                                                   Jobs::FiberAffinityMask::kAllForeground,
                                                   Jobs::FiberAffinityMask::kAllBackground,
                                                   Jobs::FiberAffinityMask::kAll };
    Threading::FiberMutex mutex;
    const Rc<WaitGroup> done = WaitGroup::Create(kJobs);
    uint32_t value = 0;
    mutex.lock();
    for (uint32_t index = 0; index < kJobs; ++index)
    {
        const Jobs::FiberAffinityMask affinity = affinities[index % 4];
        Jobs::Dispatch(
            affinity,
            festd::span<WaitGroup* const>{},
            Jobs::Priority::kNormal,
            [&, affinity] {
                for (uint32_t iteration = 0; iteration < kIterations; ++iteration)
                {
                    std::lock_guard guard(mutex);
                    const uint32_t previousValue = value;
                    const Rc<WaitGroup> resume = WaitGroup::Create();
                    Jobs::DispatchMainThread([] {}, resume.Get());
                    resume->Wait();

                    const uint64_t workerBit = festd::to_underlying(Jobs::GetAffinityMaskForCurrentThread());
                    EXPECT_NE(workerBit & festd::to_underlying(affinity), 0);
                    EXPECT_EQ(value, previousValue);
                    value = previousValue + 1;
                }
            },
            done.Get());
    }

    mutex.unlock();
    done->Wait();
    EXPECT_EQ(value, kJobs * kIterations);
    EXPECT_EQ(mutex.GetWaiterCount(), 0);
}


TEST(ConcurrentQueue, PushFrontIntoEmptyQueuePreservesFollowingEnqueues)
{
    ConcurrentQueue queue;
    ConcurrentQueue::Node first{}, second{}, third{};
    queue.PushFront(&first);
    queue.Enqueue(&second);
    queue.PushFront(&third);
    EXPECT_EQ(queue.TryDequeue(), &third);
    EXPECT_EQ(queue.TryDequeue(), &first);
    EXPECT_EQ(queue.TryDequeue(), &second);
    EXPECT_EQ(queue.TryDequeue(), nullptr);
    queue.PushFront(&second);
    queue.Enqueue(&first);
    EXPECT_EQ(queue.TryDequeue(), &second);
    EXPECT_EQ(queue.TryDequeue(), &first);
    EXPECT_TRUE(queue.Empty());
}
