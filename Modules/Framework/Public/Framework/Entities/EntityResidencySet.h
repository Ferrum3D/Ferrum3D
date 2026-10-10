#pragma once
#include <Core/IO/AssetManager.h>
#include <Framework/Entities/Base.h>
#include <festd/vector.h>

namespace FE::Framework
{
    //! @brief One logical acquisition with optional service-specific test/implementation state.
    struct EntityAssetRequest final
    {
        //! @brief Underlying asynchronous acquisition.
        IO::AssetRequest m_request;
        //! @brief Opaque token reserved for an overriding service implementation.
        uint64_t m_serviceToken = 0;
    };


    // Services outlive worlds. Overrides support deterministic asset tests without a renderer or I/O.
    //! @brief World-external asynchronous residency service; implementations outlive every consumer.
    struct EntityAssetServices
    {
        virtual ~EntityAssetServices() = default;
        //! @brief Acquire one logical residency reference; the returned request may still be pending.
        virtual EntityAssetRequest Acquire(IO::AssetID id, Rtti::TypeID expectedType);
        //! @brief Check discovery/loading and expected type without waiting; failures are content errors.
        virtual LifecycleResult Poll(const EntityAssetRequest& request, Rtti::TypeID expectedType);
        //! @brief Release one request acquired by these services.
        virtual void Release(EntityAssetRequest& request);
    };


    //! @brief Share acquisitions by asset ID and expected type, keeping incompatible consumers independent.
    struct EntityResidencySet final
    {
        explicit EntityResidencySet(EntityAssetServices& services)
            : m_services(&services)
        {
        }

        ~EntityResidencySet();
        EntityResidencySet(const EntityResidencySet&) = delete;
        EntityResidencySet& operator=(const EntityResidencySet&) = delete;

        //! @brief Add one contributor; the first contributor starts acquisition and null IDs are ignored.
        void Add(IO::AssetID id, Rtti::TypeID expectedType);
        //! @brief Remove one contributor; release acquisition when the last contributor disappears.
        void Remove(IO::AssetID id, Rtti::TypeID expectedType);

        [[nodiscard]] LifecycleResult Poll(IO::AssetID id, Rtti::TypeID expectedType) const;

        //! @brief Return the number of distinct asset/type acquisitions, independently of contributor count.
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


    //! @brief Lifecycle access to the entity and its owning world services.
    struct ComponentContext
    {
        //! @brief Entity currently undergoing the lifecycle operation.
        Entity& m_entity;
        //! @brief Owning world or its incarnation; borrowed where represented as a reference.
        EntityWorld& m_world;
    };


    //! @brief Loading-hook context used to contribute asynchronous hard dependencies.
    struct ComponentLoadingContext : ComponentContext
    {
        // Required requests contribute to the same residency owner as serialized hard dependencies.
        //! @brief Request a hard dependency while Load is executing; polling participates in component readiness.
        void Require(IO::AssetID id, Rtti::TypeID expectedType = Rtti::TypeID::kNull) const;
    };
} // namespace FE::Framework
