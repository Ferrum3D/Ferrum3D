#include <Core/Math/Packing.h>
#include <Graphics/Features/Lighting/GlobalLightingSceneModule.h>
#include <Graphics/Features/Lighting/LocalLightingSceneModule.h>

#include <ArHosekSkyModel.h>
#include <cmath>
#include <gtest/gtest.h>

namespace FE::Graphics
{
    namespace
    {
        double EvaluateSkyChannel(const GlobalLightingData& data, const uint32_t channel, const Vector3 direction)
        {
            const Vector3 sun(data.m_directionIlluminance.x, data.m_directionIlluminance.y, data.m_directionIlluminance.z);
            const double cosine = Math::Clamp(static_cast<double>(Math::Dot(direction, sun)), -1.0, 1.0);
            const double gamma = std::acos(cosine);
            const double zenith = Math::Max(static_cast<double>(direction.y), 0.0);
            double c[9];
            for (uint32_t index = 0; index < 9; ++index)
                c[index] = data.m_skyCoefficients[index].m_values[channel];

            const double mie = (1 + cosine * cosine) / std::pow(1 + c[8] * c[8] - 2 * c[8] * cosine, 1.5);
            return (1 + c[0] * std::exp(c[1] / (zenith + 0.01)))
                * (c[2] + c[3] * std::exp(c[4] * gamma) + c[5] * cosine * cosine + c[6] * mie + c[7] * std::sqrt(zenith))
                * data.m_skyRadiance.m_values[channel] * 120000;
        }
    } // namespace


    TEST(HosekSky, PackedCoefficientsMatchUpstreamRgbRadiance)
    {
        GlobalLightingParameters parameters;
        parameters.m_directionalLightIlluminanceLux = 120000;
        parameters.m_directionalLightOrientation = Quaternion::RotationX(-0.6f);
        parameters.m_atmosphere.m_groundAlbedo = PackedVector3F(0.05f, 0.3f, 0.8f);
        const Vector3 directions[] = { Vector3(0, 1, 0), Vector3(0, 0, 1), Math::Normalize(Vector3(1, 0.2f, -1)) };
        for (const float turbidity : { 1.0f, 2.5f, 10.0f })
        {
            parameters.m_atmosphere.m_turbidity = turbidity;
            const GlobalLightingData data = parameters.BuildRenderData();
            const double elevation = std::asin(static_cast<double>(data.m_directionIlluminance.y));
            for (uint32_t channel = 0; channel < 3; ++channel)
            {
                ArHosekSkyModelState* state =
                    arhosek_rgb_skymodelstate_alloc_init(turbidity,
                                                         parameters.m_atmosphere.m_groundAlbedo.m_values[channel],
                                                         elevation);
                for (const Vector3 direction : directions)
                {
                    const Vector3 sun(data.m_directionIlluminance.x,
                                      data.m_directionIlluminance.y,
                                      data.m_directionIlluminance.z);
                    const double theta = std::acos(static_cast<double>(direction.y));
                    const double gamma = std::acos(Math::Clamp(static_cast<double>(Math::Dot(direction, sun)), -1.0, 1.0));
                    const double expected = arhosek_tristim_skymodel_radiance(state, theta, gamma, channel) * 683;
                    EXPECT_NEAR(EvaluateSkyChannel(data, channel, direction), expected, std::abs(expected) * 0.0001);
                }

                arhosekskymodelstate_free(state);
            }
        }
    }


    TEST(HosekSky, SunDimsAndReddensTowardsHorizon)
    {
        GlobalLightingParameters parameters;
        parameters.m_directionalLightIlluminanceLux = 120000;
        parameters.m_directionalLightOrientation = Quaternion::RotationX(-1.57079632679f);
        const GlobalLightingData noon = parameters.BuildRenderData();
        parameters.m_directionalLightOrientation = Quaternion::RotationX(-0.05f);
        const GlobalLightingData sunset = parameters.BuildRenderData();
        EXPECT_GT(noon.m_sunIlluminance.y, 0.5f);
        EXPECT_LT(noon.m_sunIlluminance.y, 1.5f);
        EXPECT_LT(sunset.m_sunIlluminance.y, noon.m_sunIlluminance.y);
        EXPECT_GT(sunset.m_sunIlluminance.x / sunset.m_sunIlluminance.z, noon.m_sunIlluminance.x / noon.m_sunIlluminance.z);
    }


    TEST(HosekSky, DisplayRadiusPreservesSolarEnergy)
    {
        GlobalLightingParameters parameters;
        parameters.m_directionalLightIlluminanceLux = 120000;
        parameters.m_directionalLightOrientation = Quaternion::RotationX(-0.7f);
        const GlobalLightingData physical = parameters.BuildRenderData();
        parameters.m_atmosphere.m_sunAngularRadius *= 4;
        const GlobalLightingData enlarged = parameters.BuildRenderData();
        for (uint32_t channel = 0; channel < 3; ++channel)
            EXPECT_FLOAT_EQ(physical.m_sunIlluminance.m_values[channel], enlarged.m_sunIlluminance.m_values[channel]);
    }


    TEST(HosekSky, FiniteAtHorizonAndTurbidityLimits)
    {
        GlobalLightingParameters parameters;
        parameters.m_directionalLightIlluminanceLux = 120000;
        for (const float turbidity : { 1.0f, 10.0f })
        {
            parameters.m_atmosphere.m_turbidity = turbidity;
            for (const float angle : { 0.0f, -0.00001f, -1.57079632679f })
            {
                parameters.m_directionalLightOrientation = Quaternion::RotationX(angle);
                const GlobalLightingData data = parameters.BuildRenderData();
                for (uint32_t channel = 0; channel < 3; ++channel)
                {
                    EXPECT_TRUE(std::isfinite(data.m_sunIlluminance.m_values[channel]));
                    EXPECT_GE(data.m_sunIlluminance.m_values[channel], 0);
                    EXPECT_TRUE(std::isfinite(data.m_groundRadiance.m_values[channel]));
                    EXPECT_GE(data.m_groundRadiance.m_values[channel], 0);
                    EXPECT_TRUE(std::isfinite(EvaluateSkyChannel(data, channel, Vector3(0, 0, -1))));
                }
            }
        }
    }


    TEST(HosekSky, DisabledAndBelowHorizonAreDark)
    {
        GlobalLightingParameters parameters;
        const GlobalLightingData disabled = parameters.BuildRenderData();
        parameters.m_directionalLightIlluminanceLux = 120000;
        parameters.m_directionalLightOrientation = Quaternion::RotationX(0.1f);
        const GlobalLightingData night = parameters.BuildRenderData();
        for (uint32_t channel = 0; channel < 3; ++channel)
        {
            EXPECT_EQ(disabled.m_skyRadiance.m_values[channel], 0);
            EXPECT_EQ(disabled.m_sunIlluminance.m_values[channel], 0);
            EXPECT_EQ(night.m_skyRadiance.m_values[channel], 0);
            EXPECT_EQ(night.m_sunIlluminance.m_values[channel], 0);
            EXPECT_EQ(night.m_groundRadiance.m_values[channel], 0);
        }
    }


    TEST(HosekSky, ValidatesModelDomain)
    {
        SkyAtmosphereParameters parameters;
        for (const float turbidity : { 1.0f, 10.0f })
        {
            parameters.m_turbidity = turbidity;
            EXPECT_TRUE(parameters.Validate());
        }

        for (const float turbidity : { 0.99f, 10.01f, Constants::kNan, Constants::kInfinity })
        {
            parameters.m_turbidity = turbidity;
            EXPECT_FALSE(parameters.Validate());
        }

        parameters = {};
        parameters.m_groundAlbedo = PackedVector3F(0, 0.5f, 1);
        EXPECT_TRUE(parameters.Validate());
        parameters.m_groundAlbedo.z = 1.01f;
        EXPECT_FALSE(parameters.Validate());
        parameters = {};
        parameters.m_sunAngularRadius = 0;
        EXPECT_FALSE(parameters.Validate());
        parameters.m_sunAngularRadius = Constants::kNan;
        EXPECT_FALSE(parameters.Validate());
    }
} // namespace FE::Graphics


TEST(LocalLights, PhotometricUnitsPreserveIntegratedFlux)
{
    FE::Graphics::LocalLightParameters light;
    light.m_intensity = 2000;
    EXPECT_NEAR(light.GetCandela() * (4 * FE::Constants::kPI), 2000, 0.001f);
    const float inner = 0.3f;
    const float outer = 0.7f;
    const float candela = light.GetCandela(inner, outer);
    double flux = 0;
    constexpr uint32_t kSteps = 10000;
    const float outerCosine = std::cos(outer);
    const float innerCosine = std::cos(inner);
    for (uint32_t i = 0; i < kSteps; ++i)
    {
        const float cosine = -1 + (i + 0.5f) * (2.0f / kSteps);
        const float cone = FE::Math::Clamp((cosine - outerCosine) / (innerCosine - outerCosine), 0.0f, 1.0f);
        flux += candela * cone * cone * (3 - 2 * cone) * (4 * FE::Constants::kPI / kSteps);
    }

    EXPECT_NEAR(flux, 2000, 0.1);
    light.m_unit = FE::Graphics::LightUnit::kCandela;
    EXPECT_EQ(light.GetCandela(), 2000);
    EXPECT_EQ(light.GetCandela(inner, outer), 2000);
}


TEST(LocalLights, TemperatureIsLinearAndPreservesLuminance)
{
    FE::Graphics::LocalLightParameters light;
    for (const float kelvin : { 1000.0f, 2000.0f, 6500.0f, 40000.0f })
    {
        light.m_temperature = kelvin;
        ASSERT_TRUE(light.Validate());
        const auto color = light.GetLinearColor();
        EXPECT_GE(color.x, 0);
        EXPECT_GE(color.y, 0);
        EXPECT_GE(color.z, 0);
        EXPECT_NEAR(0.2126f * color.x + 0.7152f * color.y + 0.0722f * color.z, 1, 0.0001f);
    }

    light.m_range = 0;
    EXPECT_FALSE(light.Validate());
    light.m_range = 10;
    light.m_temperature = 0;
    EXPECT_FALSE(light.Validate());
    light.m_colorMode = FE::Graphics::LightColorMode::kRGB;
    light.m_color = { 0.2f, 0.3f, 0.4f };
    EXPECT_TRUE(light.Validate());
    EXPECT_EQ(light.GetLinearColor().x, 0.2f);
    light.m_intensity = -1;
    EXPECT_FALSE(light.Validate());
}


TEST(LocalLights, PackedRecordPreservesHdrColorAndSpatialPrecision)
{
    using namespace FE;
    using namespace FE::Graphics;
    EXPECT_EQ(sizeof(LocalLightData), 32u);
    const Vector3 colors[] = { Vector3(1),
                               Vector3(1, 0, 0),
                               Vector3(0.2f, 0.6f, 4),
                               Vector3(0.02f, 2, 10000),
                               Vector3(1e-20f, 2e-20f, 4e-20f),
                               Vector3(1e20f, 2e20f, 4e20f),
                               Vector3::kZero };
    for (const Vector3 color : colors)
    {
        const float candela = 1e12f;
        const auto packed = PackLocalLightData(Vector3(123456.25f, -8.125f, 0.001f), 98765.5f, color, candela);
        EXPECT_EQ(packed.m_positionRange.x, 123456.25f);
        EXPECT_EQ(packed.m_positionRange.y, -8.125f);
        EXPECT_EQ(packed.m_positionRange.z, 0.001f);
        EXPECT_EQ(packed.m_positionRange.w, 98765.5f);
        EXPECT_EQ(packed.m_coneAngles, kLocalPointLightCone);
        const Vector3 decoded = Math::Pack::R11G11B10FloatToRGB32Float(packed.m_color) * packed.m_colorMultiplier;
        const float peak = Math::Max(color.x, Math::Max(color.y, color.z));
        for (uint32_t channel = 0; channel < 3; ++channel)
        {
            const double expected = static_cast<double>(color.m_values[channel]) * candela;
            const double tolerance = expected * 0.017 + static_cast<double>(peak) * candela * 3.1e-11;
            EXPECT_NEAR(decoded.m_values[channel], expected, tolerance);
        }
    }

    const auto off = PackLocalLightData(Vector3::kZero, 10, Vector3(1), 0);
    EXPECT_EQ(off.m_colorMultiplier, 0);
    EXPECT_EQ(off.m_color, 0u);
}


TEST(LocalLights, PackedDirectionAndNarrowConesRetainAngularPrecision)
{
    using namespace FE;
    using namespace FE::Graphics;
    const Vector3 directions[] = { Vector3(0, 0, 1),   Vector3(0, 0, -1),       Vector3(1, 0, 0),  Vector3(0, -1, 0),
                                   Vector3(1, -2, -3), Vector3(-1, 2, -0.001f), Vector3(-1, -2, 3) };
    for (const Vector3 source : directions)
    {
        const Vector3 normal = Math::Normalize(source);
        const auto packed = PackLocalLightData(Vector3::kZero, 10, Vector3(1), 1, normal, 0.005f, 0.01f);
        const Vector3 decoded = Math::Pack::Octahedral16SnormToNormal(packed.m_direction);
        EXPECT_LT(Math::Length(normal - decoded), 0.00008f);
        EXPECT_NE(packed.m_coneAngles, kLocalPointLightCone);
        const float inner = (packed.m_coneAngles & 65535u) / kLocalLightConeAngleScale;
        const float outer = (packed.m_coneAngles >> 16) / kLocalLightConeAngleScale;
        EXPECT_NEAR(inner, 0.005f, 0.5f / kLocalLightConeAngleScale);
        EXPECT_NEAR(outer, 0.01f, 0.5f / kLocalLightConeAngleScale);
        EXPECT_LT(inner, outer);
    }

    const auto wide = PackLocalLightData(Vector3::kZero, 10, Vector3(1), 1, Vector3(0, 0, 1), 1.5707f, 1.57079f);
    EXPECT_NE(wide.m_coneAngles, kLocalPointLightCone);
}
