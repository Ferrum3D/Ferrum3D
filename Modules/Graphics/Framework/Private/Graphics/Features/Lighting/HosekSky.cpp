#include <Graphics/Features/Lighting/HosekSky.h>

#include <ArHosekSkyModel.h>
#include <cmath>

namespace FE::Graphics
{
    namespace
    {
        constexpr double kPi = 3.141592653589793;
        constexpr float kPhotometricScale = 683.0f / 120000.0f;

        double CieLobe(const double wavelength, const double center, const double leftScale, const double rightScale)
        {
            const double t = (wavelength - center) * (wavelength < center ? leftScale : rightScale);
            return std::exp(-0.5 * t * t);
        }


        Vector3 CieMatchingFunctions(const double wavelength)
        {
            // Wyman, Sloan and Shirley (2013), equation 4 and table 1:
            // https://jcgt.org/published/0002/02/01/paper.pdf
            const double x = 0.362 * CieLobe(wavelength, 442.0, 0.0624, 0.0374)
                + 1.056 * CieLobe(wavelength, 599.8, 0.0264, 0.0323) - 0.065 * CieLobe(wavelength, 501.1, 0.0490, 0.0382);
            const double y =
                0.821 * CieLobe(wavelength, 568.8, 0.0213, 0.0247) + 0.286 * CieLobe(wavelength, 530.9, 0.0613, 0.0322);
            const double z =
                1.217 * CieLobe(wavelength, 437.0, 0.0845, 0.0278) + 0.681 * CieLobe(wavelength, 459.0, 0.0385, 0.0725);
            return Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
        }


        Vector3 XyzToLinearRgb(const Vector3 xyz)
        {
            return Math::Max(Vector3(3.2406f * xyz.x - 1.5372f * xyz.y - 0.4986f * xyz.z,
                                     -0.9689f * xyz.x + 1.8758f * xyz.y + 0.0415f * xyz.z,
                                     0.0557f * xyz.x - 0.2040f * xyz.y + 1.0570f * xyz.z),
                             Vector3(0));
        }


        Vector3 ComputeSunIlluminance(ArHosekSkyModelState* state, const Vector3 sunDirection)
        {
            const Vector3 reference = sunDirection.y < 0.99f ? Vector3(0, 1, 0) : Vector3(0, 0, 1);
            const Vector3 tangent = Math::Normalize(Math::Cross(sunDirection, reference));
            const Vector3 bitangent = Math::Cross(sunDirection, tangent);
            const double cosineRadius = std::cos(state->solar_radius);
            Vector3 matchingFunctions[73];
            for (uint32_t sample = 0; sample <= 72; ++sample)
                matchingFunctions[sample] = CieMatchingFunctions(360 + 5 * sample);

            Vector3 xyz(0);

            // Integrate the physical solar disc, independently of the authored display radius.
            for (uint32_t ring = 0; ring < 8; ++ring)
            {
                const double cosine = 1 - (1 - cosineRadius) * (ring + 0.5) / 8;
                const double sine = std::sqrt(1 - cosine * cosine);
                const double gamma = std::acos(cosine);
                for (uint32_t sector = 0; sector < 8; ++sector)
                {
                    const double phi = 2 * kPi * (sector + 0.5) / 8;
                    const Vector3 direction = sunDirection * static_cast<float>(cosine)
                        + tangent * static_cast<float>(sine * std::cos(phi))
                        + bitangent * static_cast<float>(sine * std::sin(phi));
                    if (direction.y <= 0)
                        continue;

                    const double theta = std::acos(Math::Clamp(static_cast<double>(direction.y), 0.0, 1.0));
                    for (uint32_t sample = 0; sample <= 72; ++sample)
                    {
                        const double wavelength = 360 + 5 * sample;
                        // The public solar function includes sky radiance; remove it to avoid double counting.
                        const double solar = arhosekskymodel_solar_radiance(state, theta, gamma, wavelength)
                            - arhosekskymodel_radiance(state, theta, gamma, wavelength);
                        const double weight = sample == 0 || sample == 72 ? 2.5 : 5.0;
                        xyz = xyz + matchingFunctions[sample] * static_cast<float>(Math::Max(solar, 0.0) * weight * cosine);
                    }
                }
            }

            const float solidAnglePerSample = static_cast<float>(2 * kPi * (1 - cosineRadius) / 64);
            return XyzToLinearRgb(xyz * (solidAnglePerSample * kPhotometricScale));
        }


        Vector3 ComputeGroundRadiance(ArHosekSkyModelState* const* states, const Vector3 sunDirection,
                                      const Vector3 sunIlluminance, const Vector3 albedo)
        {
            Vector3 irradiance(0);
            for (uint32_t ring = 0; ring < 16; ++ring)
            {
                const double cosine = (ring + 0.5) / 16;
                const double sine = std::sqrt(1 - cosine * cosine);
                const double theta = std::acos(cosine);
                for (uint32_t sector = 0; sector < 32; ++sector)
                {
                    const double phi = 2 * kPi * (sector + 0.5) / 32;
                    const Vector3 direction(static_cast<float>(sine * std::cos(phi)),
                                            static_cast<float>(cosine),
                                            static_cast<float>(sine * std::sin(phi)));
                    const double gamma =
                        std::acos(Math::Clamp(static_cast<double>(Math::Dot(direction, sunDirection)), -1.0, 1.0));
                    for (uint32_t channel = 0; channel < 3; ++channel)
                    {
                        const double radiance = arhosek_tristim_skymodel_radiance(states[channel], theta, gamma, channel);
                        irradiance.m_values[channel] += static_cast<float>(Math::Max(radiance, 0.0) * cosine);
                    }
                }
            }

            irradiance = irradiance * (static_cast<float>(2 * kPi / (16 * 32)) * kPhotometricScale);
            irradiance = irradiance + sunIlluminance * sunDirection.y;
            return irradiance * albedo / static_cast<float>(kPi);
        }
    } // namespace


    void PrecomputeHosekSky(const SkyAtmosphereParameters& parameters, const Vector3 sunDirection, GlobalLightingData& data)
    {
        if (sunDirection.y < 0)
            return;

        const double elevation = std::asin(Math::Clamp(static_cast<double>(sunDirection.y), 0.0, 1.0));
        ArHosekSkyModelState* states[3]{};
        for (uint32_t channel = 0; channel < 3; ++channel)
        {
            states[channel] = arhosek_rgb_skymodelstate_alloc_init(parameters.m_turbidity,
                                                                   parameters.m_groundAlbedo.m_values[channel],
                                                                   elevation);
            data.m_skyRadiance.m_values[channel] = static_cast<float>(states[channel]->radiances[channel]) * kPhotometricScale;
            for (uint32_t coefficient = 0; coefficient < 9; ++coefficient)
                data.m_skyCoefficients[coefficient].m_values[channel] =
                    static_cast<float>(states[channel]->configs[channel][coefficient]);
        }

        ArHosekSkyModelState* solar = arhosekskymodelstate_alloc_init(elevation, parameters.m_turbidity, 0);
        const Vector3 sunIlluminance = ComputeSunIlluminance(solar, sunDirection);
        data.m_sunIlluminance = PackedVector4F(Vector4(sunIlluminance, 0));
        data.m_groundRadiance = PackedVector4F(
            Vector4(ComputeGroundRadiance(states, sunDirection, sunIlluminance, Vector3(parameters.m_groundAlbedo)), 0));
        arhosekskymodelstate_free(solar);
        for (ArHosekSkyModelState* state : states)
            arhosekskymodelstate_free(state);
    }
} // namespace FE::Graphics
