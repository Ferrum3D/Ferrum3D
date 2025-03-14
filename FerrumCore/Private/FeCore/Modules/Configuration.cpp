#include <FeCore/Modules/Configuration.h>
#include <FeCore/Strings/Parser.h>
#include <FeCore/Strings/Utils.h>

namespace FE::Env
{
    Configuration::Configuration(const festd::span<const festd::string_view> commandLine)
    {
        FE_PROFILER_ZONE();

        ConfigurationSection& commandLineSection = m_rootSections.push_back();
        for (uint32_t argumentIndex = 0; argumentIndex < commandLine.size(); ++argumentIndex)
        {
            if (argumentIndex == commandLine.size() - 1)
                break;

            const festd::string_view argument = commandLine[argumentIndex];
            if (argument == "-c" || argument == "--config")
            {
                const auto tokens = Str::SplitFixed<2>(commandLine[argumentIndex + 1], '=');
                if (tokens.size() < 2)
                    continue;

                const festd::string_view key = tokens[0];
                const festd::string_view value = tokens[1];

                if (const auto doubleResult = Parser::TryParse<double>(value))
                {
                    commandLineSection.Set(key, doubleResult.value());
                }
                else if (const auto intResult = Parser::TryParse<int64_t>(value))
                {
                    commandLineSection.Set(key, intResult.value());
                }
                else
                {
                    commandLineSection.Set(key, Name{ value });
                }

                ++argumentIndex;
            }
        }

        // TODO: Parse JSON config
    }
} // namespace FE::Env
