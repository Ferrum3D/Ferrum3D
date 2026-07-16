#include <Core/Base/Platform.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Env/Environment.h>
#include <gtest/gtest.h>

using namespace FE;

int main(int argc, char** argv)
{
    testing::FLAGS_gtest_print_utf8 = true;

    Env::ApplicationInfo appInfo;
    appInfo.m_name = "FerrumCoreTests";
    Env::Init(appInfo, argc, const_cast<const char**>(argv));

    if (Platform::IsDebuggerPresent())
    {
        testing::FLAGS_gtest_break_on_failure = true;
        testing::FLAGS_gtest_catch_exceptions = false;
    }

    testing::InitGoogleTest(&argc, argv);

    int32_t exitCode = 0;
    Jobs::DispatchMainThread([&exitCode] {
        exitCode = RUN_ALL_TESTS();
        Jobs::StopJobSystem();
    });
    Jobs::StartJobSystem();

    return exitCode;
}
