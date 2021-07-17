#pragma once
#include <FeCore/Console/FeLog.h>
#include <FeCore/Utils/CoreUtils.h>
#include <vector>

namespace FE
{
#define FT_CLASS_NAME(_testName, _testCat) _testName##_testCat
#define FT_CLASS_NAME_STR(_testName, _testCat) #_testCat "->" #_testName

#define FT_EXPECT_MSG(_expr, _message)                                                                                                     \
    if (!(_expr))                                                                                                                          \
    {                                                                                                                                      \
        RESULT = ::FE::FeTestResult(_message, false);                                                                                      \
        return;                                                                                                                            \
    }
#define FT_EXPECT(_expr) FT_EXPECT_MSG(_expr, "Expected " #_expr)

    template<class It1, class It2>
    inline bool FeSequenceEqual(It1 begin1, It1 end1, It2 begin2, It2 end2)
    {
        auto iter1 = begin1;
        auto iter2 = begin2;
        while (iter1 != end1 && iter2 != end2)
        {
            if (*iter1++ != *iter2++)
                return false;
        }

        return iter1 == end1 && iter2 == end2;
    }

    template<class Container1, class Container2>
    inline bool FeContainerEqual(const Container1& left, const Container2& right)
    {
        return FeSequenceEqual(left.cbegin(), left.cend(), right.cbegin(), right.cend());
    }

    template<class Container1, class Container2>
    inline bool FeKnownSizeContainerEqual(const Container1& left, const Container2& right)
    {
        return left.size() == right.size() && FeContainerEqual(left, right);
    }

    struct FeTestResult
    {
        bool Success;
        std::string Message;
        std::string Name;

        inline FeTestResult()
            : FeTestResult("", true)
        {
        }

        inline FeTestResult(const std::string_view message, bool success)
        {
            Message = message;
            Success = success;
        }
    };

    class IFeUnitTest
    {
    public:
        std::string FullName, Category, Name;
        virtual FeTestResult Run() = 0;
    };

    class FeUnit
    {
    public:
        std::vector<IFeUnitTest*> TestList;

        static inline FeUnit* GetInstance()
        {
            static FeUnit* instance = nullptr;
            if (instance == nullptr)
                instance = new FeUnit();
            return instance;
        }
    };

    inline void FeRegisterTest(IFeUnitTest* function)
    {
        FeUnit::GetInstance()->TestList.push_back(function);
    }

#define FE_TEST(_name, _category, ...)                                                                                                     \
    class FT_CLASS_NAME(_name, _category)                                                                                                  \
        : public ::FE::IFeUnitTest                                                                                                         \
    {                                                                                                                                      \
    public:                                                                                                                                \
        inline FT_CLASS_NAME(_name, _category)()                                                                                           \
        {                                                                                                                                  \
            ::FE::FeRegisterTest(this);                                                                                                    \
            FullName = FT_CLASS_NAME_STR(_name, _category);                                                                                \
            Name     = #_name;                                                                                                             \
            Category = #_category;                                                                                                         \
        }                                                                                                                                  \
        virtual void RunImpl(::FE::FeTestResult&);                                                                                         \
        virtual ::FE::FeTestResult Run() override                                                                                          \
        {                                                                                                                                  \
            ::FE::FeTestResult tr;                                                                                                         \
            RunImpl(tr);                                                                                                                   \
            tr.Name = FullName;                                                                                                            \
            return tr;                                                                                                                     \
        }                                                                                                                                  \
        static ::FE::IFeUnitTest* const Instance;                                                                                          \
    };                                                                                                                                     \
    ::FE::IFeUnitTest* const FT_CLASS_NAME(_name, _category)::Instance = new FT_CLASS_NAME(_name, _category)();                            \
    inline void FT_CLASS_NAME(_name, _category)::RunImpl(::FE::FeTestResult& RESULT)

    inline void FeRunAllTests()
    {
        for (auto test : FeUnit::GetInstance()->TestList)
        {
            auto res = test->Run();
            LogTrace(res.Success ? LogType::Success : LogType::Fail, "in test {}: {}", res.Name, res.Message);
        }
    }

    inline void FeRunTestsByCategory(const std::string_view category)
    {
        for (auto test : FeUnit::GetInstance()->TestList)
        {
            if (test->Category != category)
                continue;
            auto res = test->Run();
            LogTrace(res.Success ? LogType::Success : LogType::Fail, "in test {}: {}", res.Name, res.Message);
        }
    }

    inline void FeRunTestsByName(const std::string_view name)
    {
        for (auto test : FeUnit::GetInstance()->TestList)
        {
            if (test->Name != name)
                continue;
            auto res = test->Run();
            LogTrace(res.Success ? LogType::Success : LogType::Fail, "in test {}: {}", res.Name, res.Message);
        }
    }

    inline void FeRunTest(const std::string_view category, const std::string_view name)
    {
        for (auto test : FeUnit::GetInstance()->TestList)
        {
            if (test->Name != name || test->Category != category)
                continue;
            auto res = test->Run();
            LogTrace(res.Success ? LogType::Success : LogType::Fail, "in test {}: {}", res.Name, res.Message);
        }
    }

    inline void FeRunTest(const std::string_view fullName)
    {
        for (auto test : FeUnit::GetInstance()->TestList)
        {
            if (test->FullName != fullName)
                continue;
            auto res = test->Run();
            LogTrace(res.Success ? LogType::Success : LogType::Fail, "in test {}: {}", res.Name, res.Message);
        }
    }
} // namespace FE
