#include <FeCore/Modules/Environment.h>
#include <Tests/Common/TestCommon.h>

using ::testing::AtLeast;

TEST(EnvironmentTest, CreateVar)
{
    auto intVar = FE::Env::CreateGlobalVariable<int>("CreateVar", 123);
    ASSERT_EQ(*intVar, 123);
    ASSERT_EQ(intVar.GetName(), "CreateVar");
}

TEST(EnvironmentTest, SharedVar)
{
    auto instance1 = FE::Env::CreateGlobalVariable<int>("SharedVar", 123);
    auto instance2 = instance1;
    ASSERT_EQ(*instance1, 123);

    ASSERT_EQ(instance1, instance2);
    ASSERT_EQ(*instance1, *instance2);

    ASSERT_EQ(instance1.GetName(), "SharedVar");
    ASSERT_EQ(instance2.GetName(), "SharedVar");
}

TEST(EnvironmentTest, FindVar)
{
    auto owner1 = FE::Env::CreateGlobalVariable<int>("FindVar", 123);
    /* deleted var*/ FE::Env::CreateGlobalVariable<int>("Removed", 0);

    auto var123 = FE::Env::FindGlobalVariable<int>("FindVar").Unwrap();
    ASSERT_FALSE(FE::Env::FindGlobalVariable<int>("Removed").IsOk());

    ASSERT_EQ(*var123, 123);

    EXPECT_EQ(var123.GetName(), "FindVar");
}

TEST(EnvironmentTest, NoCopies)
{
    auto mock = std::make_shared<MockConstructors>();
    EXPECT_CALL(*mock, Construct()).Times(1);
    EXPECT_CALL(*mock, Destruct()).Times(1);
    EXPECT_CALL(*mock, Copy()).Times(0);
    EXPECT_CALL(*mock, Move()).Times(0);

    {
        auto handle1 = FE::Env::CreateGlobalVariable<AllocateObject>("NoCopies", mock);
        auto handle2 = FE::Env::FindGlobalVariable<AllocateObject>("NoCopies").Unwrap();

        ASSERT_EQ(&*handle1, &*handle2);
    }
}
