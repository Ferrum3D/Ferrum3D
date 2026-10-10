#pragma once
#include <Framework/Entities/EntityComponentRegistry.h>
#include <Framework/Entities/EntityReference.h>
#include <Framework/Entities/EntityWorld.h>

namespace FE::Framework::Tests
{
    struct SpawnFromLoad final
    {
        static inline const EntityCollection* s_collection = nullptr;
        static inline festd::fixed_vector<MaterializationToken, 16> s_operations;
        FE_RTTI_Reflect("BDB95E44-26C7-4523-9107-DD8BB6EDEDB1");
        FE_RTTI_Serialize();

        LifecycleResult Load(ComponentLoadingContext& context)
        {
            if (s_collection)
            {
                for (uint32_t index = 0; index < 16; ++index)
                    s_operations.push_back(context.m_world.SpawnCollection(context.m_entity.GetRegistry(), *s_collection));
            }

            return LifecycleResult::kSucceeded;
        }

        void Unload(ComponentLoadingContext&) {}
    };


    struct ReferenceComponent final
    {
        EntityReference m_internal;
        EntityReference m_external;

        FE_RTTI_Reflect("6FCC0FE8-057A-4C57-A415-6B03421C2588");
        FE_RTTI_Serialize();
    };


    struct Number final
    {
        int32_t m_value = 0;

        FE_RTTI_Reflect("1C86B496-EDD6-4611-B6A9-F521923DF111");
        FE_RTTI_Serialize();
    };


    struct Extra final
    {
        int32_t m_value = 0;

        FE_RTTI_Reflect("1572734F-D76F-4718-ADB4-85E270CCC7B3");
    };


    struct MoveOnly final
    {
        static inline int32_t s_live = 0;
        static inline int32_t s_moves = 0;
        int32_t m_value = 0;

        MoveOnly() noexcept
        {
            ++s_live;
        }

        explicit MoveOnly(int32_t value) noexcept
            : m_value(value)
        {
            ++s_live;
        }

        MoveOnly(MoveOnly&& other)
            : m_value(std::exchange(other.m_value, -1))
        {
            ++s_live;
            ++s_moves;
        }

        MoveOnly(const MoveOnly&) = delete;

        ~MoveOnly()
        {
            --s_live;
        }

        FE_RTTI_Reflect("025CFBC0-301B-40E6-98B0-34AF15C15725");
    };


    struct alignas(256) Aligned final
    {
        uint64_t m_value = 73;
        FE_RTTI_Reflect("24034252-6B50-43E2-8279-9DD7683CB451");
    };


    struct Huge final
    {
        uint8_t m_data[70000]{};
        FE_RTTI_Reflect("6E4583CA-A7EF-4936-BCCB-EE93539D12F4");
    };


    struct Hook final
    {
        static inline festd::vector<int32_t> s_trace;
        static inline int32_t s_pending = 0;
        static inline int32_t s_failStage = 0;
        int32_t m_label = 0;

        FE_RTTI_Reflect("3BE4D796-1602-4658-B205-94B15F190036");

        LifecycleResult Load(ComponentLoadingContext&)
        {
            s_trace.push_back(m_label * 10 + 1);
            if (s_failStage == 1)
                return LifecycleResult::kFailed;
            return s_pending ? LifecycleResult::kPending : LifecycleResult::kSucceeded;
        }

        void Unload(ComponentLoadingContext&)
        {
            s_trace.push_back(m_label * 10 + 6);
        }

        LifecycleResult Init(ComponentContext&)
        {
            s_trace.push_back(m_label * 10 + 2);
            return s_failStage == 2 ? LifecycleResult::kFailed : LifecycleResult::kSucceeded;
        }

        void Shutdown(ComponentContext&)
        {
            s_trace.push_back(m_label * 10 + 5);
        }

        LifecycleResult Activate(ComponentContext&)
        {
            s_trace.push_back(m_label * 10 + 3);
            return s_failStage == 3 ? LifecycleResult::kFailed : LifecycleResult::kSucceeded;
        }

        void Deactivate(ComponentContext&)
        {
            s_trace.push_back(m_label * 10 + 4);
        }
    };


    struct AssetComponent final
    {
        IO::Link<Number> m_hard;
        IO::Link<Number, IO::DependencyKind::kSoft> m_soft;
        FE_RTTI_Reflect("710FAE8D-B519-4A61-BA50-BC5E9CBC4386");
        FE_RTTI_Serialize();
    };


    struct CollectionAssets final
    {
        IO::Link<AssetComponent> m_hard;
        IO::Link<Number, IO::DependencyKind::kSoft> m_soft;
        FE_RTTI_Reflect("356E316C-AB56-4BC4-AF86-BDEE64923EC8");
        FE_RTTI_Serialize();
    };


    struct CustomLoad final
    {
        IO::AssetID m_asset = IO::AssetID::kNull;
        static inline uint32_t s_unloads = 0;
        FE_RTTI_Reflect("75315CD9-C745-4BF2-B622-0EE5DB84CA10");

        LifecycleResult Load(ComponentLoadingContext& context)
        {
            context.Require(m_asset, Rtti::GetTypeID<Number>());
            return LifecycleResult::kSucceeded;
        }

        void Unload(ComponentLoadingContext&)
        {
            ++s_unloads;
        }
    };


    struct SnapshotSystem : WorldSystem
    {
        FE_RTTI("99CE9EB1-35B8-4CC5-8586-66EECF15FE12");
        static inline int32_t s_live = 0;

        SnapshotSystem()
        {
            ++s_live;
        }


        ~SnapshotSystem() override
        {
            --s_live;
        }


        void Init(EntityWorld& world) override
        {
            world.Components().Register<Number>();
            world.Components().Register<ReferenceComponent>();
            world.Components().Register<Extra>({}, { true });
            world.Components().AddRuntimeCompanion<Number, Extra>();
        }


        void Update(EntityUpdateContext&) override {}
    };


    struct SystemPrefix
    {
        FE_RTTI("A4D961B7-D93F-4397-AD84-FCEEF5FB6B70");
        virtual ~SystemPrefix() = default;
        uint64_t m_padding = 0;
    };


    struct OffsetSnapshotSystem final
        : SystemPrefix
        , SnapshotSystem
    {
        FE_RTTI("2FE311B5-3C69-4284-A1B8-91916200907B");
    };


    struct SnapshotService : WorldService
    {
        FE_RTTI("DDB82620-0C08-49E6-9AC7-434A1611187B");
        static inline int32_t s_live = 0;
        static inline bool s_shutdownSawEmptyWorld = false;
        uint32_t m_updates = 0;

        SnapshotService()
        {
            ++s_live;
        }


        ~SnapshotService() override
        {
            --s_live;
        }


        void Shutdown(EntityWorld& world) override
        {
            s_shutdownSawEmptyWorld = world.GetEntityCount() == 0;
        }


        void Update(EntityWorld&) override
        {
            ++m_updates;
        }
    };


    struct OffsetSnapshotService final
        : SystemPrefix
        , SnapshotService
    {
        FE_RTTI("5CD205A4-CCD2-478E-89D0-7192339A8DC8");
    };


    struct NonDefaultSystem final : WorldSystem
    {
        FE_RTTI("F1D91905-8D2B-4632-855E-ACBE821FD00A");
        explicit NonDefaultSystem(uint32_t) {}
        void Update(EntityUpdateContext&) override {}
    };
} // namespace FE::Framework::Tests
