#pragma once
#include <Core/IO/Assets.h>
#include <Core/Math/Vector2.h>
#include <Core/Utils/Icons.h>

namespace FE::Input
{
    enum class ActionState : uint32_t
    {
        kStarted,
        kInProgress,
        kTriggered,
        kCompleted,
        kCanceled,
    };


    enum class ActionType : uint32_t
    {
        kBoolean,
        kScalar,
        kVector2,
    };


    //! State belongs to one stack/action pair, never to the shared processor asset.
    struct ActionProcessorContext final
    {
        bool m_active = false;
        bool m_pressed = false;
        bool m_wasActive = false;
        bool m_completed = false;
        float m_deltaSeconds = 0;
        float m_heldSeconds = 0;
    };


    struct ActionProcessor : public Memory::RefCountedObjectBase
    {
        FE_RTTI("882596AA-9BD0-475D-9CB2-3636FF35D843");

        virtual ActionState Process(ActionProcessorContext& context) const = 0;

        [[nodiscard]] virtual bool Validate() const
        {
            return true;
        }
    };


    struct HoldDurationActionProcessor final : public ActionProcessor
    {
        FE_RTTI("CEA014F9-2835-4A7B-B15F-EAF0D0CE97CE");
        FE_RTTI_Reflect();
        FE_RTTI_Serialize();

        float m_holdTime = 1;

        ActionState Process(ActionProcessorContext& context) const override;

        [[nodiscard]] bool Validate() const override
        {
            return m_holdTime >= 0;
        }

    protected:
        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }
    };


    struct ContinuousActionProcessor final : public ActionProcessor
    {
        FE_RTTI("D93D50DB-32B9-42B4-A49B-3C0AD6E7985A");
        FE_RTTI_Serialize();

        ActionState Process(ActionProcessorContext& context) const override;

    protected:
        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }
    };


    struct FE_META(Rtti::Icon(FE_ICON_INPUT)) Action final
    {
        FE_RTTI_Reflect("E6382DFF-E9F2-42F7-8FF7-C044F968AAA7");
        FE_RTTI_Serialize();

        ActionType m_type = ActionType::kBoolean;
        Rc<ActionProcessor> m_processor;

        [[nodiscard]] bool Validate() const;
    };


    struct ActionEvent final
    {
        IO::AssetID m_action;
        ActionState m_state;
        Vector2 m_value;
    };


    struct ResolvedAction final
    {
        IO::AssetID m_action = IO::AssetID::kNull;
        ActionType m_type = ActionType::kBoolean;
        ActionState m_state = ActionState::kCanceled;
        Vector2 m_value{ 0, 0 };
        bool m_hasEvent = false;
        bool m_active = false;
    };
} // namespace FE::Input
