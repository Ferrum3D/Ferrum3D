#include <Core/Math/Packing.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Features/Lighting/HosekSky.h>
#include <Graphics/Features/Lighting/LocalLightingSceneModule.h>
#include <Graphics/Passes/DeferredShadingPass.h>
#include <Graphics/Passes/ForwardTransparencyPass.h>
#include <Graphics/RendererImpl.h>
#include <Graphics/Scene/RenderViewData.h>
#include <Graphics/Scene/View.h>
#include <cmath>
#include <cstring>
#include <limits>

namespace FE::Graphics
{
    namespace
    {
        FE_COMPUTE_PIPELINE_SET(CullLocalLightsPipeline, "Shaders/Lighting/CullLocalLights.cs.hlsl");
    } // namespace


    bool LocalLightParameters::Validate() const
    {
        if (!std::isfinite(m_intensity) || m_intensity < 0 || !std::isfinite(m_range) || m_range <= 0)
            return false;

        if (m_unit != LightUnit::kCandela && m_unit != LightUnit::kLumen)
            return false;

        if (m_colorMode == LightColorMode::kTemperature)
            return std::isfinite(m_temperature) && m_temperature >= 1000 && m_temperature <= 40000;

        if (m_colorMode != LightColorMode::kRGB)
            return false;

        return std::isfinite(m_color.x) && m_color.x >= 0 && std::isfinite(m_color.y) && m_color.y >= 0
            && std::isfinite(m_color.z) && m_color.z >= 0;
    }


    PackedVector3F LocalLightParameters::GetLinearColor() const
    {
        if (m_colorMode == LightColorMode::kRGB)
            return m_color;

        return BlackbodyLinearColor(m_temperature);
    }


    float LocalLightParameters::GetCandela(const float innerAngle, const float outerAngle) const
    {
        if (m_unit == LightUnit::kCandela)
            return m_intensity;

        // Angular attenuation is smoothstep(cosOuter, cosInner, cosTheta); its integral is half the ramp width.
        const float solidAngle = 2 * Constants::kPI * (1 - 0.5f * (std::cos(innerAngle) + std::cos(outerAngle)));
        return m_intensity / Math::Max(solidAngle, 0.000001f);
    }


    LocalLightData PackLocalLightData(const Vector3 position, const float range, const Vector3 color, const float candela,
                                      const Vector3 direction, const float innerAngle, const float outerAngle)
    {
        LocalLightData data{};
        data.m_positionRange = PackedVector4F(Vector4(position, range));
        const float peak = Math::Max(color.x, Math::Max(color.y, color.z));
        if (peak > 0 && candela > 0)
        {
            // Use the shared unsigned-float exponent range to retain dim channels in high-contrast colors.
            constexpr float kColorScale = 32768.0f;
            data.m_color = Math::Pack::RGB32FloatToR11G11B10Float((color / peak) * kColorScale);
            const double multiplier = static_cast<double>(peak) * candela / kColorScale;
            data.m_colorMultiplier =
                static_cast<float>(Math::Min(multiplier, static_cast<double>(std::numeric_limits<float>::max())));
        }

        if (outerAngle >= Constants::kPI * 0.5f)
        {
            data.m_coneAngles = kLocalPointLightCone;
        }
        else
        {
            data.m_direction = Math::Pack::NormalToOctahedral16Snorm(direction);
            const uint32_t inner =
                static_cast<uint32_t>(Math::Clamp(innerAngle * kLocalLightConeAngleScale + 0.5f, 0.0f, 65534.0f));
            const uint32_t outer =
                static_cast<uint32_t>(Math::Clamp(outerAngle * kLocalLightConeAngleScale + 0.5f, 0.0f, 65534.0f));
            data.m_coneAngles = inner | (outer << 16);
        }

        return data;
    }


    LocalLightingSceneModule::LocalLightingSceneModule(Scene* scene)
        : LocalLightingSceneModule(scene, Rtti::AssertCast<RendererImpl*>(scene->GetRenderer())->GetDatabase())
    {
    }


    LocalLightingSceneModule::LocalLightingSceneModule(Scene* scene, DB::Database* database)
        : SceneModuleBase(scene)
        , m_table(Memory::DefaultNew<LocalLightTable>(database))
    {
    }


    LightHandle LocalLightingSceneModule::CreateLight(const LocalLightData& light)
    {
        const auto row = m_table->AllocateRow();
        const uint32_t index = row.m_rowIndex;
        if (index >= m_slots.size())
            m_slots.resize(index + 1);

        Slot& slot = m_slots[index];
        FE_Assert(!slot.m_active);
        slot.m_active = true;
        m_slotCount = Math::Max(m_slotCount, index + 1);
        m_table->WriteRow(row).m_light.Get() = light;
        return { index, slot.m_generation };
    }


    const LocalLightData* LocalLightingSceneModule::FindLight(const LightHandle handle) const
    {
        if (handle.m_value >= m_slots.size())
            return nullptr;

        const Slot& slot = m_slots[handle.m_value];
        if (!slot.m_active || slot.m_generation != handle.m_generation)
            return nullptr;

        return &m_table->ReadRow(handle.m_value).m_light.Get();
    }


    void LocalLightingSceneModule::DestroyLight(const LightHandle handle)
    {
        if (FindLight(handle) == nullptr)
            return;

        m_table->WriteRow(handle.m_value).m_light.Get() = {};
        m_table->Free(DB::Ref<LocalLightTable>{ handle.m_value });
        Slot& slot = m_slots[handle.m_value];
        slot.m_active = false;
        ++slot.m_generation;
        while (m_slotCount != 0 && !m_slots[m_slotCount - 1].m_active)
            --m_slotCount;
    }


    void LocalLightingSceneModule::UpdateLight(const LightHandle handle, const LocalLightData& light)
    {
        const LocalLightData* current = FindLight(handle);
        if (current == nullptr || std::memcmp(current, &light, sizeof(light)) == 0)
            return;

        m_table->WriteRow(handle.m_value).m_light.Get() = light;
    }


    void LocalLightingSceneModule::PrepareRenderView(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard,
                                                     Core::RingUploader& uploader)
    {
        const bool isLit =
            blackboard.Contains<DeferredShading::ViewModule*>() || blackboard.Contains<ForwardTransparency::ViewModule*>();
        if (!isLit || m_slotCount == 0)
            return;

        FE_FG_SCOPE(graph, "ClusterLocalLights");
        const auto& view = blackboard.Get<RendererViewData>();
        LightClusterData data{};
        data.m_view = view.m_view->GetViewMatrix();
        data.m_projection = view.m_view->GetProjectionMatrix();
        data.m_inverseViewProjection = view.m_view->GetInverseViewProjectionMatrix();
        data.m_viewport = blackboard.Get<ViewData>().m_viewport;
        const float nearPlane = Math::Max(view.m_view->GetNearPlane(), 0.001f);
        const float farPlane = Math::Max(view.m_view->GetFarPlane(), nearPlane + 0.001f);
        data.m_depth = { nearPlane,
                         farPlane,
                         kLightClusterSlices / std::log2(farPlane / nearPlane),
                         view.m_view->IsOrthographic() ? 1.0f : 0.0f };
        data.m_tiles =
            Math::CeilDivide(Vector2UInt(static_cast<uint32_t>(data.m_viewport.z), static_cast<uint32_t>(data.m_viewport.w)),
                             kLightClusterTileSize);
        data.m_lightCount = m_slotCount;
        data.m_wordCount = Math::CeilDivide(data.m_lightCount, 32u);
        auto clusters = Core::Buffer::CreateStructured<LightClusterData>(graph.GetDevice(), "LightClusters", 1);
        auto masks =
            Core::Buffer::CreateStructured<uint32_t>(graph.GetDevice(),
                                                     "LightClusterMasks",
                                                     data.m_tiles.x * data.m_tiles.y * kLightClusterSlices * data.m_wordCount);
        FE_Verify(uploader.Upload(graph, clusters.Get(), data));

        auto& binding = blackboard.Add<LocalLightingBinding>();
        binding.m_lights = m_table->GetDeviceAddress();
        binding.m_clusters = graph.GetSRV(clusters.Get());
        binding.m_masks = graph.GetSRV(masks.Get());
        binding.m_lightCount = data.m_lightCount;
        auto* pass = graph.AllocatePassData<CullLocalLightsPassDesc>();
        pass->m_pipeline = CullLocalLightsPipeline::GetPipeline();
        pass->m_constants.m_lighting = binding;
        pass->m_constants.m_lighting.m_masks = {};
        pass->m_constants.m_masks = graph.GetUAV(masks.Get());
        graph.AddDispatchPass("CullLocalLights", pass, Vector3UInt(data.m_tiles.x, data.m_tiles.y, kLightClusterSlices));
    }


    void LocalLightingSceneModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }
} // namespace FE::Graphics
