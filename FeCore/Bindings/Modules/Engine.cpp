#include <FeCore/Modules/Environment.h>
#include <FeCore/Memory/Memory.h>

namespace FE
{
    extern "C"
    {
        FE_DLL_EXPORT void InitEngine()
        {
            FE::Env::CreateEnvironment();
            FE::GlobalAllocator<FE::HeapAllocator>::Init(FE::HeapAllocatorDesc());
        }

        FE_DLL_EXPORT void DeinitEngine()
        {
            FE::GlobalAllocator<FE::HeapAllocator>::Destroy();
        }
    }
}
