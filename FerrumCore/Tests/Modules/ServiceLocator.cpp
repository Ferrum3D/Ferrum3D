#include <FeCore/Memory/Memory.h>
#include <FeCore/Modules/ServiceLocator.h>
#include <Tests/Common/TestCommon.h>


class ISingletonObject : public FE::Memory::RefCountedObjectBase
{
};


class SingletonObject final : public FE::ServiceLocatorImplBase<ISingletonObject>
{
    std::shared_ptr<MockConstructors> m_mock;

public:
    explicit SingletonObject(std::shared_ptr<MockConstructors> mock) noexcept
    {
        m_mock = std::move(mock);
        m_mock->Construct();
    }

    SingletonObject(const SingletonObject& other) noexcept
    {
        m_mock = other.m_mock;
        m_mock->Copy();
    }

    SingletonObject(SingletonObject&& other) noexcept
    {
        m_mock = other.m_mock;
        m_mock->Move();
    }

    ~SingletonObject() noexcept override
    {
        m_mock->Destruct();
    }
};

TEST(ServiceLocator, CreateDelete)
{
    const auto mock = std::make_shared<MockConstructors>();
    EXPECT_CALL(*mock, Construct()).Times(1);
    EXPECT_CALL(*mock, Destruct()).Times(1);
    EXPECT_CALL(*mock, Copy()).Times(0);
    EXPECT_CALL(*mock, Move()).Times(0);
    const auto obj = std::make_unique<SingletonObject>(mock);

    ASSERT_EQ(obj.get(), FE::ServiceLocator<ISingletonObject>::Get());
}
