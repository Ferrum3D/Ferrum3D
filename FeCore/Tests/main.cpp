#include <FeCore/Modules/Environment.h>
#include <FeCore/Memory/Memory.h>
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    FE::Env::CreateEnvironment();
    FE::GlobalAllocator<FE::HeapAllocator>::Init(FE::HeapAllocatorDesc{});
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::FLAGS_gtest_print_utf8 = true;
    int ret = RUN_ALL_TESTS();
    FE::GlobalAllocator<FE::HeapAllocator>::Destroy();
    return ret;
}
