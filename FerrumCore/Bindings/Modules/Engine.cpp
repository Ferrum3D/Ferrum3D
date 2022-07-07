#include <FeCore/Modules/Environment.h>
#include <FeCore/Memory/Memory.h>

namespace FE
{
    extern "C"
    {
        FE_DLL_EXPORT void Engine_Construct()
        {
            FE::Env::CreateEnvironment();
            FE::GlobalAllocator<FE::HeapAllocator>::Init(FE::HeapAllocatorDesc());
        }

        FE_DLL_EXPORT void Engine_Destruct()
        {
            FE::GlobalAllocator<FE::HeapAllocator>::Destroy();
        }

        FE_DLL_EXPORT Env::Internal::IEnvironment* Engine_GetEnvironment()
        {
            return &Env::GetEnvironment();
        }
    }
}
