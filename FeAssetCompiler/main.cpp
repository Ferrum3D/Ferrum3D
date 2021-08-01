#include <FeCore/Console/FeLog.h>
#include <FeCore/Memory/Memory.h>
#include <array>
#include <iostream>

int main()
{
    FE::Env::CreateEnvironment();
    FE::GlobalAllocator<FE::HeapAllocator>::Init(FE::HeapAllocatorDesc{});
    {
        auto logger = FE::MakeShared<FE::Debug::ConsoleLogger>();
        FE_LOG_MESSAGE("Hello, World");
    }
    FE::GlobalAllocator<FE::HeapAllocator>::Destroy();
    FE::Env::DetachEnvironment();
}
