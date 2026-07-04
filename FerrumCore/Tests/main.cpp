#include <Core/Base/Platform.h>
#include <Core/DI/Builder.h>
#include <Core/Modules/Configuration.h>
#include <Core/Modules/Environment.h>
#include <gtest/gtest.h>

using namespace FE;

int main(int argc, char** argv)
{
    testing::FLAGS_gtest_print_utf8 = true;

    Env::ApplicationInfo appInfo;
    appInfo.m_name = "FerrumCoreTests";
    Env::Init(appInfo);

    DI::ServiceRegistryBuilder builder{ Env::GetRootServiceRegistry() };
    builder.Bind<Env::Configuration>()
        .ToFunc([](DI::IServiceProvider*, Memory::RefCountedObjectBase** result) {
            *result = Memory::DefaultNew<Env::Configuration>(festd::span<const festd::string_view>{});
            return DI::ResultCode::kSuccess;
        })
        .InSingletonScope();
    builder.Build();

    if (Platform::IsDebuggerPresent())
    {
        testing::FLAGS_gtest_break_on_failure = true;
        testing::FLAGS_gtest_catch_exceptions = false;
    }

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
