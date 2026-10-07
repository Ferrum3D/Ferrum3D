#pragma once
#include <Core/IO/AssetManager.h>
#include <Framework/Entities/Base.h>
#include <festd/vector.h>

namespace FE::Framework
{
    struct EntityAssetRequest final
    {
        IO::AssetRequest m_request;
        uint64_t m_serviceToken = 0;
    };


    // Services outlive worlds. Overrides support deterministic asset tests without a renderer or I/O.
    struct EntityAssetServices
    {
        virtual ~EntityAssetServices() = default;
        virtual EntityAssetRequest Acquire(IO::AssetID id, Rtti::TypeID expectedType);
        virtual LifecycleResult Poll(const EntityAssetRequest& request, Rtti::TypeID expectedType);
        virtual void Release(EntityAssetRequest& request);
    };


    struct EntityResidencySet final
    {
        explicit EntityResidencySet(EntityAssetServices& services)
            : m_services(&services)
        {
        }

        ~EntityResidencySet();
        EntityResidencySet(const EntityResidencySet&) = delete;
        EntityResidencySet& operator=(const EntityResidencySet&) = delete;

        bool Add(IO::AssetID id, Rtti::TypeID expectedType);
        void Remove(IO::AssetID id, Rtti::TypeID expectedType);

        [[nodiscard]] LifecycleResult Poll(IO::AssetID id, Rtti::TypeID expectedType) const;

        [[nodiscard]] uint32_t GetAcquisitionCount() const
        {
            return m_entries.size();
        }

    private:
        struct Entry
        {
            IO::AssetID m_id;
            Rtti::TypeID m_expectedType;
            EntityAssetRequest m_request;
            uint32_t m_contributors = 1;
        };

        EntityAssetServices* m_services;
        festd::vector<Entry> m_entries;
    };


    struct ComponentContext
    {
        Entity& m_entity;
        EntityWorld& m_world;
        void* m_services = nullptr;
    };


    struct ComponentLoadingContext : ComponentContext
    {
        // Required requests contribute to the same residency owner as serialized hard dependencies.
        bool Require(IO::AssetID id, Rtti::TypeID expectedType = Rtti::TypeID::kNull) const;
    };
} // namespace FE::Framework
