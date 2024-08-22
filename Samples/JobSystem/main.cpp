#include <FeCore/Console/FeLog.h>
#include <FeCore/DI/Builder.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/EventBus/FrameEvents.h>
#include <FeCore/Jobs/Job.h>
#include <FeCore/Jobs/JobSystem.h>
#include <FeCore/Modules/EnvironmentPrivate.h>
#include <algorithm>
#include <chrono>
#include <random>

using namespace FE;

inline constexpr uint32_t MinJobArrayLength = 64 * 1024;

static void MergeArraysSync(festd::span<const int32_t> lhs, festd::span<const int32_t> rhs, festd::span<int32_t> space)
{
    ZoneScoped;
    const uint32_t wholeSize = lhs.size() + rhs.size();
    FE_ASSERT_MSG(
        wholeSize <= space.size(), "Size of result ({}) was less than size of provided space ({})", wholeSize, space.size());

    uint32_t leftIndex = 0;
    uint32_t rightIndex = 0;
    uint32_t resultIndex = 0;

    while (resultIndex < wholeSize)
    {
        if (leftIndex >= lhs.size())
        {
            space[resultIndex++] = rhs[rightIndex++];
        }
        else if (rightIndex >= rhs.size())
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

static void MergeSortImplAsync(festd::span<int32_t> array, festd::span<int32_t> mergingSpace);

class MergeSortJob : public SmallJob<MergeSortJob>
{
public:
    void DoExecute() const
    {
        ZoneScoped;
        MergeSortImplAsync(Array, Space);
    }

    festd::span<int32_t> Array;
    festd::span<int32_t> Space;

    inline MergeSortJob(festd::span<int32_t> array, festd::span<int32_t> space)
        : Array(array)
        , Space(space)
    {
    }
};

static void MergeSortImplAsync(festd::span<int32_t> array, festd::span<int32_t> mergingSpace)
{
    ZoneScoped;
    ZoneTextF("%d", array.size());
    if (array.size() <= MinJobArrayLength)
    {
        std::sort(array.begin(), array.end());
        return;
    }

    const uint32_t middleIndex = (array.size() + 1) / 2;
    const uint32_t endIndex = array.size();
    const festd::span left = array.subspan(0, middleIndex);
    const festd::span right = array.subspan(middleIndex, endIndex - middleIndex);

    MergeSortJob* leftJob = SmallJob<MergeSortJob>::Create(left, mergingSpace.subspan(0, middleIndex));
    MergeSortJob* rightJob = SmallJob<MergeSortJob>::Create(right, mergingSpace.subspan(middleIndex, endIndex - middleIndex));

    Rc pWaitGroup = WaitGroup::Create();
    leftJob->Schedule(pWaitGroup.Get());
    rightJob->Schedule(pWaitGroup.Get());

    pWaitGroup->Wait();
    MergeArraysSync(festd::span(left), festd::span(right), mergingSpace);
    Memory::Copy(mergingSpace, array);
}

static void ParallelSort(festd::span<int32_t> array)
{
    ZoneScoped;
    festd::vector<int32_t> space;
    space.resize(array.size());
    MergeSortImplAsync(array, festd::span(space));
}

static void AssertSorted(festd::span<int32_t> values)
{
    ZoneScoped;
    uint32_t unsortedCount = 0;
    for (uint32_t i = 0; i < values.size() - 1; ++i)
    {
        if (values[i] > values[i + 1])
        {
            if (unsortedCount < 8)
            {
                FE_LOG_WARNING("Unsorted pair: {{ {}: {}, {}: {} }}", i, values[i], i + 1, values[i + 1]);
            }

            ++unsortedCount;
        }
    }

    if (unsortedCount)
    {
        FE_LOG_ERROR("Total unsorted pairs found: {}", unsortedCount);
    }
}

template<class F>
inline static uint64_t MeasureTime(F&& function)
{
    auto s = std::chrono::high_resolution_clock::now();
    function();
    auto e = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(e - s).count();
}

struct MainJob final : Job
{
    void Execute() override
    {
        ZoneScoped;
        std::random_device device;
        std::mt19937 mt(device());
        std::uniform_int_distribution<int32_t> distribution;
        const size_t length = 2'000'000;
        festd::vector<int32_t> values1, values2;
        values1.reserve(length);
        for (int32_t i = 0; i < length; ++i)
        {
            values1.push_back(distribution(mt));
        }

        values2.resize(length);
        Memory::Copy(festd::span(values1), festd::span(values2));

        auto parallel = MeasureTime([&values1]() {
            ParallelSort(festd::span(values1));
        });
        AssertSorted(values1);

        auto stdSort = MeasureTime([&values2]() {
            std::sort(values2.begin(), values2.end());
        });
        AssertSorted(values2);

        FE_LOG_MESSAGE("Parallel: {}mcs, std::sort: {}mcs, {}x speedup", parallel, stdSort, double(stdSort) / parallel);
        Env::GetServiceProvider()->ResolveRequired<IJobSystem>()->Stop();
    }
};

int main()
{
    Env::CreateEnvironment();
    Rc logger = Rc<Debug::ConsoleLogger>::DefaultNew();
    Rc eventBus = Rc<EventBus<FrameEvents>>::DefaultNew();

    DI::ServiceRegistryBuilder builder{ Env::Internal::GetRootServiceRegistry() };
    builder.Bind<IJobSystem>().To<JobSystem>().InSingletonScope();
    builder.Build();

    MainJob mainJob;
    mainJob.Schedule();
    Env::GetServiceProvider()->Resolve<IJobSystem>().Unwrap()->Start();
}
