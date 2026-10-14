#include "Reflection.gen.h"
#include <Core/Env/Environment.h>
#include <Core/Jobs/Jobs.h>
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    FE::Env::ApplicationInfo info;
    info.m_name = "FerrumGraphicsTests";
    FE::Env::Init(info, argc, const_cast<const char**>(argv));
    FE::CallLinkerAnchor_GraphicsTests();
    testing::InitGoogleTest(&argc, argv);

    int result = 0;
    FE::Jobs::DispatchMainThread([&result] {
        result = RUN_ALL_TESTS();
        FE::Jobs::StopJobSystem();
    });
    FE::Jobs::StartJobSystem();

    FE::Env::Shutdown();
    return result;
}
