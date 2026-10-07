#pragma once
#include <Framework/Entities/EntityComponentRegistry.h>
#include <Framework/Entities/EntityReference.h>

namespace FE::Framework::Tests
{
    struct SpawnFromLoad final
    {
        static inline const EntityCollection* s_collection = nullptr;
        static inline festd::fixed_vector<MaterializationToken, 16> s_operations;
        FE_RTTI_Reflect("1c7ec0da-7e5b-4ef2-97ef-8a0b846c6017");
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
        FE_RTTI_Reflect("1c7ec0da-7e5b-4ef2-97ef-8a0b846c6015");
        FE_RTTI_Serialize();
    };


    struct Number final
    {
        int32_t m_value = 0;
        FE_RTTI_Reflect("1c7ec0da-7e5b-4ef2-97ef-8a0b846c6001");
        FE_RTTI_Serialize();
    };


    struct Extra final
    {
        int32_t m_value = 0;
        FE_RTTI_Reflect("1c7ec0da-7e5b-4ef2-97ef-8a0b846c6002");
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


        MoveOnly(MoveOnly&& other) noexcept
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
        FE_RTTI_Reflect("1c7ec0da-7e5b-4ef2-97ef-8a0b846c6003");
    };


    struct alignas(256) Aligned final
    {
        uint64_t m_value = 73;
        FE_RTTI_Reflect("1c7ec0da-7e5b-4ef2-97ef-8a0b846c6004");
    };


    struct Huge final
    {
        uint8_t m_data[70000]{};
        FE_RTTI_Reflect("1c7ec0da-7e5b-4ef2-97ef-8a0b846c6005");
    };


    struct ThrowingMove final
    {
        ThrowingMove() = default;
        ThrowingMove(ThrowingMove&&) noexcept(false) {}
        FE_RTTI_Reflect("1c7ec0da-7e5b-4ef2-97ef-8a0b846c6006");
    };


    struct Hook final
    {
        static inline festd::vector<int32_t> s_trace;
        static inline int32_t s_pending = 0;
        static inline int32_t s_failStage = 0;
        int32_t m_label = 0;
        FE_RTTI_Reflect("1c7ec0da-7e5b-4ef2-97ef-8a0b846c6007");
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
        FE_RTTI_Reflect("1c7ec0da-7e5b-4ef2-97ef-8a0b846c6008");
        FE_RTTI_Serialize();
    };


    struct CollectionAssets final
    {
        IO::Link<AssetComponent> m_hard;
        IO::Link<Number, IO::DependencyKind::kSoft> m_soft;
        FE_RTTI_Reflect("1c7ec0da-7e5b-4ef2-97ef-8a0b846c6016");
        FE_RTTI_Serialize();
    };


    struct CustomLoad final
    {
        IO::AssetID m_asset = IO::AssetID::kNull;
        static inline uint32_t s_unloads = 0;
        FE_RTTI_Reflect("1c7ec0da-7e5b-4ef2-97ef-8a0b846c6009");
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
} // namespace FE::Framework::Tests
