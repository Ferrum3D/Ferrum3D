#pragma once
#include <FerrumCore/CoreUtils.h>
#include <FerrumCore/FeLog.h>
#include <vector>

namespace Ferrum
{
#define FT_CLASS_NAME(_testName, _testCat) _testName ## _testCat
#define FT_CLASS_NAME_STR(_testName, _testCat) # _testCat "->" # _testName

#define FT_EXPECT_MSG(_expr, _message) if (!(_expr)) { RESULT = Ferrum::FeTestResult(_message, false); return; }
#define FT_EXPECT(_expr) FT_EXPECT_MSG(_expr, "Expected " # _expr)

	struct FeTestResult
	{
		bool Success;
		std::string Message;
		std::string Name;

		inline FeTestResult() : FeTestResult("", true) { }

		inline FeTestResult(const std::string_view message, bool success) {
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

		static inline FeUnit* GetInstance() {
			static FeUnit* instance = nullptr;
			if (instance == nullptr)
				instance = new FeUnit();
			return instance;
		}
	};

	inline void FeRegisterTest(IFeUnitTest* function) {
		FeUnit::GetInstance()->TestList.push_back(function);
	}

#define FE_TEST(_name, _category, ...)																				\
	class FT_CLASS_NAME(_name, _category) : public ::Ferrum::IFeUnitTest											\
	{																												\
	public:																											\
		inline FT_CLASS_NAME(_name, _category)() {																	\
			::Ferrum::FeRegisterTest(this);																			\
			FullName = FT_CLASS_NAME_STR(_name, _category);															\
			Name = # _name; Category = # _category;																	\
		}																											\
		virtual void RunImpl(::Ferrum::FeTestResult&);																\
		virtual ::Ferrum::FeTestResult Run() override {																\
			::Ferrum::FeTestResult tr;																				\
			RunImpl(tr);																							\
			tr.Name = FullName;																						\
			return tr;																								\
		}																											\
		static ::Ferrum::IFeUnitTest* const Instance;																\
	};																												\
	::Ferrum::IFeUnitTest* const FT_CLASS_NAME(_name, _category) :: Instance										\
			 = new FT_CLASS_NAME(_name, _category)();																\
	inline void FT_CLASS_NAME(_name, _category) :: RunImpl(::Ferrum::FeTestResult& RESULT)

	inline void FeRunAllTests() {
		for (auto test : FeUnit::GetInstance()->TestList) {
			auto res = test->Run();
			FeLog(res.Success ? FeLogType::Success : FeLogType::Fail, "in test {}: {}", res.Name, res.Message);
		}
	}

	inline void FeRunTestsByCategory(const std::string_view category) {
		for (auto test : FeUnit::GetInstance()->TestList) {
			if (test->Category != category)
				continue;
			auto res = test->Run();
			FeLog(res.Success ? FeLogType::Success : FeLogType::Fail, "in test {}: {}", res.Name, res.Message);
		}
	}

	inline void FeRunTestsByName(const std::string_view name) {
		for (auto test : FeUnit::GetInstance()->TestList) {
			if (test->Name != name)
				continue;
			auto res = test->Run();
			FeLog(res.Success ? FeLogType::Success : FeLogType::Fail, "in test {}: {}", res.Name, res.Message);
		}
	}

	inline void FeRunTest(const std::string_view category, const std::string_view name) {
		for (auto test : FeUnit::GetInstance()->TestList) {
			if (test->Name != name || test->Category != category)
				continue;
			auto res = test->Run();
			FeLog(res.Success ? FeLogType::Success : FeLogType::Fail, "in test {}: {}", res.Name, res.Message);
		}
	}

	inline void FeRunTest(const std::string_view fullName) {
		for (auto test : FeUnit::GetInstance()->TestList) {
			if (test->FullName != fullName)
				continue;
			auto res = test->Run();
			FeLog(res.Success ? FeLogType::Success : FeLogType::Fail, "in test {}: {}", res.Name, res.Message);
		}
	}
}
