#include <FeCore/Containers/LRUCacheMap.h>
#include <FeCore/Strings/String.h>
#include <gtest/gtest.h>

using FE::LRUCacheMap;
using FE::String;

TEST(LRUCacheMap, AddValues)
{
    String value;
    LRUCacheMap<int, String> cache(3);
    cache.Emplace(0, "Zero");
    cache.Emplace(1, "One");
    cache.Emplace(2, "Two");

    EXPECT_TRUE(cache.TryGetValue(0, value));
    EXPECT_EQ(value, "Zero");

    cache.Emplace(3, "Three");

    EXPECT_TRUE(cache.TryGetValue(0, value));
    EXPECT_EQ(value, "Zero");
    EXPECT_FALSE(cache.TryGetValue(1, value));
    EXPECT_TRUE(cache.TryGetValue(2, value));
    EXPECT_EQ(value, "Two");
    EXPECT_TRUE(cache.TryGetValue(3, value));
    EXPECT_EQ(value, "Three");
}

TEST(LRUCacheMap, SetCapacity)
{
    LRUCacheMap<int, int> cache(1);
    cache.Emplace(0, 0);
    cache.Emplace(1, 1);
    EXPECT_FALSE(cache.HasKey(0));
    EXPECT_TRUE(cache.HasKey(1));
    cache.SetCapacity(4);
    for (int i = 0; i < 10; ++i)
    {
        cache.Emplace(i, i);
    }
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(cache.HasKey(i), i >= 6);
    }
    cache.SetCapacity(1);
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(cache.HasKey(i), i == 9);
    }
}

TEST(LRUCacheMap, Operator)
{
    LRUCacheMap<int, int> cache(1);
    cache.Emplace(0, 0);
    cache.Emplace(1, 1);
    auto r0 = cache[0];
    auto r1 = cache[1];
    EXPECT_FALSE(r0.IsOk());
    EXPECT_TRUE(r1.IsOk());
    EXPECT_EQ(r1.Unwrap(), 1);
}
