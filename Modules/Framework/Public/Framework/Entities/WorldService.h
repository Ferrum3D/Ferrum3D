#pragma once
#include <Core/RTTI/Any.h>
#include <Framework/Entities/Base.h>

namespace FE::Framework
{
    struct WorldServiceDescriptor final
    {
        FE_RTTI_Reflect("25ABA075-7BEF-4C33-B113-72002B0CB391");
        FE_RTTI_Serialize();

        Rtti::TypeID m_type = Rtti::TypeID::kNull;
        Rtti::Any m_configuration;
        WorldServiceDescriptor() = default;
        WorldServiceDescriptor(Rtti::TypeID type)
            : m_type(type)
        {
        }
    };


    //! @brief World integration object with main-thread lifetime and updates outside traversal collection.
    struct WorldService
    {
        FE_RTTI("BB0BDFFE-F26A-4E94-8A9C-B8588475D196");

        virtual ~WorldService() = default;
        //! Apply authored configuration before Init; unknown configuration types are rejected.
        virtual bool Configure(const Rtti::Any& settings)
        {
            return !settings.HasValue();
        }
        //! Capture configuration only, excluding runtime resources and transient state.
        virtual Rtti::Any CaptureConfiguration() const
        {
            return {};
        }
        virtual void Init(EntityWorld&) {}
        virtual void Shutdown(EntityWorld&) {}
        //! @brief Apply queued requests before structural commit, outside the update schedule.
        virtual void Update(EntityWorld&) {}

    private:
        friend struct EntityWorld;
    };
} // namespace FE::Framework
