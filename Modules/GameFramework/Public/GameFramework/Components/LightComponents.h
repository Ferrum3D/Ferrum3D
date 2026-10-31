#pragma once
#include <Framework/Entities/EntityComponentRegistry.h>
#include <Graphics/Features/Lighting/LocalLightingSceneModule.h>

namespace FE::GameFramework
{
    using LightUnit = Graphics::LightUnit;
    using LightColorMode = Graphics::LightColorMode;

    struct PointLightComponent final : Graphics::LocalLightParameters
    {
        FE_RTTI_Reflect("01983C05-87C8-410F-9ED1-822A133691B9");
        FE_RTTI_Serialize();

        FE_SKIP_SERIALIZING Graphics::LightHandle m_light;

        PointLightComponent() = default;
        PointLightComponent(const PointLightComponent& other);
        PointLightComponent(PointLightComponent&&) noexcept = default;
        Framework::LifecycleResult Activate(Framework::ComponentContext& context);
        void Deactivate(Framework::ComponentContext& context);
    };


    //! Emission follows local +Z. Angles are half-angles in radians; scale does not change the range.
    struct SpotLightComponent final : Graphics::LocalLightParameters
    {
        FE_RTTI_Reflect("8FE31BCE-BAAD-4AE8-8437-BA465D5B1773");
        FE_RTTI_Serialize();

        FE_SKIP_SERIALIZING Graphics::LightHandle m_light;

        float m_innerAngle = Constants::kPI / 8;
        float m_outerAngle = Constants::kPI / 6;

        SpotLightComponent() = default;
        SpotLightComponent(const SpotLightComponent& other);
        SpotLightComponent(SpotLightComponent&&) noexcept = default;
        Framework::LifecycleResult Activate(Framework::ComponentContext& context);
        void Deactivate(Framework::ComponentContext& context);
        [[nodiscard]] bool Validate() const;
    };
} // namespace FE::GameFramework
