#pragma once
#include <Framework/Entities/EntityResources.h>
#include <Framework/Entities/EntityWorld.h>

namespace FE::Framework
{
    //! @brief Per-frame admission limit shared by every update epoch of one rendered frame.
    struct FrameQuota final
    {
        uint64_t m_frame = UINT64_MAX;
        uint32_t m_remaining = 0;

        //! @brief Reset the remaining count when a new frame starts and borrow it for consumption.
        uint32_t& Refresh(const uint64_t frame, const uint32_t limit)
        {
            if (m_frame != frame)
            {
                m_frame = frame;
                m_remaining = limit;
            }

            return m_remaining;
        }
    };


    //! @brief Component load/init/activate state machine, per-entity asset contributions and pending replacements.
    //! Every method runs at a main-thread safe point.
    struct EntityLifecycle final
    {
        //! @brief Hard asset dependency discovered while copying an authored component.
        using AssetDependency = std::pair<IO::AssetID, Rtti::TypeID>;

        static constexpr uint32_t kEntitiesPerFrame = 32;

        explicit EntityLifecycle(EntityWorld::Impl& world);

        //! @brief Finish ready replacements, then prepare, activate and publish every dirty hierarchy root.
        void Advance();

        //! @brief Queue the entity's root for preparation and clear cached readiness up the hierarchy.
        void MarkUnready(Entity& entity);

        //! @brief Take ownership of a candidate value for an active component; the old value stays live until it is ready.
        void StageReplacement(Entity& entity, const EntityComponentInfo& info, void* value);
        //! @brief Tear down pending candidates of one type, or all types when null.
        void CancelReplacements(Entity& entity, Rtti::TypeID type = Rtti::TypeID::kNull, bool keepAuthoredValues = false);
        [[nodiscard]] static bool HasPendingReplacements(const Entity& entity);

        //! @brief Contribute discovered dependencies for a freshly copied component, skipping a second discovery pass.
        void InstallDependencies(Entity& entity, uint32_t column, festd::span<const AssetDependency> dependencies);
        //! @brief Contribute a dependency from a component Load hook.
        void RequireAsset(Entity& entity, IO::AssetID id, Rtti::TypeID type);
        //! @brief Advance one stored component through discovery and loading.
        LifecycleResult LoadComponent(Entity& entity, uint32_t column);

        //! @brief Undo every completed stage of a stored component, optionally destroying its value.
        void TeardownComponent(Entity& entity, uint32_t column, bool destroy);
        void DeactivateSubtree(Entity& entity, bool keepAuthoredValues = true);
        //! @brief Reset runtime state descendants-first while retaining allocated identity.
        void UnloadSubtree(Entity& entity);

    private:
        struct LoadingScope
        {
            Rtti::TypeID m_component = Rtti::TypeID::kNull;
            uint64_t m_transition = 0;
        };

        EntityResources& GetResources(Entity& entity);
        EntityResidencySet& ResidencyOwner(Entity& entity);
        void ContributeAsset(Entity& entity, const LoadingScope& scope, IO::AssetID id, Rtti::TypeID type);
        void ReleaseAssets(Entity& entity, Rtti::TypeID component, uint64_t transition = 0);
        LifecycleResult PollDependencies(Entity& entity, const LoadingScope& scope);

        LifecycleResult LoadValue(Entity& entity, const EntityComponentInfo& info, void* data, ComponentStage& stage,
                                  uint64_t transition = 0);
        void TeardownValue(Entity& entity, const EntityComponentInfo& info, void* data, ComponentStage& stage,
                           uint64_t transition = 0);
        void AdvanceReplacements(Entity& entity);

        bool AdmitEntity(Entity& entity);
        bool PrepareSubtree(Entity& entity);
        LifecycleResult ActivateSubtree(Entity& entity);
        void DeactivateEntity(Entity& entity, bool keepAuthoredValues);
        void PublishSubtree(Entity& entity);
        void PublishAdditions(Entity& entity);
        void UnwindSubtree(Entity& entity);

        EntityWorld::Impl& m_world;
        festd::vector<EntityID> m_replacements;
        festd::vector<EntityID> m_roots;
        //! @brief Component and transition currently allowed to contribute dependencies through RequireAsset.
        LoadingScope m_loading;
        uint64_t m_nextTransition = 1;
        FrameQuota m_quota;
    };
} // namespace FE::Framework
