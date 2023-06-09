#pragma once
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <utility>

class MockConstructors
{
public:
    MOCK_METHOD(void, Construct, (), (noexcept));
    MOCK_METHOD(void, Destruct, (), (noexcept));
    MOCK_METHOD(void, Copy, (), (noexcept));
    MOCK_METHOD(void, Move, (), (noexcept));
};

class AllocateObject
{
    std::shared_ptr<MockConstructors> m_Mock;

public:
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
        m_Mock       = other.m_Mock;
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
