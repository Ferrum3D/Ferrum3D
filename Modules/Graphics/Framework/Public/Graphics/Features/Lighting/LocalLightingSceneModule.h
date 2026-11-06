#pragma once
#include <Graphics/Scene/Scene.h>
#include <Graphics/Tables/LocalLightTable.h>
#include <Shaders/Lighting/LocalLighting.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    enum class LightUnit : uint32_t
    {
        kCandela,
        kLumen,
    };


    enum class LightColorMode : uint32_t
    {
        kRGB,
        kTemperature,
    };


    struct LocalLightParameters
    {
        FE_RTTI_Reflect("908BFF07-CD19-4EC3-8BF0-AC05B59C43AE");
        FE_RTTI_Serialize();

        LightUnit m_unit = LightUnit::kLumen;
        LightColorMode m_colorMode = LightColorMode::kTemperature;
        float m_intensity = 1500.0f;
        float m_temperature = 6500.0f;
        PackedVector3F m_color{ 1.0f, 1.0f, 1.0f };
        float m_range = 10.0f;
        //! Opt-in, since every shadowed light takes shadow map pages and rendering time; see LocalShadows::kMaxMapCount.
        bool m_castsShadows = false;

        [[nodiscard]] bool Validate() const;
        [[nodiscard]] PackedVector3F GetLinearColor() const;
        [[nodiscard]] float GetCandela(float innerAngle = Constants::kPI, float outerAngle = Constants::kPI) const;
    };


    [[nodiscard]] LocalLightData PackLocalLightData(Vector3 position, float range, Vector3 color, float candela,
                                                    Vector3 direction = Vector3(0, 0, 1), float innerAngle = Constants::kPI,
                                                    float outerAngle = Constants::kPI);


    struct LightHandle final
    {
        uint32_t m_value = kInvalidIndex;
        uint32_t m_generation = 0;

        [[nodiscard]] bool IsValid() const
        {
            return m_value != kInvalidIndex;
        }

        friend bool operator==(const LightHandle&, const LightHandle&) = default;
    };


    struct LocalLightingSceneModule final : public SceneModuleBase
    {
        FE_RTTI("ABC6D5C6-93A2-429F-8FBB-C943223D63D0");

        explicit LocalLightingSceneModule(Scene* scene);
        LocalLightingSceneModule(Scene* scene, DB::Database* database);
        [[nodiscard]] LightHandle CreateLight(const LocalLightData& light = {});
        void DestroyLight(LightHandle handle);
        void UpdateLight(LightHandle handle, const LocalLightData& light);
        [[nodiscard]] const LocalLightData* FindLight(LightHandle handle) const;
        void SetLightCastsShadows(LightHandle handle, bool castsShadows);

        //! Call function(LightHandle, const LocalLightData&) for every light that casts shadows.
        template<class TFunction>
        void ForEachShadowCaster(const TFunction& function) const
        {
            for (uint32_t index = 0; index < m_slotCount; ++index)
            {
                const Slot& slot = m_slots[index];
                if (slot.m_active && slot.m_castsShadows)
                    function(LightHandle{ index, slot.m_generation }, slot.m_data);
            }
        }

        [[nodiscard]] uint32_t GetLightSlotCount() const
        {
            return m_slotCount;
        }

        [[nodiscard]] LocalLightTable* GetLightTable() const
        {
            return m_table.Get();
        }

        void PrepareRenderView(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard,
                               Core::RingUploader& uploader) override;

    private:
        void DoRelease() override;

        void WriteLightRow(uint32_t rowIndex, const LocalLightData& light);

        struct Slot final
        {
            //! Host copy of the columns of the row of this slot.
            LocalLightData m_data{};
            uint32_t m_generation = 1;
            bool m_active = false;
            bool m_castsShadows = false;
        };

        Rc<LocalLightTable> m_table;
        festd::vector<Slot> m_slots;
        uint32_t m_slotCount = 0;
    };
} // namespace FE::Graphics

FE_RTTI_Reflect(FE::Graphics::LightUnit, "63722280-197B-465C-8A61-D666CCA9FB0B");
FE_RTTI_Reflect(FE::Graphics::LightColorMode, "0E16B434-FAFF-45DD-9C4E-297083475F73");
