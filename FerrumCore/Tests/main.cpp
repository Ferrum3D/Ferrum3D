#include <FeCore/Logging/Trace.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Modules/Environment.h>
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    FE::Env::CreateEnvironment();
    int ret = 0;
    {
        ::testing::FLAGS_gtest_print_utf8 = true;
        ::testing::InitGoogleTest(&argc, argv);
        ret = RUN_ALL_TESTS();
    }
    return ret;
}
