#include <Core/Jobs/Job.h>
#include <Core/Jobs/TaskGraph.h>
#include <festd/vector.h>

namespace FE
{
    struct TaskGraph::JobImpl final : public Job
    {
        void Execute() override
        {
            FE_PROFILER_ZONE_TEXT("%.*s/%.*s", m_graphName.size(), m_graphName.c_str(), m_taskName.size(), m_taskName.c_str());
            m_function(m_data);
            this->~JobImpl();
        }

        void* m_data = nullptr;
        TaskFunction m_function = nullptr;
        Env::Name m_graphName;
        Env::Name m_taskName;
    };


    struct TaskGraph::JobRecord final
    {
        JobRecord* m_next = nullptr;
        JobImpl* m_job = nullptr;
        Rc<WaitGroup> m_completionWaitGroup;
    };


    TaskGraph::TaskGraph(const Env::Name name, IJobSystem* jobSystem, const FiberAffinityMask affinity,
                         const JobPriority priority)
        : m_name(name)
        , m_jobSystem(jobSystem)
        , m_affinity(affinity)
        , m_priority(priority)
        , m_allocator(1024)
    {
    }


    TaskGraph::~TaskGraph()
    {
        FE_Assert(!m_isValid, "TaskGraph must be either detached or waited to completion before destroying");
        CleanUp();
    }


    TaskGraph::TaskGraph(TaskGraph&& other) noexcept
    {
        swap(*this, other);
    }


    TaskGraph& TaskGraph::operator=(TaskGraph&& other) noexcept
    {
        swap(*this, other);
        return *this;
    }


    Rc<WaitGroup> TaskGraph::Detach()
    {
        struct DetachJob final : public Job
        {
            void Execute() override
            {
                m_graph.CleanUp();
                Memory::DefaultDelete(this);
            }

            DetachJob(TaskGraph&& graph)
                : m_graph(std::move(graph))
            {
            }

            TaskGraph m_graph;
        };

        m_isValid = false;

        const auto prerequisites = MakeAllWaitGroupsArray();
        if (prerequisites.empty())
        {
            CleanUp();
            return WaitGroup::Create(0);
        }

        const Rc<WaitGroup> waitGroup = WaitGroup::Create();
        auto* job = Memory::DefaultNew<DetachJob>(std::move(*this));
        job->AddPrerequisites(prerequisites);
        job->Schedule(m_jobSystem, FiberAffinityMask::kAll, waitGroup.Get(), JobPriority::kHigh);
        return waitGroup;
    }


    void TaskGraph::Wait()
    {
        const auto prerequisites = MakeAllWaitGroupsArray();
        WaitGroup::WaitAll(prerequisites);
        m_isValid = false;
    }


    festd::span<WaitGroup* const> TaskGraph::MakeAllWaitGroupsArray()
    {
        if (m_jobCount == 0)
            return {};

        WaitGroup** array = Memory::AllocateArray<WaitGroup*>(&m_allocator, m_jobCount);

        uint32_t waitGroupIndex = 0;
        JobRecord* record = m_jobRecords;
        while (record)
        {
            array[waitGroupIndex++] = record->m_completionWaitGroup.Get();
            record = record->m_next;
        }

        return { array, m_jobCount };
    }


    void TaskGraph::CleanUp()
    {
        JobRecord* record = m_jobRecords;
        while (record)
        {
            auto* next = record->m_next;
            record->~JobRecord();
            record = next;
        }

        m_jobRecords = nullptr;
        m_allocator = {};
    }


    Rc<WaitGroup> TaskGraph::ScheduleTaskImpl(const Env::Name name, const festd::span<WaitGroup* const> prerequisites,
                                              const TaskFunction taskFunction, void* data)
    {
        FE_Assert(m_isValid);
        ++m_jobCount;

        const Rc<WaitGroup> waitGroup = WaitGroup::Create();

        auto* job = Memory::New<JobImpl>(&m_allocator);
        job->m_data = data;
        job->m_function = taskFunction;
        job->m_graphName = m_name;
        job->m_taskName = name;
        job->AddPrerequisites(prerequisites);

        auto* record = Memory::New<JobRecord>(&m_allocator);
        record->m_next = m_jobRecords;
        record->m_job = job;
        record->m_completionWaitGroup = waitGroup;
        m_jobRecords = record;

        job->Schedule(m_jobSystem, m_affinity, waitGroup.Get(), m_priority);
        return waitGroup;
    }


    void swap(TaskGraph& lhs, TaskGraph& rhs) noexcept
    {
        using festd::swap;
        swap(lhs.m_name, rhs.m_name);
        swap(lhs.m_jobCount, rhs.m_jobCount);
        swap(lhs.m_isValid, rhs.m_isValid);
        swap(lhs.m_jobSystem, rhs.m_jobSystem);
        swap(lhs.m_affinity, rhs.m_affinity);
        swap(lhs.m_priority, rhs.m_priority);
        swap(lhs.m_jobRecords, rhs.m_jobRecords);
        swap(lhs.m_allocator, rhs.m_allocator);
    }
} // namespace FE
