#include <Core/IO/DirectoryWatcher.h>
#include <Core/IO/FileStream.h>
#include <Core/Math/Uuid.h>
#include <Core/Threading/Thread.h>
#include <gtest/gtest.h>

namespace FE::IO::Tests
{
    namespace
    {
        void WriteWatchedFile(const Path& path, const festd::string_view text)
        {
            auto fileResult = FileStream::Open(path, OpenMode::kCreate);
            ASSERT_TRUE(fileResult);
            ASSERT_EQ((*fileResult)->WriteFromBuffer(text.data(), text.size()), text.size());
            (*fileResult)->Close();
        }


        //! Poll until the watcher reports the path; notifications arrive asynchronously.
        bool WaitForChange(DirectoryWatcher& watcher, const Path& path)
        {
            const Path normalizedPath = NormalizePath(path);
            for (uint32_t attempt = 0; attempt < 200; ++attempt)
            {
                festd::vector<Path> changes;
                EXPECT_TRUE(watcher.PollChanges(changes));
                for (const Path& change : changes)
                {
                    if (change == normalizedPath)
                        return true;
                }

                Threading::Sleep(10);
            }

            return false;
        }
    } // namespace


    TEST(DirectoryWatcher, ReportsCreatedModifiedAndDeletedFilesInSubdirectories)
    {
        const Path root = Directory::GetExecutableDirectory() / Fmt::FixedFormat("DirectoryWatcherTests/{}", Uuid::Random());
        const Path nested = root / "Nested";
        ASSERT_EQ(Directory::Create(nested), ResultCode::kSuccess);

        const Path file = nested / "File.txt";
        {
            const Rc watcher = DirectoryWatcher::Create(root);
            ASSERT_TRUE(watcher);

            festd::vector<Path> changes;
            EXPECT_TRUE(watcher->PollChanges(changes));
            EXPECT_TRUE(changes.empty());

            WriteWatchedFile(file, "first");
            EXPECT_TRUE(WaitForChange(*watcher, file));

            WriteWatchedFile(file, "second");
            EXPECT_TRUE(WaitForChange(*watcher, file));

            EXPECT_EQ(File::Delete(file), ResultCode::kSuccess);
            EXPECT_TRUE(WaitForChange(*watcher, file));
        }

        EXPECT_EQ(Directory::Delete(nested), ResultCode::kSuccess);
        EXPECT_EQ(Directory::Delete(root), ResultCode::kSuccess);
    }


    TEST(DirectoryWatcher, ReturnsNullForMissingDirectory)
    {
        const Path missing = Directory::GetExecutableDirectory() / Fmt::FixedFormat("DirectoryWatcherTests/{}", Uuid::Random());
        EXPECT_FALSE(DirectoryWatcher::Create(missing));
    }
} // namespace FE::IO::Tests
