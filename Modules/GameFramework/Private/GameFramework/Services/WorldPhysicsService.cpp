#include <Core/Base/Platform.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Memory/Memory.h>
#include <GameFramework/Services/WorldPhysicsService.h>

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/FixedSizeFreeList.h>
#include <Jolt/Core/JobSystem.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>
#include <cmath>

namespace FE::GameFramework
{
    namespace
    {
        uint32_t g_physicsUsers = 0;

        JPH::Vec3 ToJolt(const Vector3 value)
        {
            return { value.x, value.y, value.z };
        }


        JPH::Quat ToJolt(const Quaternion value)
        {
            return { value.x, value.y, value.z, value.w };
        }


        bool IsFinite(const Vector3 value)
        {
            return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
        }


        bool IsRotation(const Quaternion value)
        {
            const float length = value.x * value.x + value.y * value.y + value.z * value.z + value.w * value.w;
            return std::isfinite(length) && std::abs(length - 1.0f) < 0.001f;
        }


        struct PhysicsJobs final : JPH::JobSystem
        {
            struct EngineJob final
                : Job
                , Jobs::JobNode
            {
                EngineJob(const char* name, JPH::ColorArg color, PhysicsJobs* owner, const JobFunction& function,
                          const JPH::uint32 dependencies)
                    : Job(name, color, owner, function, dependencies)
                {
                }


                void Execute() override
                {
                    FE_PROFILER_ZONE_NAMED("Physics job");
                    Job::Execute();
                    Job::Release();
                }
            };


            struct PhysicsBarrier final : Barrier
            {
                Rc<WaitGroup> m_completion = WaitGroup::Create(0);

                void AddJob(const JobHandle& job) override
                {
                    m_completion->Add(1);
                    if (!job.GetPtr()->SetBarrier(this))
                        m_completion->Signal();
                }

                void AddJobs(const JobHandle* jobs, const JPH::uint count) override
                {
                    for (JPH::uint i = 0; i < count; ++i)
                        AddJob(jobs[i]);
                }

                void OnJobFinished(Job*) override
                {
                    m_completion->Signal();
                }
            };

            JPH::FixedSizeFreeList<EngineJob> m_jobs;
            PhysicsBarrier m_barrier;
            Rc<WaitGroup> m_queuedJobs = WaitGroup::Create(1);
            const int m_concurrency = static_cast<int>(Math::Clamp(Platform::GetCpuInfo().m_physicalCores, 1u, 16u));

            PhysicsJobs()
            {
                m_jobs.Init(JPH::cMaxPhysicsJobs, 128);
            }


            int GetMaxConcurrency() const override
            {
                return m_concurrency;
            }

            JobHandle CreateJob(const char* name, JPH::ColorArg color, const JobFunction& function,
                                const JPH::uint32 dependencies) override
            {
                const uint32_t index = m_jobs.ConstructObject(name, color, this, function, dependencies);
                FE_Assert(index != JPH::FixedSizeFreeList<EngineJob>::cInvalidObjectIndex, "Physics job capacity exceeded");

                Job* job = &m_jobs.Get(index);
                JobHandle handle(job);
                if (dependencies == 0)
                    QueueJob(job);

                return handle;
            }

            Barrier* CreateBarrier() override
            {
                FE_AssertDebug(m_barrier.m_completion->IsSignaled());
                m_barrier.m_completion->Add(1);
                return &m_barrier;
            }

            void DestroyBarrier([[maybe_unused]] Barrier* barrier) override
            {
                FE_AssertDebug(barrier == &m_barrier && m_barrier.m_completion->IsSignaled());
            }

            void WaitForJobs(Barrier* barrier) override
            {
                auto& completion = static_cast<PhysicsBarrier*>(barrier)->m_completion;
                completion->Signal();
                completion->Wait();

                // Drain engine jobs as well: Jolt can finish its barrier before their final Release call.
                m_queuedJobs->Signal();
                m_queuedJobs->Wait();
                m_queuedJobs->Add(1);
            }

            void QueueJob(Job* job) override
            {
                job->AddRef();
                m_queuedJobs->Add(1);
                static_cast<EngineJob*>(job)->DispatchForeground(m_queuedJobs.Get());
            }

            void QueueJobs(Job** jobs, const JPH::uint count) override
            {
                for (JPH::uint i = 0; i < count; ++i)
                    QueueJob(jobs[i]);
            }

            void FreeJob(Job* job) override
            {
                m_jobs.DestructObject(static_cast<EngineJob*>(job));
            }
        };


        struct BroadPhaseLayers final : JPH::BroadPhaseLayerInterface
        {
            JPH::uint GetNumBroadPhaseLayers() const override
            {
                return 2;
            }


            JPH::BroadPhaseLayer GetBroadPhaseLayer(const JPH::ObjectLayer layer) const override
            {
                return JPH::BroadPhaseLayer(static_cast<JPH::uint8>(layer));
            }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
            const char* GetBroadPhaseLayerName(const JPH::BroadPhaseLayer layer) const override
            {
                return layer.GetValue() == 0 ? "Static" : "Moving";
            }
#endif
        };


        struct BroadPhaseFilter final : JPH::ObjectVsBroadPhaseLayerFilter
        {
            bool ShouldCollide(const JPH::ObjectLayer object, const JPH::BroadPhaseLayer layer) const override
            {
                return object != 0 || layer.GetValue() != 0;
            }
        };


        struct ObjectFilter final : JPH::ObjectLayerPairFilter
        {
            bool ShouldCollide(const JPH::ObjectLayer left, const JPH::ObjectLayer right) const override
            {
                return left != 0 || right != 0;
            }
        };


        JPH::ShapeSettings::ShapeResult CreatePrimitive(const PhysicsShapeAsset& shape)
        {
            using Result = JPH::ShapeSettings::ShapeResult;
            const auto invalid = [] {
                Result result;
                result.SetError("Invalid primitive dimensions");
                return result;
            };

            switch (shape.m_type)
            {
            case PhysicsShapeType::kSphere:
                if (!(std::isfinite(shape.m_radius) && shape.m_radius > 0))
                    return invalid();

                return JPH::SphereShapeSettings(shape.m_radius).Create();
            case PhysicsShapeType::kBox:
                if (!IsFinite(shape.m_halfExtent)
                    || Math::Min(shape.m_halfExtent.x, Math::Min(shape.m_halfExtent.y, shape.m_halfExtent.z)) <= 0)
                {
                    return invalid();
                }

                return JPH::BoxShapeSettings(ToJolt(shape.m_halfExtent), 0.0f).Create();
            case PhysicsShapeType::kCapsule:
            case PhysicsShapeType::kCylinder:
                if (!(std::isfinite(shape.m_radius) && shape.m_radius > 0))
                    return invalid();

                if (!(std::isfinite(shape.m_halfHeight) && shape.m_halfHeight > 0))
                    return invalid();

                if (shape.m_type == PhysicsShapeType::kCapsule)
                    return JPH::CapsuleShapeSettings(shape.m_halfHeight, shape.m_radius).Create();

                return JPH::CylinderShapeSettings(shape.m_halfHeight, shape.m_radius, 0.0f).Create();
            }

            return invalid();
        }


        JPH::Ref<JPH::Shape> CreateEntryShape(const PhysicsBodyShape& entry)
        {
            if (!IsFinite(entry.m_position) || !IsRotation(entry.m_rotation))
                return {};

            const bool hasAsset = entry.m_asset.GetAssetID().IsValid();
            const auto asset = hasAsset ? entry.m_asset.GetAssetHandle().Read() : IO::AssetRead<PhysicsShapeAsset>();
            if (hasAsset && !asset)
                return {};

            const auto primitive = CreatePrimitive(asset ? *asset.Get() : entry.m_shape);
            if (primitive.HasError())
            {
                Logger::LogError("Invalid physics shape: {}", primitive.GetError().c_str());
                return {};
            }

            return primitive.Get();
        }


        JPH::RefConst<JPH::Shape> CreateShape(const PhysicsBodyComponent& definition, const Vector3 scale)
        {
            JPH::ShapeSettings::ShapeResult result;
            if (definition.m_shapes.size() == 1)
            {
                const PhysicsBodyShape& entry = definition.m_shapes.front();
                const JPH::Ref<JPH::Shape> shape = CreateEntryShape(entry);
                if (!shape)
                    return {};

                const bool hasOffset = entry.m_position != Vector3::kZero || entry.m_rotation != Quaternion::kIdentity;
                if (hasOffset)
                    result =
                        JPH::RotatedTranslatedShapeSettings(ToJolt(entry.m_position), ToJolt(entry.m_rotation), shape).Create();
                else
                    result.Set(shape);
            }
            else
            {
                JPH::StaticCompoundShapeSettings compound;
                for (const PhysicsBodyShape& entry : definition.m_shapes)
                {
                    const JPH::Ref<JPH::Shape> shape = CreateEntryShape(entry);
                    if (!shape)
                        return {};

                    compound.AddShape(ToJolt(entry.m_position), ToJolt(entry.m_rotation), shape);
                }

                result = compound.Create();
            }

            if (result.HasError() || !result.Get()->IsValidScale(ToJolt(scale)))
                return {};

            const auto scaled = result.Get()->ScaleShape(ToJolt(scale));
            return scaled.HasError() ? JPH::RefConst<JPH::Shape>() : JPH::RefConst<JPH::Shape>(scaled.Get());
        }
    } // namespace


    struct WorldPhysicsService::Impl final
    {
        BroadPhaseLayers m_layers;
        BroadPhaseFilter m_broadPhaseFilter;
        ObjectFilter m_objectFilter;
        PhysicsJobs m_jobs;
        JPH::JobSystemSingleThreaded m_serialJobs{ JPH::cMaxPhysicsJobs };
        JPH::TempAllocatorImplWithMallocFallback m_temporary{ 16 * 1024 * 1024 };
        JPH::PhysicsSystem m_physics;
    };


    WorldPhysicsService::WorldPhysicsService() = default;


    WorldPhysicsService::~WorldPhysicsService()
    {
        FE_Assert(!m_impl, "Remove physics service before destroying it");
    }


    void WorldPhysicsService::Init(Framework::EntityWorld&)
    {
        FE_Assert(!m_impl);
        if (g_physicsUsers++ == 0)
        {
            JPH::Allocate = [](const size_t size) {
                return Memory::DefaultAllocate(size, 16);
            };
            JPH::Reallocate = [](void* block, size_t, const size_t size) {
                return Memory::DefaultReallocate(block, size);
            };
            JPH::Free = Memory::DefaultFree;
            JPH::AlignedAllocate = [](const size_t size, const size_t alignment) {
                return Memory::DefaultAllocate(size, alignment);
            };
            JPH::AlignedFree = Memory::DefaultFree;
            FE_Assert(!JPH::Factory::sInstance);
            JPH::Factory::sInstance = Memory::DefaultNew<JPH::Factory>();
            JPH::RegisterTypes();
        }

        m_impl = Memory::DefaultNew<Impl>();
        m_impl->m_physics.Init(65536, 0, 65536, 16384, m_impl->m_layers, m_impl->m_broadPhaseFilter, m_impl->m_objectFilter);
    }


    void WorldPhysicsService::Shutdown(Framework::EntityWorld&)
    {
        FE_Assert(m_impl && GetBodyCount() == 0);
        Memory::DefaultDelete(m_impl);
        m_impl = nullptr;
        if (--g_physicsUsers == 0)
        {
            JPH::UnregisterTypes();
            Memory::DefaultDelete(JPH::Factory::sInstance);
            JPH::Factory::sInstance = nullptr;
        }
    }


    bool WorldPhysicsService::CreateBody(const PhysicsBodyComponent& definition, const Transform& transform, const Vector3 scale,
                                         uint32_t& body)
    {
        if (definition.m_shapes.empty() || !IsFinite(scale) || Math::Min(scale.x, Math::Min(scale.y, scale.z)) <= 0)
            return false;

        if (!IsFinite(transform.Translation()) || !IsRotation(transform.Rotation()))
            return false;

        if (!IsFinite(definition.m_linearVelocity) || !IsFinite(definition.m_angularVelocity))
            return false;

        if (!(std::isfinite(definition.m_mass) && definition.m_mass > 0))
            return false;

        if (!(std::isfinite(definition.m_friction) && definition.m_friction >= 0))
            return false;

        if (!(std::isfinite(definition.m_restitution) && definition.m_restitution >= 0 && definition.m_restitution <= 1))
            return false;

        JPH::EMotionType motion;
        switch (definition.m_motionType)
        {
        case PhysicsMotionType::kStatic:
            motion = JPH::EMotionType::Static;
            break;
        case PhysicsMotionType::kDynamic:
            motion = JPH::EMotionType::Dynamic;
            break;
        case PhysicsMotionType::kKinematic:
            motion = JPH::EMotionType::Kinematic;
            break;
        default:
            return false;
        }

        const auto shape = CreateShape(definition, scale);
        if (!shape)
            return false;

        JPH::BodyCreationSettings settings(shape,
                                           ToJolt(transform.Translation()),
                                           ToJolt(transform.Rotation()),
                                           motion,
                                           motion == JPH::EMotionType::Static ? 0 : 1);
        settings.mFriction = definition.m_friction;
        settings.mRestitution = definition.m_restitution;
        settings.mLinearVelocity = ToJolt(definition.m_linearVelocity);
        settings.mAngularVelocity = ToJolt(definition.m_angularVelocity);
        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        settings.mMassPropertiesOverride.mMass = definition.m_mass;

        const auto id = m_impl->m_physics.GetBodyInterface().CreateAndAddBody(
            settings,
            motion == JPH::EMotionType::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
        if (id.IsInvalid())
            return false;

        body = id.GetIndexAndSequenceNumber();
        return true;
    }


    void WorldPhysicsService::DestroyBody(const uint32_t body)
    {
        if (body == UINT32_MAX)
            return;

        auto& bodies = m_impl->m_physics.GetBodyInterface();
        const JPH::BodyID id(body);
        bodies.RemoveBody(id);
        bodies.DestroyBody(id);
    }


    void WorldPhysicsService::SetTransform(const uint32_t body, const Transform& transform, const PhysicsMotionType motion,
                                           const float deltaSeconds)
    {
        auto& bodies = m_impl->m_physics.GetBodyInterface();
        if (motion == PhysicsMotionType::kKinematic)
        {
            bodies.MoveKinematic(JPH::BodyID(body), ToJolt(transform.Translation()), ToJolt(transform.Rotation()), deltaSeconds);
        }
        else
        {
            bodies.SetPositionAndRotation(JPH::BodyID(body),
                                          ToJolt(transform.Translation()),
                                          ToJolt(transform.Rotation()),
                                          motion == PhysicsMotionType::kStatic ? JPH::EActivation::DontActivate
                                                                               : JPH::EActivation::Activate);
        }
    }


    Transform WorldPhysicsService::GetTransform(const uint32_t body, const float scale) const
    {
        JPH::RVec3 position;
        JPH::Quat rotation;
        m_impl->m_physics.GetBodyInterface().GetPositionAndRotation(JPH::BodyID(body), position, rotation);
        return Transform::Create(Vector3(position.GetX(), position.GetY(), position.GetZ()),
                                 Quaternion(rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW()),
                                 scale);
    }


    uint32_t WorldPhysicsService::GetBodyCount() const
    {
        return m_impl->m_physics.GetNumBodies();
    }


    void WorldPhysicsService::GetVelocity(const uint32_t body, Vector3& linear, Vector3& angular) const
    {
        JPH::Vec3 linearVelocity;
        JPH::Vec3 angularVelocity;
        m_impl->m_physics.GetBodyInterface().GetLinearAndAngularVelocity(JPH::BodyID(body), linearVelocity, angularVelocity);
        linear = Vector3(linearVelocity.GetX(), linearVelocity.GetY(), linearVelocity.GetZ());
        angular = Vector3(angularVelocity.GetX(), angularVelocity.GetY(), angularVelocity.GetZ());
    }


    void WorldPhysicsService::Step(const float deltaSeconds)
    {
        FE_PROFILER_ZONE();
        constexpr uint32_t kMinParallelBodies = 16;
        const uint32_t activeBodies = m_impl->m_physics.GetNumActiveBodies(JPH::EBodyType::RigidBody);
        JPH::JobSystem* jobs = activeBodies < kMinParallelBodies ? static_cast<JPH::JobSystem*>(&m_impl->m_serialJobs)
                                                                 : static_cast<JPH::JobSystem*>(&m_impl->m_jobs);
        [[maybe_unused]] const auto error = m_impl->m_physics.Update(deltaSeconds, 1, &m_impl->m_temporary, jobs);
        FE_AssertDebug(error == JPH::EPhysicsUpdateError::None, "Physics capacity exceeded");
    }
} // namespace FE::GameFramework
