#include <Core/Jobs/Jobs.h>
#include <Core/Threading/FiberMutex.h>
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
