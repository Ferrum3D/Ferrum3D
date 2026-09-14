#include <Core/IO/BaseIO.h>
#include <Core/IO/FileStream.h>
#include <gtest/gtest.h>

namespace FE::IO::Tests
{
    namespace
    {
        void WriteText(const Path& path, const festd::string_view text)
        {
            auto fileResult = FileStream::Open(path, OpenMode::kCreate);
            ASSERT_TRUE(fileResult);
            ASSERT_EQ((*fileResult)->WriteFromBuffer(text.data(), text.size()), text.size());
            (*fileResult)->Close();
        }


        festd::string ReadText(const Path& path)
        {
            auto fileResult = FileStream::Open(path, OpenMode::kReadOnly);
            EXPECT_TRUE(fileResult);
            if (!fileResult)
                return {};

            festd::string result;
            result.resize(static_cast<uint32_t>((*fileResult)->Length()), 0);
            EXPECT_EQ((*fileResult)->ReadToBuffer(result.data(), result.size()), result.size());
            (*fileResult)->Close();
            return result;
        }
    } // namespace


    TEST(BaseIO, MovePreservesExistingFileAndReplaceSwapsAtomically)
    {
        const Path source = GetAbsolutePath("base-io-move-source.ferrum-test-file");
        const Path destination = GetAbsolutePath("base-io-move-destination.ferrum-test-file");
        File::Delete(source);
        File::Delete(destination);
        const auto deferCleanup = festd::defer([&] {
            File::Delete(source);
            File::Delete(destination);
        });

        WriteText(source, "new");
        WriteText(destination, "old");

        EXPECT_EQ(File::Move(source, destination), ResultCode::kFileExists);
        EXPECT_TRUE(File::Exists(source));
        EXPECT_EQ(ReadText(destination), "old");

        EXPECT_EQ(File::Replace(source, destination), ResultCode::kSuccess);
        EXPECT_FALSE(File::Exists(source));
        EXPECT_EQ(ReadText(destination), "new");
    }
} // namespace FE::IO::Tests
