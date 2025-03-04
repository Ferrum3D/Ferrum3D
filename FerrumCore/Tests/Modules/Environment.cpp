#include <FeCore/Modules/Environment.h>
#include <FeCore/Strings/Format.h>
#include <Tests/Common/TestCommon.h>
#include <festd/vector.h>

using ::testing::AtLeast;

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

TEST(EnvironmentTest, EnvNamePages)
{
    const char* longStr = "long_long_long_long";
    festd::vector<Env::Name> names;
    for (uint32_t i = 0; i < 64 * 1024; ++i)
    {
        const festd::string s = Fmt::Format("{}{}{}{}", longStr, longStr, longStr, i);
        const Env::Name name{ std::string_view{ s.data(), s.size() } };
        ASSERT_TRUE(name.Valid());
        names.push_back(name);
    }

    for (uint32_t i = 0; i < 64 * 1024; ++i)
    {
        const festd::string s = Fmt::Format("{}{}{}{}", longStr, longStr, longStr, i);
        const Env::Name name{ std::string_view{ s.data(), s.size() } };
        ASSERT_TRUE(name.Valid());
        EXPECT_EQ(s, name);
        EXPECT_EQ(name, names[i]);
        EXPECT_EQ(name.GetRecord(), names[i].GetRecord());
    }
}

TEST(EnvironmentTest, CreateVar)
{
    auto intVar = Env::CreateGlobalVariable<int>(Env::Name{ "CreateVar" }, 123);
    ASSERT_EQ(*intVar, 123);
    ASSERT_EQ(intVar.GetName(), festd::string_view{ "CreateVar" });
}

TEST(EnvironmentTest, SharedVar)
{
    auto instance1 = Env::CreateGlobalVariable<int>(Env::Name{ "SharedVar" }, 123);
    auto instance2 = instance1;
    ASSERT_EQ(*instance1, 123);

    ASSERT_EQ(instance1, instance2);
    ASSERT_EQ(*instance1, *instance2);

    ASSERT_EQ(instance1.GetName(), festd::string_view{ "SharedVar" });
    ASSERT_EQ(instance2.GetName(), festd::string_view{ "SharedVar" });
}

TEST(EnvironmentTest, FindVar)
{
    auto owner1 = Env::CreateGlobalVariable<int>(Env::Name{ "FindVar" }, 123);
    /* deleted var*/ Env::CreateGlobalVariable<int>(Env::Name{ "Removed" }, 0);

    auto var123 = Env::FindGlobalVariable<int>(Env::Name{ "FindVar" });
    ASSERT_TRUE(var123);
    ASSERT_FALSE(Env::FindGlobalVariable<int>(Env::Name{ "Removed" }));

    ASSERT_EQ(*var123, 123);

    EXPECT_EQ(var123.GetName(), festd::string_view{ "FindVar" });
}

TEST(EnvironmentTest, NoCopies)
{
    auto mock = std::make_shared<MockConstructors>();
    EXPECT_CALL(*mock, Construct()).Times(1);
    EXPECT_CALL(*mock, Destruct()).Times(1);
    EXPECT_CALL(*mock, Copy()).Times(0);
    EXPECT_CALL(*mock, Move()).Times(0);

    {
        auto handle1 = Env::CreateGlobalVariable<AllocateObject>(Env::Name{ "NoCopies" }, mock);
        auto handle2 = Env::FindGlobalVariable<AllocateObject>(Env::Name{ "NoCopies" });

        ASSERT_EQ(&*handle1, &*handle2);
    }
}
