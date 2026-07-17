#include <Core/Jobs/JobGraph.h>

#include <Core/Base/Base.h>
#include <Core/Jobs/JobNode.h>
#include <Core/Jobs/WaitGroup.h>
#include <festd/vector.h>

namespace FE::Jobs
{
    struct Graph::JobImpl final : public JobNode
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


    struct Graph::JobRecord final
    {
        JobRecord* m_next = nullptr;
        JobImpl* m_job = nullptr;
        Rc<WaitGroup> m_completionWaitGroup;
    };


    Graph::Graph(const Env::Name name, const FiberAffinityMask affinity, const Priority priority)
        : m_name(name)
        , m_affinity(affinity)
        , m_priority(priority)
        , m_allocator(1024)
    {
    }


    Graph::~Graph()
    {
        FE_Assert(!m_isValid || m_jobCount == 0, "Job graph must be either detached or waited to completion before destroying");
        CleanUp();
    }


    Graph::Graph(Graph&& other) noexcept
    {
        swap(*this, other);
    }


    Graph& Graph::operator=(Graph&& other) noexcept
    {
        swap(*this, other);
        return *this;
    }


    Rc<WaitGroup> Graph::Detach()
    {
        struct DetachJob final : public JobNode
        {
            void Execute() override
            {
                if (m_graph.m_completionCallback)
                    m_graph.m_completionCallback(m_graph.m_completionCallbackData);

                m_graph.CleanUp();
                Memory::DefaultDelete(this);
            }

            DetachJob(Graph&& graph)
                : m_graph(std::move(graph))
            {
            }

            Graph m_graph;
        };

        m_isValid = false;
        if (m_jobCount == 0)
            return WaitGroup::Create(0);

        const auto prerequisites = MakeAllWaitGroupsArray();
        if (prerequisites.empty())
        {
            CleanUp();
            return WaitGroup::Create(0);
        }

        const Rc<WaitGroup> waitGroup = WaitGroup::Create();
        auto* job = Memory::DefaultNew<DetachJob>(std::move(*this));
        job->AddPrerequisites(prerequisites);
        job->Dispatch(FiberAffinityMask::kAll, waitGroup.Get(), Priority::kHigh);
        return waitGroup;
    }


    void Graph::Wait()
    {
        const auto prerequisites = MakeAllWaitGroupsArray();
        WaitGroup::WaitAll(prerequisites);

        if (m_completionCallback)
            m_completionCallback(m_completionCallbackData);

        m_isValid = false;
    }


    festd::span<WaitGroup* const> Graph::MakeAllWaitGroupsArray()
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


    void Graph::CleanUp()
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


    Rc<WaitGroup> Graph::DispatchJobImpl(const Env::Name name, const festd::span<WaitGroup* const> prerequisites,
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

        job->Dispatch(m_affinity, waitGroup.Get(), m_priority);
        return waitGroup;
    }


    Rc<WaitGroup> Graph::DispatchJobImpl(const Env::Name name, const festd::span<const Rc<WaitGroup>> prerequisites,
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

        job->Dispatch(m_affinity, waitGroup.Get(), m_priority);
        return waitGroup;
    }


    void swap(Graph& lhs, Graph& rhs) noexcept
    {
        using festd::swap;
        swap(lhs.m_name, rhs.m_name);
        swap(lhs.m_jobCount, rhs.m_jobCount);
        swap(lhs.m_isValid, rhs.m_isValid);
        swap(lhs.m_affinity, rhs.m_affinity);
        swap(lhs.m_priority, rhs.m_priority);
        swap(lhs.m_completionCallback, rhs.m_completionCallback);
        swap(lhs.m_completionCallbackData, rhs.m_completionCallbackData);
        swap(lhs.m_jobRecords, rhs.m_jobRecords);
        swap(lhs.m_allocator, rhs.m_allocator);
    }
} // namespace FE::Jobs
