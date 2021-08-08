#include <FeCore/Modules/Environment.h>
#include <FeCore/Memory/Memory.h>
#include <gtest/gtest.h>
#include <FeCore/Console/FeLog.h>
#include <FeCore/Memory/Object.h>

int main(int argc, char** argv)
{
    FE::Env::CreateEnvironment();
    FE::GlobalAllocator<FE::HeapAllocator>::Init(FE::HeapAllocatorDesc{});
    int ret = 0;
    {
        auto logger = FE::MakeShared<FE::Debug::ConsoleLogger>();
        ::testing::FLAGS_gtest_print_utf8 = true;
        ::testing::InitGoogleTest(&argc, argv);
        ret = RUN_ALL_TESTS();
    }
    FE::GlobalAllocator<FE::HeapAllocator>::Destroy();
    return ret;
}
