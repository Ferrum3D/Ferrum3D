#include <FeCore/Containers/List.h>
#include <FeCore/Strings/String.h>
#include <Tests/Common/TestCommon.h>
#include <gtest/gtest.h>

using FE::List;

TEST(List, ConstructDestructObject)
{
    auto mock = std::make_shared<MockConstructors>();
    EXPECT_CALL(*mock, Construct()).Times(1);
    EXPECT_CALL(*mock, Destruct()).Times(1);
    EXPECT_CALL(*mock, Copy()).Times(0);
    EXPECT_CALL(*mock, Move()).Times(0);

    List<AllocateObject> lst;
    lst.Emplace(mock);
}

TEST(List, MoveConstruct)
{
    List<int> lst1 = { 0, 1, 2 };
    int* data      = lst1.Data();
    List<int> lst2 = std::move(lst1);
    EXPECT_TRUE(lst1.Empty());
    EXPECT_EQ(data, lst2.Data());
}

TEST(List, CopyConstruct)
{
    List<int> lst1 = { 0, 1, 2 };
    lst1.Reserve(32);
    List<int> lst2 = lst1;
    EXPECT_GT(lst1.Capacity(), lst2.Capacity());
    ASSERT_EQ(lst1.Size(), lst2.Size());
    for (FE::USize i = 0; i < lst1.Size(); ++i)
    {
        EXPECT_EQ(lst1[i], lst2[i]);
    }
}

TEST(List, NValuesConstruct)
{
    List<int> lst(5, 123);
    for (int i = 0; i < 5; ++i)
    {
        EXPECT_EQ(lst.Pop(), 123);
    }
}

TEST(List, InitializerListConstruct)
{
    List<int> lst = { 0, 1, 2, 3 };
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_EQ(lst[i], i);
    }
    auto lst1 = List<int>({ 0, 1, 2, 3 });
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_EQ(lst1[i], i);
    }
}

TEST(List, SizeCapacity)
{
    List<int> lst;
    EXPECT_TRUE(lst.Empty());
    for (int i = 0; i < 5; ++i)
    {
        lst.Push(i);
    }
    EXPECT_EQ(lst.Size(), 5);
    lst.Pop();
    EXPECT_EQ(lst.Size(), 4);
    EXPECT_LT(lst.Size(), lst.Capacity());
}

TEST(List, Indexing)
{
    List<FE::USize> lst = { 0, 0, 0 };
    lst[1]              = 1;
    lst[2]              = 2;
    ASSERT_EQ(lst.Size(), 3);
    for (FE::USize i = 0; i < lst.Size(); ++i)
    {
        EXPECT_EQ(lst[i], i);
    }
}

TEST(List, Assign)
{
    List<int> lst;
    lst.Assign({ 0, 1, 2, 3 });
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_EQ(lst[i], i);
    }
    lst.Assign(4, 123);
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_EQ(lst[i], 123);
    }
}

TEST(List, Append)
{
    List<const char*> lst1 = { "0", "1" };
    lst1.Append(2).Append("2").Append(3, "123");
    List<const char*> lst2 = { "0", "1", nullptr, nullptr, "2", "123", "123", "123" };
    ASSERT_EQ(lst1.Size(), lst2.Size());
    for (FE::USize i = 0; i < lst1.Size(); ++i)
    {
        EXPECT_EQ(FE::StringSlice(lst1[i]), FE::StringSlice(lst2[i]));
    }
}

TEST(List, Emplace)
{
    List<FE::String> lst;
    lst.Emplace(3, '*');
    EXPECT_EQ(lst.Pop(), "***");
}

TEST(List, PushPop)
{
    List<int> lst;
    lst.Push(0);
    lst.Push(1);
    lst.Push(2);
    EXPECT_EQ(lst.Pop(), 2);
    lst.Push(3);
    EXPECT_EQ(lst.Pop(), 3);
    EXPECT_EQ(lst.Pop(), 1);
    EXPECT_EQ(lst.Pop(), 0);
}

TEST(List, RangeForLoop)
{
    List<int> lst   = { 123, 123, 123 };
    FE::USize count = 0;
    for (auto& v : lst)
    {
        EXPECT_EQ(v, lst[count++]);
    }
    EXPECT_EQ(count, lst.Size());
}

TEST(List, MoveObject)
{
    List<FE::String> lst;
    FE::String s = "123";
    lst.Push(std::move(s));
    EXPECT_TRUE(s.Empty());
    EXPECT_EQ(lst.Pop(), "123");
}

TEST(List, Remove)
{
    auto allocatedBefore = FE::GlobalAllocator<FE::HeapAllocator>::Get().TotalAllocated();
    {
        auto lst = List<FE::String>{}.Append("1").Append("2").Append("3").Append("4").Append("5").Append("6");
        lst.Remove("2");
        EXPECT_EQ(lst.Size(), 5);
        EXPECT_EQ(lst[0], "1");
        EXPECT_EQ(lst[1], "3");
        EXPECT_EQ(lst[2], "4");
        EXPECT_EQ(lst[3], "5");
        EXPECT_EQ(lst[4], "6");

        lst.RemoveAt(0);
        EXPECT_EQ(lst.Size(), 4);
        EXPECT_EQ(lst[0], "3");
        EXPECT_EQ(lst[1], "4");
        EXPECT_EQ(lst[2], "5");
        EXPECT_EQ(lst[3], "6");

        lst.RemoveAt(3);
        EXPECT_EQ(lst.Size(), 3);
        EXPECT_EQ(lst[0], "3");
        EXPECT_EQ(lst[1], "4");
        EXPECT_EQ(lst[2], "5");

        lst.RemoveAt(1);
        EXPECT_EQ(lst.Size(), 2);
        EXPECT_EQ(lst[0], "3");
        EXPECT_EQ(lst[1], "5");
    }
    EXPECT_EQ(FE::GlobalAllocator<FE::HeapAllocator>::Get().TotalAllocated(), allocatedBefore);
}

TEST(List, SwapRemove)
{
    auto allocatedBefore = FE::GlobalAllocator<FE::HeapAllocator>::Get().TotalAllocated();
    {
        auto lst = List<FE::String>{}.Append("1").Append("2").Append("3").Append("4").Append("5").Append("6");
        lst.SwapRemove("2");
        EXPECT_EQ(lst.Size(), 5);
        EXPECT_EQ(lst[0], "1");
        EXPECT_EQ(lst[1], "6");
        EXPECT_EQ(lst[2], "3");
        EXPECT_EQ(lst[3], "4");
        EXPECT_EQ(lst[4], "5");

        lst.SwapRemoveAt(0);
        EXPECT_EQ(lst.Size(), 4);
        EXPECT_EQ(lst[0], "5");
        EXPECT_EQ(lst[1], "6");
        EXPECT_EQ(lst[2], "3");
        EXPECT_EQ(lst[3], "4");

        lst.SwapRemoveAt(3);
        EXPECT_EQ(lst.Size(), 3);
        EXPECT_EQ(lst[0], "5");
        EXPECT_EQ(lst[1], "6");
        EXPECT_EQ(lst[2], "3");

        lst.SwapRemoveAt(1);
        EXPECT_EQ(lst.Size(), 2);
        EXPECT_EQ(lst[0], "5");
        EXPECT_EQ(lst[1], "3");
    }
    EXPECT_EQ(FE::GlobalAllocator<FE::HeapAllocator>::Get().TotalAllocated(), allocatedBefore);
}

TEST(List, Reserve)
{
    List<int> lst;
    EXPECT_EQ(lst.Capacity(), 0);
    lst.Reserve(16);
    EXPECT_GE(lst.Capacity(), 16);
    int* data = lst.Data();
    for (int i = 0; i < 16; ++i)
    {
        lst.Push(i);
    }
    EXPECT_EQ(data, lst.Data());
}

TEST(List, Resize)
{
    List<int> lst = { 0, 1 };
    lst.Resize(3, 2);
    ASSERT_EQ(lst.Size(), 3);
    for (FE::USize i = 0; i < 3; ++i)
    {
        EXPECT_EQ(lst[i], i);
    }
    lst.Resize(1);
    ASSERT_EQ(lst.Size(), 1);
    ASSERT_EQ(lst.Front(), lst.Back());
    ASSERT_EQ(lst.Front(), 0);
}

TEST(List, Shrink)
{
    List<int> lst;
    lst.Reserve(64);
    auto cap = lst.Capacity();
    ASSERT_GE(cap, 64);
    lst.Push(1);
    lst.Shrink();
    ASSERT_LT(lst.Capacity(), cap);
    lst.RemoveBack();
    lst.Shrink();
    ASSERT_EQ(lst.Capacity(), 0);
}
