#include <FeCore/Console/FeLog.h>
#include <FeCore/Memory/Memory.h>

int main()
{
    FE::Env::CreateEnvironment();
    FE::GlobalAllocator<FE::HeapAllocator>::Init(FE::HeapAllocatorDesc{});
    auto logger = FE::MakeUnique<FE::Debug::ConsoleLogger>();
    FE::String str = "loooooooooooooooooooooooooong";
    for (int i = 0; i < 16; ++i)
    {
        FE_LOG_MESSAGE("{}", FE::GlobalAllocator<FE::HeapAllocator>::Get().TotalAllocated());
        FE_LOG_MESSAGE("{}", str.Capacity());
    }
}
