#pragma once
#include <FeCore/Base/BaseTypes.h>

namespace FE::Graphics
{
    enum class AssetLoadingStatus : uint32_t
    {
        kNone,
        kHasLoadedLods,
        kCompletelyLoaded,
        kFailed,
    };


    struct Asset : public Memory::RefCountedObjectBase
    {
        ~Asset() override
        {
            FE_Assert(m_completionWaitGroup == nullptr || m_completionWaitGroup->IsSignaled(),
                      "Cannot destroy asset while it is being loaded");
        }

        Env::Name m_name;
        std::atomic<AssetLoadingStatus> m_status = AssetLoadingStatus::kNone;
        Rc<WaitGroup> m_completionWaitGroup;
    };
} // namespace FE::Graphics
