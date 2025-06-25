#include <FeCore/Modules/Environment.h>
#include <FeCore/Strings/Format.h>
#include <Tests/Common/TestCommon.h>
#include <festd/vector.h>

using namespace FE;

TEST(EnvironmentTest, EnvName)
{
    const Env::Name empty;
    ASSERT_FALSE(empty);
    EXPECT_EQ(empty.size(), 0);
    EXPECT_EQ(empty, festd::string_view{ "" });
    EXPECT_EQ(empty, Env::Name{});
    EXPECT_EQ(empty.c_str(), nullptr);
    EXPECT_EQ(empty.GetRecord(), nullptr);

    const Env::Name test1{ std::string_view{ "name123" } };
    const Env::Name test2{ std::string_view{ "name124" } };
    const Env::Name test3{ std::string_view{ "name123" } };
    const Env::Name test4{ test1 };
    EXPECT_EQ(test1.GetRecord(), test3.GetRecord());
    EXPECT_EQ(test1.GetRecord(), test4.GetRecord());
    EXPECT_EQ(test1.c_str(), test3.c_str());
    EXPECT_EQ(test1.c_str(), test4.c_str());
    EXPECT_EQ(test1, test3);
    EXPECT_EQ(test1, test4);
    EXPECT_EQ(test1.size(), 7);
    EXPECT_EQ(test2, festd::string_view{ "name124" });
    EXPECT_EQ(test2.size(), 7);
    EXPECT_EQ(test3, festd::string_view{ "name123" });
    EXPECT_EQ(test3.size(), 7);
    EXPECT_EQ(test1, festd::string_view{ "name123" });
    EXPECT_EQ(test4.size(), 7);
    EXPECT_EQ(test4, festd::string_view{ "name123" });
}
