#include "CommandLineTypes.h"

#include <gtest/gtest.h>

using namespace FE;

namespace
{
    template<uint32_t TSize>
    auto Parse(const festd::string_view (&args)[TSize])
    {
        static std::byte storage[4096];
        static std::pmr::monotonic_buffer_resource allocator{ storage, sizeof(storage) };
        allocator.release();
        return Cli::Parse<Cli::Tests::TestParser>(&allocator, festd::span<const festd::string_view>{ args });
    }
} // namespace

TEST(Cli, ParsesFlagsOptionsAndSubcommands)
{
    const festd::string_view args[] = { "--help", "build", "--asset=input.glb" };
    const auto parser = Parse(args);

    ASSERT_TRUE(parser.IsValid()) << parser.GetError().data();
    EXPECT_TRUE(parser.m_help);

    const auto* build = parser.GetSubcommand<Cli::Tests::Build>();
    ASSERT_NE(build, nullptr);
    EXPECT_TRUE(build->m_asset);
    EXPECT_EQ(build->m_asset.Get(), "input.glb");
}

TEST(Cli, ParsesSeparateOptionValue)
{
    const festd::string_view args[] = { "build", "--asset", "input.png" };
    const auto parser = Parse(args);

    ASSERT_TRUE(parser.IsValid()) << parser.GetError().data();
    ASSERT_NE(parser.GetSubcommand<Cli::Tests::Build>(), nullptr);
    EXPECT_EQ(parser.GetSubcommand<Cli::Tests::Build>()->m_asset.Get(), "input.png");
}

TEST(Cli, RejectsInvalidArguments)
{
    const festd::string_view unknownOption[] = { "--unknown" };
    EXPECT_FALSE(Parse(unknownOption).IsValid());

    const festd::string_view missingValue[] = { "build", "--asset" };
    EXPECT_FALSE(Parse(missingValue).IsValid());

    const festd::string_view duplicate[] = { "--help", "--help" };
    EXPECT_FALSE(Parse(duplicate).IsValid());

    const festd::string_view flagValue[] = { "--help=true" };
    EXPECT_FALSE(Parse(flagValue).IsValid());

    const festd::string_view unexpected[] = { "--", "build" };
    EXPECT_FALSE(Parse(unexpected).IsValid());
}

TEST(Cli, TerminatesOptions)
{
    const festd::string_view args[] = { "--" };
    EXPECT_TRUE(Parse(args).IsValid());
}

TEST(Cli, BuildsHelpFromReflection)
{
    const festd::string_view args[] = { "build" };
    std::byte storage[4096];
    std::pmr::monotonic_buffer_resource allocator{ storage, sizeof(storage) };
    const auto parser = Cli::Parse<Cli::Tests::TestParser>(&allocator, festd::span<const festd::string_view>{ args });
    ASSERT_TRUE(parser.IsValid());

    const auto rootHelp = Cli::BuildHelp(&allocator, "ferrum", parser);
    EXPECT_NE(rootHelp.find("Commands:"), rootHelp.end());
    EXPECT_NE(rootHelp.find("build"), rootHelp.end());

    const auto buildHelp = Cli::BuildHelp(&allocator, "ferrum", *parser.GetSubcommand<Cli::Tests::Build>());
    EXPECT_NE(buildHelp.find("Usage: ferrum build"), buildHelp.end());
    EXPECT_NE(buildHelp.find("--asset <path>"), buildHelp.end());
}
