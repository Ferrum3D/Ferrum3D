#include <Graphics/Features/Lighting/GlobalLightingSceneModule.h>

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
