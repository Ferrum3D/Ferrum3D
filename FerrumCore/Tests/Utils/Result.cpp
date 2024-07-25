#include <FeCore/Utils/Result.h>
#include <Tests/Common/TestCommon.h>
#include <gtest/gtest.h>

enum class NumberError
{
    GreaterThan100,
    GreaterThan1000,
    TestError
};

auto TryNumber(FE::Int32 number) -> FE::Result<FE::Int32, NumberError>
{
    if (number > 1000)
    {
        return FE::Err(NumberError::GreaterThan1000);
    }

    if (number > 100)
    {
        return FE::Err(NumberError::GreaterThan100);
    }

    return number;
}

TEST(Result, OkValues)
{
    auto result = TryNumber(12);
    EXPECT_TRUE(result.IsOk());
    EXPECT_FALSE(result.IsErr());

    auto value = result.Unwrap();
    EXPECT_EQ(value, 12);

    if (auto r = TryNumber(0))
    {
        value = r.Unwrap();
    }

    EXPECT_EQ(value, 0);
}

TEST(Result, ErrValues)
{
    auto result = TryNumber(101);
    EXPECT_FALSE(result.IsOk());
    EXPECT_TRUE(result.IsErr());
    EXPECT_EQ(result.UnwrapErr(), NumberError::GreaterThan100);

    result = TryNumber(1001);
    EXPECT_FALSE(result.IsOk());
    EXPECT_TRUE(result.IsErr());
    EXPECT_EQ(result.UnwrapErr(), NumberError::GreaterThan1000);

    auto flag = false;
    if (auto r = TryNumber(101))
    {
        flag = true;
    }

    EXPECT_FALSE(flag);
}

TEST(Result, UnwrapOr)
{
    auto result = TryNumber(101);
    EXPECT_EQ(result.UnwrapOr(123), 123);
    EXPECT_EQ(result.UnwrapOrDefault(), 0);

    result = TryNumber(12);
    EXPECT_EQ(result.UnwrapOr(123), 12);
}

auto MockResult(const std::shared_ptr<MockConstructors>& mock, bool ok) -> FE::Result<AllocateObject, FE::Int32>
{
    if (ok)
    {
        return AllocateObject(mock);
    }

    return FE::Err(0);
}

TEST(Result, Contruct)
{
    auto mock = std::make_shared<MockConstructors>();
    EXPECT_CALL(*mock, Construct()).Times(1);
    EXPECT_CALL(*mock, Destruct()).Times(1);
    EXPECT_CALL(*mock, Copy()).Times(0);
    EXPECT_CALL(*mock, Move()).Times(1);

    auto result = MockResult(mock, true);
    EXPECT_TRUE(result.IsOk());
}
