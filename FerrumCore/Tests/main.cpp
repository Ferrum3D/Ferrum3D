#include <FeCore/Base/Platform.h>
#include <FeCore/Modules/Environment.h>
#include <gtest/gtest.h>

using namespace FE;

int main(int argc, char** argv)
{
    Env::ApplicationInfo appInfo;
    appInfo.m_name = "FerrumCoreTests";
    Env::Init(appInfo);

    testing::FLAGS_gtest_print_utf8 = true;

    if (Platform::IsDebuggerPresent())
    {
        testing::FLAGS_gtest_break_on_failure = true;
        testing::FLAGS_gtest_catch_exceptions = false;
    }

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
