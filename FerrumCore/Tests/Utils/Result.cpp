#include <FeCore/Utils/ResultExp.h>
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
        return FE::Result<FE::Int32, NumberError>::Err(NumberError::GreaterThan1000);
    }

    if (number > 100)
    {
        return FE::Result<FE::Int32, NumberError>::Err(NumberError::GreaterThan100);
    }

    return FE::Result<FE::Int32, NumberError>::Ok(number);
}

FE::Int32 Increment(FE::Int32 value)
{
    return value + 1;
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

TEST(Result, OkMap)
{
    auto result    = TryNumber(12);
    auto mapResult = result.Map(&Increment);

    EXPECT_EQ(mapResult.Unwrap(), 13);
}

TEST(Result, ErrMap)
{
    auto result    = TryNumber(101);
    auto mapResult = result.Map(&Increment);
    EXPECT_FALSE(mapResult.IsOk());
    EXPECT_EQ(mapResult.UnwrapErr(), NumberError::GreaterThan100);
}

TEST(Result, ThenElseOk)
{
    auto result = TryNumber(12)
                      .AndThen([](FE::Int32 number) {
                          return number * 2;
                      })
                      .AndThen(&Increment)
                      .OrElse([](NumberError error) {
                          EXPECT_TRUE(false);
                          return error;
                      });

    EXPECT_EQ(result.Unwrap(), 12 * 2 + 1);

    result = TryNumber(12);
    result = result.AndThen([](FE::Int32 number) {
        return number * 2;
    });
    result = result.AndThen(&Increment);
    result = result.OrElse([](NumberError error) {
        EXPECT_TRUE(false);
        return error;
    });

    EXPECT_EQ(result.Unwrap(), 12 * 2 + 1);
}

TEST(Result, ThenElseErr)
{
    auto result = TryNumber(101)
                      .AndThen([](FE::Int32 number) {
                          EXPECT_TRUE(false);
                          return number * 2;
                      })
                      .AndThen(&Increment)
                      .OrElse([](NumberError) {
                          return NumberError::TestError;
                      });

    EXPECT_EQ(result.UnwrapErr(), NumberError::TestError);

    result = TryNumber(101);
    result = result.AndThen([](FE::Int32 number) {
        EXPECT_TRUE(false);
        return number * 2;
    });
    result = result.AndThen(&Increment);
    result = result.OrElse([](NumberError) {
        return NumberError::TestError;
    });

    EXPECT_EQ(result.UnwrapErr(), NumberError::TestError);
}

auto MockResult(const std::shared_ptr<MockConstructors>& mock, bool ok) -> FE::Result<AllocateObject, FE::Int32>
{
    if (ok)
    {
        return FE::Result<AllocateObject, FE::Int32>::Ok(AllocateObject(mock));
    }

    return FE::Result<AllocateObject, FE::Int32>::Err(0);
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

TEST(Result, MoveSemantic)
{
    auto mock = std::make_shared<MockConstructors>();
    EXPECT_CALL(*mock, Construct()).Times(1);
    EXPECT_CALL(*mock, Destruct()).Times(1);
    EXPECT_CALL(*mock, Copy()).Times(0);
    EXPECT_CALL(*mock, Move()).Times(11);

    auto result = MockResult(mock, true)
                      .Map([](auto x) {
                          return x;
                      })
                      .AndThen([](auto x) {
                          return x;
                      })
                      .OrElse([](auto x) {
                          return x;
                      });
    EXPECT_TRUE(result.IsOk());
}
