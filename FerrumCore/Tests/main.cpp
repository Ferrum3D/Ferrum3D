#include <FeCore/Modules/Environment.h>
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    FE::Env::CreateEnvironment();
    testing::FLAGS_gtest_print_utf8 = true;

    if (FE::Env::IsDebuggerPresent())
    {
        testing::FLAGS_gtest_break_on_failure = true;
        testing::FLAGS_gtest_catch_exceptions = false;
    }

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
