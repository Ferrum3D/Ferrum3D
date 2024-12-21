#include <FeCore/Logging/Logger.h>

namespace FE
{
    LogSinkBase::LogSinkBase(Logger* logger)
        : m_pLogger(logger)
    {
        std::lock_guard lk{ logger->m_lock };
        logger->m_sinks.push_back(*this);
    }


    LogSinkBase::~LogSinkBase()
    {
        std::lock_guard lk{ m_pLogger->m_lock };
        m_pLogger->m_sinks.remove(*this);
    }
} // namespace FE
