#include <FeCore/IO/Path.h>
#include <FeCore/Logging/Trace.h>
#include <festd/vector.h>
#include <gtest/gtest.h>

using namespace FE;

TEST(Path, Combine)
{
    const IO::Path a = "C:/Foo";
    const IO::Path b = a / "Bar.txt";

    constexpr festd::string_view expected = "C:/Foo/Bar.txt";
    EXPECT_EQ(b, expected);
    EXPECT_EQ(IO::Path{ "C:/Foo/" } / "Bar.txt", expected);
    EXPECT_EQ(IO::Path{ "C:/Foo/" } / "Bar.txt", IO::Path{ expected });
    EXPECT_EQ(IO::Path{ "/" } / "Bar.txt", "/Bar.txt");

    // Combining with an absolute path
    EXPECT_EQ(IO::Path{ "C:/Foo/" } / "/Bar.txt", "/Bar.txt");
    EXPECT_EQ(IO::Path{ "C:/Foo/" } / "F:/Bar.txt", "F:/Bar.txt");
}

TEST(PathView, Basic)
{
    const IO::Path p = "C:/Foo/Bar.txt";
    const IO::PathView v{ p };

    EXPECT_TRUE(v.is_absolute());
    EXPECT_FALSE(v.is_relative());

    EXPECT_TRUE(v.has_root_name());
    EXPECT_TRUE(v.has_root_directory());
    EXPECT_TRUE(v.has_root_path());
    EXPECT_TRUE(v.has_relative_path());
    EXPECT_TRUE(v.has_filename());
    EXPECT_TRUE(v.has_stem());
    EXPECT_TRUE(v.has_extension());

    EXPECT_EQ(v.root_name(), "C:");
    EXPECT_EQ(v.root_directory(), "/");
    EXPECT_EQ(v.root_path(), "C:/");
    EXPECT_EQ(v.parent_directory(), "C:/Foo/");
    EXPECT_EQ(v.relative_path(), "Foo/Bar.txt");
    EXPECT_EQ(v.filename(), "Bar.txt");
    EXPECT_EQ(v.stem(), "Bar");
    EXPECT_EQ(v.extension(), ".txt");
}

TEST(PathView, FilenameStartingWithDot)
{
    const IO::Path p = "C:/Foo/.txt";
    const IO::PathView v{ p };

    EXPECT_FALSE(v.has_extension());
    EXPECT_TRUE(v.has_stem());

    EXPECT_EQ(v.root_name(), "C:");
    EXPECT_EQ(v.root_directory(), "/");
    EXPECT_EQ(v.root_path(), "C:/");
    EXPECT_EQ(v.parent_directory(), "C:/Foo/");
    EXPECT_EQ(v.relative_path(), "Foo/.txt");
    EXPECT_EQ(v.filename(), ".txt");
    EXPECT_EQ(v.stem(), ".txt");
    EXPECT_EQ(v.extension(), "");
}

TEST(PathView, RootPaths)
{
    EXPECT_EQ(IO::PathView{ "C:" }.root_path(), "C:");
    EXPECT_EQ(IO::PathView{ "C:/" }.root_path(), "C:/");
    EXPECT_EQ(IO::PathView{ "/" }.root_path(), "/");
    EXPECT_EQ(IO::PathView{ "foo.txt" }.root_path(), "");
}

TEST(PathView, Traverse)
{
    const festd::vector expected{ "C:", "Foo", "Bar", "File.txt" };

    uint32_t tokenIndex = 0;
    IO::TraversePath("C:/Foo/Bar////File.txt", [&](const festd::string_view token) {
        EXPECT_EQ(token, expected[tokenIndex++]);
    });
}
