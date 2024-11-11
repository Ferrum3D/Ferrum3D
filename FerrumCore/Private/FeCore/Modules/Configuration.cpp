#include <FeCore/Modules/Configuration.h>

namespace FE::Env
{
    Configuration::Configuration(festd::span<const StringSlice> commandLine)
    {
        ConfigurationSection& commandLineSection = m_rootSections.push_back();
        for (uint32_t argumentIndex = 0; argumentIndex < commandLine.size() - 1; ++argumentIndex)
        {
            const StringSlice argument = commandLine[argumentIndex];
            if (argument == "-c" || argument == "--config")
            {
                festd::fixed_vector<StringSlice, 2> tokens;
                commandLine[argumentIndex + 1].Split(tokens, 2, '=');
                if (tokens.size() < 2)
                    continue;

                const StringSlice key = tokens[0];
                const StringSlice value = tokens[1];

                if (const auto doubleResult = value.Parse<double>())
                {
                    commandLineSection.Set(key, doubleResult.Unwrap());
                }
                else if (const auto intResult = value.Parse<int64_t>())
                {
                    commandLineSection.Set(key, intResult.Unwrap());
                }
                else
                {
                    commandLineSection.Set(key, Env::Name{ value });
                }

                ++argumentIndex;
            }
        }

        // TODO: Parse JSON config
    }
} // namespace FE::Env
