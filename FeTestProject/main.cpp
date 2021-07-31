#include <FeCore/Console/FeLog.h>
#include <FeCore/Jobs/JobSystem.h>
#include <FeCore/Math/Vector3.h>
#include <FeCore/Time/DateTime.h>
#include <FeCore/Utils/Result.h>
#include <array>
#include <iostream>
#include <FeCore/FeCore/Strings/String.h>
#include <immintrin.h>
#include <FeCore/Memory/Allocator.h>
#include <FeCore/Memory/HeapAllocator.h>

int main()
{
    FE::InitLogger();
    FE::Env::CreateEnvironment();
    FE::GlobalAllocator<FE::HeapAllocator>::Init(FE::HeapAllocatorDesc{});
    FE::String str = "loooooooooooooooooooooooooong";
    FE::LogMsg("{}", FE::GlobalAllocator<FE::HeapAllocator>::Get().TotalAllocated());
    FE::LogMsg("{}", str.Capacity());
}
