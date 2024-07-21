#include <FeCore/Modules/Environment.h>
#include <FeCore/Memory/Memory.h>
#include <gtest/gtest.h>
#include <FeCore/Console/FeLog.h>

int main(int argc, char** argv)
{
    FE::Env::CreateEnvironment();
    int ret = 0;
    {
        FE::Rc logger = FE::Rc<FE::Debug::ConsoleLogger>::DefaultNew();
        ::testing::FLAGS_gtest_print_utf8 = true;
        ::testing::InitGoogleTest(&argc, argv);
        ret = RUN_ALL_TESTS();
    }
    return ret;
}
