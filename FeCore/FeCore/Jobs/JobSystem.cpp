#include "JobSystem.h"

namespace FE
{
    void FeJobSystem::Init(const std::vector<FeJobType>& threadFlagArray)
    {
        m_Threads.reserve(threadFlagArray.size());
        for (size_t i = 0; i < threadFlagArray.size(); ++i)
        {
            FeWorkerThreadDesc desc{};
            desc.Flags          = threadFlagArray[i];
            desc.Id             = i;
            desc.Name           = Fmt::Format("Worker thread #{}", i);
            desc.JobSystemCv    = &m_Cv;
            desc.JobSystemMutex = &m_Mutex;
            desc.JobSystemQueue = &m_Queue;
            m_Threads.push_back(std::make_unique<FeWorkerThread>(desc));
            m_Threads.back()->State = FeWorkerThreadState::Working;
        }
    }

    std::unique_ptr<FeJobHandle> FeJobSystem::ScheduleJob(FeJobFunction job, FeJobType type, void* inst)
    {
        auto handle = std::make_unique<FeJobHandle>(job, type, inst);
        m_Queue.Enqueue(handle.get());
        m_Cv.notify_all();
        return handle;
    }
} // namespace FE
