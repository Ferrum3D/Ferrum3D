#pragma once
#include <FeCore/Base/Base.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

struct MockConstructors
{
    MOCK_METHOD(void, Construct, (), (noexcept));
    MOCK_METHOD(void, Destruct, (), (noexcept));
    MOCK_METHOD(void, Copy, (), (noexcept));
    MOCK_METHOD(void, Move, (), (noexcept));
};

struct AllocateObject
{
    std::shared_ptr<MockConstructors> m_Mock;

    AllocateObject(std::shared_ptr<MockConstructors> mock) noexcept
    {
        m_Mock = std::move(mock);
        m_Mock->Construct();
    }

    AllocateObject(const AllocateObject& other) noexcept
    {
        m_Mock = other.m_Mock;
        m_Mock->Copy();
    }

    AllocateObject(AllocateObject&& other) noexcept
    {
        m_Mock = other.m_Mock;
        other.m_Mock = nullptr;
        m_Mock->Move();
    }

    ~AllocateObject() noexcept
    {
        if (m_Mock)
        {
            m_Mock->Destruct();
        }
    }
};


struct TestAllocator final : public std::pmr::memory_resource
{
    uint32_t m_allocationCount = 0;
    uint32_t m_deallocationCount = 0;
    size_t m_totalSize = 0;

private:
    void* do_allocate(size_t bytes, size_t align) override
    {
        ++m_allocationCount;
        m_totalSize += bytes;
        return FE::Memory::DefaultAllocate(bytes, align);
    }

    void do_deallocate(void* ptr, size_t bytes, size_t) override
    {
        ++m_deallocationCount;
        m_totalSize -= bytes;
        FE::Memory::DefaultFree(ptr);
    }

    bool do_is_equal(const memory_resource& other) const noexcept override
    {
        return this == &other;
    }
};
