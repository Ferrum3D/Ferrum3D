#include <FeCore/Logging/Logger.h>

namespace FE
{
    LogSinkBase::LogSinkBase(Logger* logger)
        : m_pLogger(logger)
    {
        std::lock_guard lk{ logger->m_Lock };
        logger->m_Sinks.push_back(*this);
    }


    LogSinkBase::~LogSinkBase()
    {
        std::lock_guard lk{ m_pLogger->m_Lock };
        m_pLogger->m_Sinks.remove(*this);
    }
} // namespace FE
