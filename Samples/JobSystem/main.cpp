#include <FeCore/Console/FeLog.h>
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/Jobs/Job.h>
#include <FeCore/Jobs/JobScheduler.h>
#include <FeCore/Jobs/SignalJob.h>
#include <algorithm>
#include <chrono>
#include <random>

using FE::Int32;
using FE::UInt64;
using FE::USize;

inline constexpr USize MinJobArrayLength = 64 * 1024;

void MergeArraysSync(const FE::ArraySlice<Int32>& lhs, const FE::ArraySlice<Int32>& rhs, FE::ArraySliceMut<Int32> space)
{
    const USize wholeSize = lhs.Length() + rhs.Length();
    FE_ASSERT_MSG(
        wholeSize <= space.Length(), "Size of result ({}) was less than size of provided space ({})", wholeSize, space.Length());

    USize leftIndex   = 0;
    USize rightIndex  = 0;
    USize resultIndex = 0;

    while (resultIndex < wholeSize)
    {
        if (leftIndex >= lhs.Length())
        {
            space[resultIndex++] = rhs[rightIndex++];
        }
        else if (rightIndex >= rhs.Length())
        {
            space[resultIndex++] = lhs[leftIndex++];
        }
        else if (lhs[leftIndex] < rhs[rightIndex])
        {
            space[resultIndex++] = lhs[leftIndex++];
        }
        else
        {
            space[resultIndex++] = rhs[rightIndex++];
        }
    }
}

void MergeSortImplAsync(FE::ArraySliceMut<Int32> array, FE::ArraySliceMut<Int32> mergingSpace);

class MergeSortJob : public FE::Job
{
protected:
    void Execute(const FE::JobExecutionContext&) override
    {
        MergeSortImplAsync(Array, Space);
    }

public:
    FE::ArraySliceMut<Int32> Array;
    FE::ArraySliceMut<Int32> Space;

    inline MergeSortJob(const FE::ArraySliceMut<int>& array, const FE::ArraySliceMut<int>& space)
        : Array(array)
        , Space(space)
    {
    }
};

void MergeSortImplAsync(FE::ArraySliceMut<Int32> array, FE::ArraySliceMut<Int32> mergingSpace)
{
    if (array.Length() <= MinJobArrayLength)
    {
        std::sort(array.begin(), array.end());
        return;
    }

    const auto middleIndex = (array.Length() + 1) / 2;
    const auto endIndex    = array.Length();
    auto left              = array(0, middleIndex);
    auto right             = array(middleIndex, endIndex);

    MergeSortJob leftJob(left, mergingSpace(0, middleIndex));
    MergeSortJob rightJob(right, mergingSpace(middleIndex, endIndex));
    FE::SignalJob signalJob;
    signalJob.AttachParent(&leftJob);
    signalJob.AttachParent(&rightJob);
    leftJob.Schedule();
    rightJob.Schedule();

    signalJob.Complete();
    MergeArraysSync(FE::ArraySlice(left), FE::ArraySlice(right), mergingSpace);
    mergingSpace.CopyDataTo(array);
}

void ParallelSort(FE::ArraySliceMut<Int32> array)
{
    FE::Vector<Int32> space;
    space.resize(array.Length());
    MergeSortImplAsync(array, FE::ArraySliceMut(space));
    ;
}

void AssertSorted(const FE::ArraySlice<int>& values)
{
    for (USize i = 0; i < values.Length() - 1; ++i)
    {
        if (values[i] > values[i + 1])
        {
            FE_LOG_WARNING("Unsorted pair: {{ {}: {}, {}: {} }}", i, values[i], i + 1, values[i + 1]);
        }
    }
}

template<class F>
inline UInt64 MeasureTime(F&& function)
{
    auto s = std::chrono::high_resolution_clock::now();
    function();
    auto e = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(e - s).count();
}

int main()
{
    FE::Env::CreateEnvironment();
    FE::GlobalAllocator<FE::HeapAllocator>::Init(FE::HeapAllocatorDesc{});
    {
        [[maybe_unused]] std::random_device device;
        [[maybe_unused]] std::mt19937 mt(device());
        [[maybe_unused]] std::uniform_int_distribution<Int32> distribution;
        auto logger       = FE::MakeShared<FE::Debug::ConsoleLogger>();
        auto eventBus     = FE::MakeShared<FE::EventBus<FE::FrameEvents>>();
        auto jobScheduler = FE::MakeShared<FE::JobScheduler>(std::thread::hardware_concurrency() - 1);

        const USize length = 2'000'000;
        FE::Vector<Int32> values1, values2;
        values1.reserve(length);
        for (Int32 i = 0; i < length; ++i)
        {
            values1.push_back(distribution(mt));
        }
        values2.resize(length);
        FE::ArraySlice(values1).CopyDataTo(FE::ArraySliceMut(values2));

        auto parallel = MeasureTime([&values1]() {
            ParallelSort(FE::ArraySliceMut(values1));
        });
        AssertSorted(values1);

        auto stdSort = MeasureTime([&values2]() {
            std::sort(values2.begin(), values2.end());
        });
        AssertSorted(values2);

        FE_LOG_MESSAGE("Parallel: {}mcs, std::sort: {}msc, {}x speedup", parallel, stdSort, double(stdSort) / parallel);
    }
    FE::GlobalAllocator<FE::HeapAllocator>::Destroy();
}
