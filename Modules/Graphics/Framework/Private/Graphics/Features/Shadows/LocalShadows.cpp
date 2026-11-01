#include <Core/Math/Packing.h>
#include <Graphics/Features/Shadows/LocalShadows.h>
#include <cmath>
#include <cstring>

namespace FE::Graphics::LocalShadows
{
    namespace
    {
        //! Spot maps are a little wider than the cone so that filtering near its edge stays inside the map. Wider cones are
        //! clamped, which leaves their rim unshadowed.
        constexpr float kSpotFovMargin = 1.05f;
        constexpr float kMaxSpotHalfFov = 1.45f;

        Vector3 GetFaceDirection(const uint32_t face)
        {
            Vector3 direction = Vector3::kZero;
            direction.m_values[face / 2] = (face & 1) == 0 ? 1.0f : -1.0f;
            return direction;
        }


        struct Candidate final
        {
            LightHandle m_light;
            LocalLightData m_data;
            uint32_t m_faceCount = 1;
            float m_priority = 0.0f;
        };


        bool IsPointLight(const LocalLightData& data)
        {
            return data.m_coneAngles == kLocalPointLightCone;
        }


        // A rotation whose +Z axis looks along the direction.
        Quaternion CalculateLookRotation(const Vector3 forward)
        {
            const Vector3 up = Math::Abs(forward.y) > 0.99f ? Vector3::AxisX() : Vector3::AxisY();
            return Math::ExtractRotation(Math::Invert(Matrix4x4::LookAt(Vector3::kZero, forward, up)));
        }


        MapSetup CreateMapSetup(const Candidate& candidate, const uint32_t face)
        {
            const LocalLightData& data = candidate.m_data;
            const Vector3 position(data.m_positionRange.x, data.m_positionRange.y, data.m_positionRange.z);
            const float range = data.m_positionRange.w;

            MapSetup map;
            map.m_light = candidate.m_light;
            map.m_lightData = data;
            map.m_face = face;
            map.m_faceCount = candidate.m_faceCount;
            map.m_isUsed = true;

            Vector3 forward;
            if (IsPointLight(data))
            {
                forward = GetFaceDirection(face);
                map.m_fovY = Constants::kPI * 0.5f;
            }
            else
            {
                forward = Math::Normalize(Math::Pack::Octahedral16SnormToNormal(data.m_direction));
                const float outerAngle = static_cast<float>(data.m_coneAngles >> 16) / kLocalLightConeAngleScale;
                map.m_fovY = 2.0f * Math::Min(outerAngle * kSpotFovMargin + 0.02f, kMaxSpotHalfFov);
            }

            map.m_far = range;
            map.m_near = Math::Clamp(range * 0.002f, 0.01f, range * 0.5f);
            map.m_camera = Transform::Create(position, CalculateLookRotation(forward), 1.0f);

            // Matches View::SetCameraTransform followed by View::SetProjection.
            const Matrix4x4 viewMatrix = Transform::ToMatrix(Math::Invert(map.m_camera));
            map.m_viewProjection = viewMatrix * Matrix4x4::Projection(map.m_fovY, 1.0f, map.m_near, map.m_far);
            map.m_texelScale = 2.0f * std::tan(map.m_fovY * 0.5f) / kSunShadowVirtualSize;
            return map;
        }


        void CollectCandidates(const LocalLightingSceneModule& lights, const CameraDesc& camera,
                               festd::inline_vector<Candidate, 32>& candidates)
        {
            lights.ForEachShadowCaster([&](const LightHandle light, const LocalLightData& data) {
                const Vector3 position(data.m_positionRange.x, data.m_positionRange.y, data.m_positionRange.z);
                const float range = data.m_positionRange.w;
                if (!(range > 0.0f))
                    return;

                const Vector3 extent(range);
                if (!camera.m_frustum.Overlaps(Aabb{ position - extent, position + extent }))
                    return;

                // Approximate screen coverage: the angular size of the light sphere.
                const float distance = Math::Length(position - camera.m_position);
                Candidate& candidate = candidates.emplace_back();
                candidate.m_light = light;
                candidate.m_data = data;
                candidate.m_faceCount = IsPointLight(data) ? kLocalShadowPointFaceCount : 1;
                candidate.m_priority = range / Math::Max(distance - range, 0.1f);
            });

            festd::sort(candidates, [](const Candidate& lhs, const Candidate& rhs) {
                return lhs.m_priority > rhs.m_priority;
            });
        }


        //! First map of the light in the allocation, or kInvalidIndex.
        uint32_t FindLightMaps(const MapAllocation& allocation, const Candidate& candidate)
        {
            for (uint32_t mapIndex = 0; mapIndex < kMaxMapCount; ++mapIndex)
            {
                const MapSetup& map = allocation.m_maps[mapIndex];
                const bool isLightMap = map.IsFirstFace() && map.m_light == candidate.m_light;
                if (isLightMap && map.m_faceCount == candidate.m_faceCount)
                    return mapIndex;
            }

            return kInvalidIndex;
        }


        //! First map of a run of free maps, or kInvalidIndex.
        uint32_t FindFreeMaps(const MapAllocation& allocation, const uint32_t count)
        {
            uint32_t runLength = 0;
            for (uint32_t mapIndex = 0; mapIndex < kMaxMapCount; ++mapIndex)
            {
                runLength = allocation.m_maps[mapIndex].m_isUsed ? 0 : runLength + 1;
                if (runLength == count)
                    return mapIndex + 1 - count;
            }

            return kInvalidIndex;
        }
    } // namespace


    uint32_t SelectMaps(const LocalLightingSceneModule& lights, const CameraDesc& camera, MapAllocation& allocation)
    {
        FE_PROFILER_ZONE();

        festd::inline_vector<Candidate, 32> candidates;
        CollectCandidates(lights, camera, candidates);

        // The lights that fit into the budget, in priority order.
        festd::inline_vector<Candidate, kMaxMapCount> selected;
        uint32_t budget = kMaxMapCount;
        for (const Candidate& candidate : candidates)
        {
            if (candidate.m_faceCount > budget)
                continue;

            budget -= candidate.m_faceCount;
            selected.push_back(candidate);
        }

        // Maps of lights that are no longer selected are released first, so that new lights can take them.
        uint32_t resetMask = 0;
        festd::array<uint32_t, kMaxMapCount> firstMaps;
        for (uint32_t selectedIndex = 0; selectedIndex < selected.size(); ++selectedIndex)
            firstMaps[selectedIndex] = FindLightMaps(allocation, selected[selectedIndex]);

        festd::array<bool, kMaxMapCount> isKept{};
        for (uint32_t selectedIndex = 0; selectedIndex < selected.size(); ++selectedIndex)
        {
            if (firstMaps[selectedIndex] == kInvalidIndex)
                continue;

            for (uint32_t face = 0; face < selected[selectedIndex].m_faceCount; ++face)
                isKept[firstMaps[selectedIndex] + face] = true;
        }

        for (uint32_t mapIndex = 0; mapIndex < kMaxMapCount; ++mapIndex)
        {
            MapSetup& map = allocation.m_maps[mapIndex];
            if (!map.m_isUsed || isKept[mapIndex])
                continue;

            map = {};
            resetMask |= 1u << mapIndex;
        }

        // Selected lights keep their maps or take free ones; a light whose maps no longer fit in one run waits a frame.
        for (uint32_t selectedIndex = 0; selectedIndex < selected.size(); ++selectedIndex)
        {
            const Candidate& candidate = selected[selectedIndex];
            uint32_t firstMap = firstMaps[selectedIndex];
            if (firstMap == kInvalidIndex)
                firstMap = FindFreeMaps(allocation, candidate.m_faceCount);

            if (firstMap == kInvalidIndex)
                continue;

            for (uint32_t face = 0; face < candidate.m_faceCount; ++face)
            {
                MapSetup& map = allocation.m_maps[firstMap + face];
                const bool isSameLight = map.m_isUsed && map.m_light == candidate.m_light;
                const bool isUnchanged =
                    isSameLight && std::memcmp(&map.m_lightData, &candidate.m_data, sizeof(LocalLightData)) == 0;
                if (!isUnchanged)
                    resetMask |= 1u << (firstMap + face);

                map = CreateMapSetup(candidate, face);
            }
        }

        return resetMask;
    }


    LocalShadowMapData BuildMapData(const MapSetup& map, const uint32_t firstBlock)
    {
        LocalShadowMapData data{};
        data.m_viewProjection = map.m_viewProjection;
        const Vector3 position = map.m_camera.Translation();
        data.m_positionTexelScale = PackedVector4F(Vector4(position, map.m_texelScale));
        data.m_forward = PackedVector3F(Math::Rotate(Vector3::AxisZ(), map.m_camera.Rotation()));
        data.m_firstBlock = firstBlock;
        return data;
    }
} // namespace FE::Graphics::LocalShadows
