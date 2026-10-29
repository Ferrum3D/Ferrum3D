#include <Core/IO/Artifact.h>
#include <Core/IO/FileStream.h>
#include <Core/Threading/Mutex.h>
#include <Graphics/Core/ShaderPermutationManifest.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Core
{
    namespace
    {
        struct StageToken final
        {
            ShaderStage m_stage;
            festd::string_view m_token;
        };


        constexpr StageToken kStageTokens[] = {
            { ShaderStage::kVertex, "vs" }, { ShaderStage::kAmplification, "as" }, { ShaderStage::kMesh, "ms" },
            { ShaderStage::kPixel, "ps" },  { ShaderStage::kCompute, "cs" },
        };


        festd::string_view GetStageToken(const ShaderStage stage)
        {
            for (const StageToken& token : kStageTokens)
            {
                if (token.m_stage == stage)
                    return token.m_token;
            }

            return {};
        }


        ShaderStage ParseStageToken(const festd::string_view text)
        {
            for (const StageToken& token : kStageTokens)
            {
                if (token.m_token == text)
                    return token.m_stage;
            }

            return ShaderStage::kUndefined;
        }


        bool IsToken(const festd::string_view text)
        {
            for (uint32_t index = 0; index < text.size(); ++index)
            {
                const char character = static_cast<char>(text.byte_at(index));
                if (character == ' ' || character == '\t' || character == '\r' || character == '\n' || character == '=')
                    return false;
            }

            return !text.empty();
        }


        //! Format a permutation as one manifest line without the line break. Returns false if it cannot be represented.
        bool FormatLine(const ShaderCompilerArgs& args, festd::string& line)
        {
            const festd::string_view stage = GetStageToken(args.m_stage);
            const festd::string_view path{ args.m_shaderName };
            if (stage.empty() || !IsToken(path))
                return false;

            line.append(stage);
            line.push_back(' ');
            line.append(path);
            for (const ShaderDefine& define : args.m_defines)
            {
                if (!IsToken(define.m_name) || (!define.m_value.empty() && !IsToken(define.m_value)))
                    return false;

                line.push_back(' ');
                line.append(define.m_name);
                line.push_back('=');
                line.append(define.m_value);
            }

            return true;
        }


        void SplitTokens(const festd::string_view line, festd::inline_vector<festd::string_view, 8>& tokens)
        {
            uint32_t start = 0;
            for (uint32_t index = 0; index <= line.size(); ++index)
            {
                const bool isSeparator = index == line.size() || line.byte_at(index) == ' ' || line.byte_at(index) == '\t';
                if (!isSeparator)
                    continue;

                if (index > start)
                    tokens.push_back(line.substr_ascii(start, index - start));

                start = index + 1;
            }
        }


        bool ParseLine(const festd::string_view line, ShaderPermutation& permutation)
        {
            festd::inline_vector<festd::string_view, 8> tokens;
            SplitTokens(line, tokens);
            if (tokens.size() < 2)
                return false;

            permutation.m_stage = ParseStageToken(tokens[0]);
            if (permutation.m_stage == ShaderStage::kUndefined)
                return false;

            permutation.m_shaderName = Env::Name{ tokens[1] };
            for (uint32_t index = 2; index < tokens.size(); ++index)
            {
                const festd::string_view token = tokens[index];

                uint32_t separator = kInvalidIndex;
                for (uint32_t character = 0; character < token.size(); ++character)
                {
                    if (token.byte_at(character) == '=')
                    {
                        separator = character;
                        break;
                    }
                }

                if (separator == kInvalidIndex || separator == 0)
                    return false;

                ShaderPermutationDefine& define = permutation.m_defines.emplace_back();
                define.m_name = token.substr_ascii(0, separator);
                define.m_value = token.substr_ascii(separator + 1);
            }

            return true;
        }


        bool ReadText(const IO::Path& path, festd::string& text)
        {
            auto stream = IO::FileStream::Open(path, IO::OpenMode::kReadOnly);
            if (!stream)
                return false;

            const uint32_t size = static_cast<uint32_t>(stream.value()->Length());
            text.resize(size, ' ');
            return stream.value()->ReadToBuffer(text.data(), size) == size;
        }


        template<class TFunctor>
        void ForEachLine(const festd::string_view text, TFunctor&& functor)
        {
            uint32_t start = 0;
            for (uint32_t index = 0; index <= text.size(); ++index)
            {
                const bool isLineEnd = index == text.size() || text.byte_at(index) == '\n';
                if (!isLineEnd)
                    continue;

                uint32_t end = index;
                if (end > start && text.byte_at(end - 1) == '\r')
                    --end;

                if (end > start)
                    functor(text.substr_ascii(start, end - start));

                start = index + 1;
            }
        }


#if FE_DEVELOPMENT
        //! Permutations recorded by this process, and those found in the manifest when it was first used.
        struct ShaderPermutationRecorder final
        {
            Threading::Mutex m_lock;
            IO::Path m_manifestPath;
            festd::unordered_dense_set<uint64_t> m_recordedLines;
        };


        ShaderPermutationRecorder& GetRecorder()
        {
            static ShaderPermutationRecorder recorder;
            return recorder;
        }
#endif
    } // namespace


    void ShaderPermutation::GetCompilerArgs(ShaderCompilerArgs& args, festd::inline_vector<ShaderDefine, 4>& defines) const
    {
        defines.clear();
        for (const ShaderPermutationDefine& define : m_defines)
            defines.push_back({ festd::string_view{ define.m_name }, festd::string_view{ define.m_value } });

        args.m_shaderName = m_shaderName;
        args.m_stage = m_stage;
        args.m_defines = festd::span{ defines.data(), defines.size() };
    }


    IO::Path GetShaderPermutationManifestPath(const IO::Path& catalogDirectory)
    {
        return catalogDirectory / "artifacts/manifests/shader-permutations.txt";
    }


    bool LoadShaderPermutationManifest(const IO::Path& path, festd::vector<ShaderPermutation>& permutations)
    {
        festd::string text;
        if (!ReadText(path, text))
            return false;

        festd::unordered_dense_set<uint64_t> lines;
        ForEachLine(festd::string_view{ text }, [&permutations, &lines](const festd::string_view line) {
            if (!lines.insert(DefaultHash(line.data(), line.size())).second)
                return;

            ShaderPermutation permutation;
            if (ParseLine(line, permutation))
                permutations.push_back(std::move(permutation));
        });

        return true;
    }


    void RecordShaderPermutation(const ShaderCompilerArgs& args)
    {
#if FE_DEVELOPMENT
        const festd::string_view catalog = IO::ArtifactStore::GetCatalogSource();
        if (catalog.empty())
            return;

        festd::string line;
        if (!FormatLine(args, line))
            return;

        const uint64_t lineHash = DefaultHash(line.data(), line.size());

        ShaderPermutationRecorder& recorder = GetRecorder();
        std::lock_guard lock{ recorder.m_lock };

        const IO::Path manifestPath = GetShaderPermutationManifestPath(IO::Path{ catalog });
        if (recorder.m_manifestPath != manifestPath)
        {
            recorder.m_manifestPath = manifestPath;
            recorder.m_recordedLines.clear();

            festd::string text;
            if (ReadText(manifestPath, text))
            {
                ForEachLine(festd::string_view{ text }, [&recorder](const festd::string_view existingLine) {
                    recorder.m_recordedLines.insert(DefaultHash(existingLine.data(), existingLine.size()));
                });
            }
        }

        if (!recorder.m_recordedLines.insert(lineHash).second)
            return;

        line.push_back('\n');

        const bool exists = IO::File::Exists(manifestPath);
        if (!exists && IO::Directory::Create(IO::PathView{ manifestPath }.parent_directory()) != IO::ResultCode::kSuccess)
            return;

        // Another process may hold the manifest open; the permutation is then recorded by a later run.
        auto stream = IO::FileStream::Open(manifestPath, exists ? IO::OpenMode::kAppend : IO::OpenMode::kCreate);
        if (!stream)
            return;

        stream.value()->Seek(0, IO::SeekMode::kEnd);
        stream.value()->WriteFromBuffer(line.data(), line.size());
        stream.value()->Close();
#else
        (void)args;
#endif
    }
} // namespace FE::Graphics::Core
