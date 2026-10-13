#include <EASTL/algorithm.h>
#include <GameFramework/Systems/TransformationSystem.h>
#include <cmath>

namespace FE::GameFramework
{
    namespace
    {
        bool IsAffine(const Matrix4x4& matrix)
        {
            return matrix.m_03 == 0 && matrix.m_13 == 0 && matrix.m_23 == 0 && matrix.m_33 == 1;
        }


        Matrix4x4 ComposeLocal(const TransformComponent& local, const NonUniformScaleComponent* scale)
        {
            const Matrix4x4 matrix = Transform::ToMatrix(local.m_local);
            return scale ? Matrix4x4::Scale(scale->m_scale) * matrix : matrix;
        }


        bool DecomposeLocal(const Matrix4x4& matrix, bool hasScaleModifier, Transform& local, Vector3& modifier)
        {
            Vector3 translation;
            Quaternion rotation;
            Vector3 scale;
            Vector3 shear;
            if (!Math::DecomposeTransform(matrix, translation, rotation, scale, shear))
                return false;

            local = Transform::Create(translation, rotation, hasScaleModifier ? 1.0f : scale.x);
            modifier = hasScaleModifier ? scale : Vector3(1, 1, 1);

            if (Build::IsDebug())
            {
                if (!Math::CmpEqual(shear, Vector3::kZero))
                    FE_DebugBreak();

                if (!hasScaleModifier)
                {
                    const float scaleMagnitude = Math::Max(Math::Abs(scale.x), Math::Max(Math::Abs(scale.y), Math::Abs(scale.z)));
                    if (!Math::CmpEqual(scale, Vector3(scale.x), Constants::kEpsilon * scaleMagnitude))
                        FE_DebugBreak();
                }

                const Matrix4x4 reconstructed = Matrix4x4::Scale(modifier) * Transform::ToMatrix(local);
                for (uint32_t i = 0; i < 16; ++i)
                {
                    const float tolerance = Constants::kEpsilon * Math::Max(1.0f, Math::Abs(matrix.m_values[i]));
                    if (Math::Abs(reconstructed.m_values[i] - matrix.m_values[i]) > tolerance)
                        FE_DebugBreak();
                }
            }

            return true;
        }


        bool EvaluateWorld(const Framework::ReparentContext& context, uint32_t entity, Matrix4x4& result)
        {
            result = Matrix4x4::kIdentity;

            uint32_t depth = 0;
            while (entity < context.m_entityCount)
            {
                if (++depth > context.m_entityCount)
                    return false;

                const auto* local = context.Read<TransformComponent>(entity);
                if (!local || !context.Read<WorldTransformComponent>(entity))
                    break;

                const auto* scale = context.Read<NonUniformScaleComponent>(entity);
                result = result * ComposeLocal(*local, scale);
                entity = context.GetParent(entity);
            }

            return IsAffine(result);
        }


        bool PrepareReparent(Framework::ReparentContext& context)
        {
            if (!context.Read<TransformComponent>(context.m_target) || !context.Read<WorldTransformComponent>(context.m_target))
                return true;

            Matrix4x4 oldWorld;
            if (!EvaluateWorld(context, context.m_target, oldWorld))
                return false;

            Matrix4x4 parentWorld;
            if (!EvaluateWorld(context, context.m_newParent, parentWorld))
                return false;

            const Vector3 parentX{ parentWorld.m_00, parentWorld.m_01, parentWorld.m_02 };
            const Vector3 parentY{ parentWorld.m_10, parentWorld.m_11, parentWorld.m_12 };
            const Vector3 parentZ{ parentWorld.m_20, parentWorld.m_21, parentWorld.m_22 };
            if (Math::Dot(parentX, Math::Cross(parentY, parentZ)) == 0)
                return false;

            Matrix4x4 inverseParent = Math::InvertTransform(parentWorld);
            inverseParent.m_03 = inverseParent.m_13 = inverseParent.m_23 = 0;
            inverseParent.m_33 = 1;

            const Matrix4x4 candidate = oldWorld * inverseParent;
            const bool hasScaleModifier = context.Read<NonUniformScaleComponent>(context.m_target) != nullptr;

            Transform candidateLocal;
            Vector3 candidateScale;
            if (!DecomposeLocal(candidate, hasScaleModifier, candidateLocal, candidateScale))
                return false;

            auto* local = context.Write<TransformComponent>(context.m_target);
            if (!local)
                return false;

            local->m_local = candidateLocal;

            if (hasScaleModifier)
            {
                auto* scale = context.Write<NonUniformScaleComponent>(context.m_target);
                if (!scale)
                    return false;

                scale->m_scale = candidateScale;
            }

            return true;
        }
    } // namespace


    void TransformationSystem::Init(Framework::EntityWorld& world)
    {
        world.Components().Register<TransformComponent>();
        world.Components().Register<NonUniformScaleComponent>();
        world.Components().Register<WorldTransformComponent>({}, { true });
        world.Components().AddRuntimeCompanion<TransformComponent, WorldTransformComponent>();
        world.SetReparentHandler(PrepareReparent);
        m_changes = {};
    }


    void TransformationSystem::Shutdown(Framework::EntityWorld& world)
    {
        world.SetReparentHandler(nullptr);
        m_completion.Reset();
    }


    void TransformationSystem::Update(Framework::EntityUpdateContext& context)
    {
        FE_PROFILER_ZONE();

        using namespace Framework;

        const auto traverse = [](const TransformComponent& local,
                                 const NonUniformScaleComponent* scale,
                                 const WorldTransformComponent* parent,
                                 WorldTransformComponent& output) {
            // Row-vector convention: apply the scale modifier, then local transform, then parent world.
            const Matrix4x4 effectiveLocal = ComposeLocal(local, scale);
            output.m_world = parent ? effectiveLocal * parent->m_world : effectiveLocal;
        };

        using Query = CascadeQuery<const TransformComponent,
                                   const NonUniformScaleComponent*,
                                   Parent<const WorldTransformComponent*>,
                                   WorldTransformComponent>;

        m_completion = Query::TraverseChanged(context, Phases::Transformation, m_changes, traverse, ExecutionPolicy::kParallel);
    }
} // namespace FE::GameFramework
