#include <Tests/Common/TestCommon.h>
#include <FeCore/Modules/Singleton.h>

class ISingletonObject : public FE::IObject
{
};

class SingletonObject : public FE::SingletonImplBase<ISingletonObject>
{
    std::shared_ptr<MockConstructors> m_Mock;
public:
    SingletonObject(std::shared_ptr<MockConstructors> mock) noexcept
    {
        m_Mock = mock;
        m_Mock->Construct();
    }

    SingletonObject(const SingletonObject& other) noexcept
    {
        m_Mock = other.m_Mock;
        m_Mock->Copy();
    }

    SingletonObject(SingletonObject&& other) noexcept
    {
        m_Mock = other.m_Mock;
        m_Mock->Move();
    }

    ~SingletonObject() noexcept
    {
        m_Mock->Destruct();
    }
};

TEST(SingletonTest, CreateDelete)
{
    auto mock = std::make_shared<MockConstructors>();
    EXPECT_CALL(*mock, Construct()).Times(1);
    EXPECT_CALL(*mock, Destruct()).Times(1);
    EXPECT_CALL(*mock, Copy()).Times(0);
    EXPECT_CALL(*mock, Move()).Times(0);
    auto obj = std::make_unique<SingletonObject>(mock);

    ASSERT_EQ(obj.get(), FE::Singleton<ISingletonObject>::Get());
}
