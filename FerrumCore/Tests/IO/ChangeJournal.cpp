#include <Core/IO/ChangeJournal.h>
#include <Core/IO/FileStream.h>
#include <Core/Math/Uuid.h>
#include <Core/Threading/Thread.h>
#include <gtest/gtest.h>

namespace FE::IO::Tests
{
    namespace
    {
        void WriteJournalFile(const Path& path, const festd::string_view text)
        {
            auto fileResult = FileStream::Open(path, OpenMode::kCreate);
            ASSERT_TRUE(fileResult);
            ASSERT_EQ((*fileResult)->WriteFromBuffer(text.data(), text.size()), text.size());
            (*fileResult)->Close();
        }


        //! Read until the journal reports the path; records become readable shortly after the file is closed.
        bool WaitForChange(ChangeJournal& journal, ChangeJournalCursor& cursor, const Path& path)
        {
            const Path normalizedPath = NormalizePath(path);
            for (uint32_t attempt = 0; attempt < 200; ++attempt)
            {
                festd::vector<Path> changes;
                EXPECT_EQ(journal.ReadChanges(cursor, changes), ChangeJournalReadResult::kSuccess);
                for (const Path& change : changes)
                {
                    if (change == normalizedPath)
                        return true;
                }

                Threading::Sleep(10);
            }

            return false;
        }


        struct ChangeJournalTest : testing::Test
        {
            void SetUp() override
            {
                m_root = Directory::GetExecutableDirectory() / Fmt::FixedFormat("ChangeJournalTests/{}", Uuid::Random());
                ASSERT_EQ(Directory::Create(m_root), ResultCode::kSuccess);

                m_journal = ChangeJournal::Open(m_root);
                if (!m_journal)
                    GTEST_SKIP() << "The volume has no readable change journal";

                ASSERT_TRUE(m_journal->AddDirectory(m_root));
            }

            void TearDown() override
            {
                m_journal.Reset();

                festd::vector<Path> files;
                festd::vector<Path> directories;
                Directory::TraverseRecursively(m_root, [&files, &directories](const DirectoryEntry& entry) {
                    if (Bit::AnySet(entry.m_attributes, FileAttributeFlags::kDirectory))
                        directories.push_back(Path{ entry.m_path });
                    else
                        files.push_back(Path{ entry.m_path });

                    return true;
                });

                for (const Path& file : files)
                    File::Delete(file);

                for (auto iter = directories.rbegin(); iter != directories.rend(); ++iter)
                    Directory::Delete(*iter);

                Directory::Delete(m_root);
            }

            Path m_root;
            Rc<ChangeJournal> m_journal;
        };
    } // namespace


    TEST_F(ChangeJournalTest, ReportsChangesInNewSubdirectories)
    {
        ChangeJournalCursor cursor = m_journal->GetEndCursor();

        const Path nested = m_root / "Nested";
        ASSERT_EQ(Directory::Create(nested), ResultCode::kSuccess);
        EXPECT_TRUE(WaitForChange(*m_journal, cursor, nested));

        const Path file = nested / "File.txt";
        WriteJournalFile(file, "first");
        EXPECT_TRUE(WaitForChange(*m_journal, cursor, file));

        EXPECT_EQ(File::Delete(file), ResultCode::kSuccess);
        EXPECT_TRUE(WaitForChange(*m_journal, cursor, file));
    }


    TEST_F(ChangeJournalTest, SavedCursorReportsChangesMadeWhileNotReading)
    {
        const ChangeJournalCursor savedCursor = m_journal->GetEndCursor();
        m_journal.Reset();

        const Path file = m_root / "Offline.txt";
        WriteJournalFile(file, "written while nothing was reading");

        m_journal = ChangeJournal::Open(m_root);
        ASSERT_TRUE(m_journal);
        ASSERT_TRUE(m_journal->AddDirectory(m_root));

        ChangeJournalCursor cursor = savedCursor;
        EXPECT_TRUE(WaitForChange(*m_journal, cursor, file));
    }


    TEST_F(ChangeJournalTest, CursorOfAnotherJournalExpires)
    {
        ChangeJournalCursor cursor = m_journal->GetEndCursor();
        cursor.m_journalId ^= 1;

        festd::vector<Path> changes;
        EXPECT_EQ(m_journal->ReadChanges(cursor, changes), ChangeJournalReadResult::kCursorExpired);
        EXPECT_EQ(cursor.m_journalId, m_journal->GetEndCursor().m_journalId);
    }
} // namespace FE::IO::Tests
