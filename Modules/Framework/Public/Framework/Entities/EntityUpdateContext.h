#pragma once
#include <FeCore/Jobs/Base.h>
#include <Framework/Entities/Base.h>

namespace FE::Framework
{
    struct EntityLoadingContext final
    {
        IJobSystem* m_jobSystem = nullptr;

        void Initialize(IJobSystem* jobSystem)
        {
            m_jobSystem = jobSystem;
        }
    };


    struct EntityUpdateContext final
    {
        IJobSystem* m_jobSystem = nullptr;

        void Initialize(IJobSystem* jobSystem)
        {
            m_jobSystem = jobSystem;
        }
    };
} // namespace FE::Framework
