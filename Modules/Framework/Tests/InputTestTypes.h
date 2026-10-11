#pragma once
#include <Framework/Input/Action.h>

namespace FE::Input::Tests
{
    struct OffsetBase
    {
        virtual ~OffsetBase() = default;
        uint64_t m_padding = 0;
    };


    struct OffsetProcessor final
        : OffsetBase
        , ActionProcessor
    {
        FE_RTTI("3D1660FD-9772-44D7-8875-D0B7ECFC4F78");
        FE_RTTI_Reflect();
        FE_RTTI_Serialize();

        uint32_t m_value = 123;
        ActionState Process(ActionProcessorContext&) const override
        {
            return ActionState::kFinished;
        }

    protected:
        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }
    };
} // namespace FE::Input::Tests
