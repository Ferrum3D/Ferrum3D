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

        template<class Service, class Configuration>
            requires std::same_as<Configuration, typename Service::Configuration>
        static WorldServiceDescriptor Create(const Configuration& configuration)
        {
            WorldServiceDescriptor descriptor(Rtti::GetTypeID<Service>());
            descriptor.m_configuration.Emplace<Configuration>(configuration);
            return descriptor;
        }
    };


    //! @brief World integration object with main-thread lifetime and updates outside traversal collection.
    struct WorldService
    {
        FE_RTTI("BB0BDFFE-F26A-4E94-8A9C-B8588475D196");

        virtual ~WorldService() = default;
        virtual void Init(EntityWorld&) {}
        virtual void Shutdown(EntityWorld&) {}
        //! @brief Apply queued requests before structural commit, outside the update schedule.
        virtual void Update(EntityWorld&) {}

    private:
        // FindService resolves registered services through the private RTTI cast.
        friend struct EntityWorld;
    };
} // namespace FE::Framework
