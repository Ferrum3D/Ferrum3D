#include <GameFramework/TransformationSystem.h>
#include <algorithm>
#include <cmath>

namespace FE::GameFramework
{
    namespace
    {
        bool IsFinite(const Matrix4x4& matrix)
        {
            for (const float value : matrix.m_values)
            {
                if (!std::isfinite(value))
                    return false;
            }

            return true;
        }


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

            constexpr float kTolerance = 0.00001f;
            if (!Math::CmpEqual(shear, Vector3::kZero, kTolerance))
                return false;

            if (!hasScaleModifier)
            {
                const float scaleMagnitude = std::max({ std::abs(scale.x), std::abs(scale.y), std::abs(scale.z) });
                if (!Math::CmpEqual(scale, Vector3(scale.x), kTolerance * scaleMagnitude))
                    return false;
            }

            local = Transform::Create(translation, rotation, hasScaleModifier ? 1.0f : scale.x);
            modifier = hasScaleModifier ? scale : Vector3(1, 1, 1);
            const Matrix4x4 reconstructed = Matrix4x4::Scale(modifier) * Transform::ToMatrix(local);
            if (!IsFinite(reconstructed))
                return false;

            for (uint32_t i = 0; i < 16; ++i)
            {
                const float tolerance = kTolerance * std::max(1.0f, std::abs(matrix.m_values[i]));
                if (std::abs(reconstructed.m_values[i] - matrix.m_values[i]) > tolerance)
                    return false;
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

            return IsFinite(result) && IsAffine(result);
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

            Matrix4x4 inverseParent = Math::InvertTransform(parentWorld);
            if (!IsFinite(inverseParent))
                return false;

            inverseParent.m_03 = inverseParent.m_13 = inverseParent.m_23 = 0;
            inverseParent.m_33 = 1;
            const Matrix4x4 candidate = oldWorld * inverseParent;
            if (!IsFinite(candidate))
                return false;

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
        using TransformQuery = Framework::CascadeQuery<const TransformComponent,
                                                       const NonUniformScaleComponent*,
                                                       Framework::Parent<const WorldTransformComponent*>,
                                                       WorldTransformComponent>;

        const auto updateWorldTransforms = [](const TransformComponent& local,
                                              const NonUniformScaleComponent* scale,
                                              const WorldTransformComponent* parent,
                                              WorldTransformComponent& output) {
            // Row-vector convention: apply the scale modifier, then local transform, then parent world.
            const Matrix4x4 effectiveLocal = ComposeLocal(local, scale);
            output.m_world = parent ? effectiveLocal * parent->m_world : effectiveLocal;
        };

        m_completion = TransformQuery::TraverseChanged(context,
                                                       Phases::Transformation,
                                                       m_changes,
                                                       updateWorldTransforms,
                                                       Framework::ExecutionPolicy::kParallel);
    }
} // namespace FE::GameFramework
