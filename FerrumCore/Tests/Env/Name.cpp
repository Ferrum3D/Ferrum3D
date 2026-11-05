#include <Core/Env/Environment.h>
#include <Core/Math/UUID.h>
#include <Core/Strings/Format.h>
#include <Core/Threading/Thread.h>
#include <festd/string.h>
#include <gtest/gtest.h>

namespace FE::Tests
{
    TEST(EnvName, ConcurrentCreationPublishesOneCompleteRecord)
    {
        constexpr uint32_t kThreadCount = 8;
        constexpr uint32_t kNameCount = 16 * 1024;

        // Names must be new to this process for the threads to race on their creation, even with --gtest_repeat.
        const festd::fixed_string prefix = Fmt::FixedFormat("EnvNameTests/{}", Uuid::Random());

        festd::vector<uint32_t> handles(kThreadCount * kNameCount);
        std::atomic<uint32_t> readyThreadCount = 0;
        std::atomic<uint32_t> incompleteRecordCount = 0;

        Threading::Thread threads[kThreadCount];
        for (uint32_t threadIndex = 0; threadIndex < kThreadCount; ++threadIndex)
        {
            threads[threadIndex].Start("EnvNameRace", [&, threadIndex] {
                readyThreadCount.fetch_add(1, std::memory_order_relaxed);
                while (readyThreadCount.load(std::memory_order_relaxed) < kThreadCount)
                    _mm_pause();

                for (uint32_t nameIndex = 0; nameIndex < kNameCount; ++nameIndex)
                {
                    const festd::fixed_string string = Fmt::FixedFormat("{}/{}", prefix, nameIndex);
                    const Env::Name name{ string };

                    handles[threadIndex * kNameCount + nameIndex] = name.GetHandle();
                    if (name.size() != string.size() || strcmp(name.c_str(), string.c_str()) != 0)
                        incompleteRecordCount.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }

        for (Threading::Thread& thread : threads)
            thread.Join();

        EXPECT_EQ(incompleteRecordCount.load(), 0u);

        uint32_t mismatchedHandleCount = 0;
        for (uint32_t threadIndex = 1; threadIndex < kThreadCount; ++threadIndex)
        {
            for (uint32_t nameIndex = 0; nameIndex < kNameCount; ++nameIndex)
            {
                if (handles[threadIndex * kNameCount + nameIndex] != handles[nameIndex])
                    ++mismatchedHandleCount;
            }
        }

        EXPECT_EQ(mismatchedHandleCount, 0u);
    }
} // namespace FE::Tests
