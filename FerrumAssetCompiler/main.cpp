#include <FeCore/Assets/AssetManager.h>
#include <FeCore/Assets/AssetProviderDev.h>
#include <FeCore/Assets/IAssetLoader.h>
#include <FeCore/Console/FeLog.h>
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/Framework/ApplicationFramework.h>
#include <FeCore/IO/FileStream.h>
#include <FeCore/Memory/Memory.h>
#include <OsAssets/OsmiumAssetsModule.h>

#define RAPIDJSON_SSE2
#define RAPIDJSON_PARSE_DEFAULT_FLAGS kParseCommentsFlag
#include <rapidjson/document.h>

namespace FE::Assets
{
    inline constexpr StringSlice HeaderTemplate = "feassetc - Ferrum3D v{}.{}.{} Asset compiler\n";

    inline constexpr StringSlice CopyrightMessage = "Nikita Dubovikov, Ferrum3D (C) 2022\n";

    inline constexpr StringSlice HelpMessage = R"(usage: feassetc [--version | -v] [--help | -h] <command> [<args>]
Supported commands:
    compile [-D<key>=<value>...] <filepaths>...
            This command compiles asset files to the format supported by the engine. There must exist a file with name
            `<filepath>.meta.json` containing all the metadata needed for the asset loading. You can also specify
            additional metadata with -D options.
            The compiled assets will be written to `<filepath>.compiled.pak` files.

    build   [--release] [--archive-size=<megabytes>] <index-filepath>
            This command incrementally builds the entire project asset database. All asset paths must be contained in the
            specified `FerrumAssetIndex.json` file and have corresponding `*.meta.json` files. The command emits a FerrumAssets
            file and `*.compiled.pak` files for each asset (in dev mode) or a set of large `asset-archive-*.pak` archive files
            (in release mode).
)";

    void PrintHeader()
    {
        Console::PrintToStdout(Fmt::Format(HeaderTemplate, 1, 1, 0));
    }

    template<AssetMetadataType T>
    bool TryConvertArrayToVector(const rapidjson::Value& value, typename CppTypeForAssetMetadataType<T>::Type& vector)
    {
        auto arr = value.GetArray();
        const USize vecSize = T == AssetMetadataType::Vector3 ? 3 : 4;
        if (arr.Size() != vecSize)
        {
            return false;
        }

        for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
        {
            if (!arr[i].IsFloat())
            {
                return false;
            }

            vector(i) = arr[i].GetFloat();
        }

        return true;
    }

    eastl::vector<AssetMetadataField> ReadMetadataForAsset(const StringSlice& assetFilePath, IAssetLoader** assetLoader)
    {
        auto metadataPath = Fmt::Format("{}.meta.json", assetFilePath);
        if (!IO::File::Exists(metadataPath))
        {
            FE_LOG_WARNING("Metadata file {} not found, metadata for the asset will be empty", metadataPath);
            return {};
        }

        auto contents = IO::File::ReadAllText(metadataPath);

        rapidjson::Document doc;
        if (doc.ParseInsitu(contents.Data()).HasParseError())
        {
            USize lineNumber = 1;
            for (auto c : StringSlice(contents.Data(), contents.Data() + doc.GetErrorOffset()))
            {
                if (c == '\n')
                {
                    lineNumber++;
                }
            }

            FE_FATAL_ERROR("Error while parsing asset metadata in file: {} at line: {}", metadataPath, lineNumber);
        }

        FE_EXPECT_OR_FATAL_ERROR(doc.HasMember("asset-type") && doc["asset-type"].IsString(),
                                 "`asset-type` not found or has invalid format in asset metadata in file: {}",
                                 metadataPath);

        StringSlice assetTypeStr = doc["asset-type"].GetString();

        auto assetType = assetTypeStr.Parse<Assets::AssetType>().UnwrapOrElse([&metadataPath](auto) {
            FE_UNREACHABLE("`asset-type` has invalid format in asset metadata in file: {}. It must be a UUID", metadataPath);
            return Assets::AssetType{};
        });

        doc.RemoveMember("asset-type");

        auto* loader = ServiceLocator<Assets::IAssetManager>::Get()->GetAssetLoader(assetType);
        FE_EXPECT_OR_FATAL_ERROR(loader != nullptr, "Asset loader for `{}` not found", assetType);
        if (assetLoader)
        {
            *assetLoader = loader;
        }

        auto metadata = loader->GetAssetMetadataFields();
        for (auto& field : metadata)
        {
            auto fieldSet = false;
            if (doc.HasMember(field.GetKey().Data()))
            {
                auto& value = doc[field.GetKey().Data()];
                switch (value.GetType())
                {
                case rapidjson::kNullType:
                    FE_LOG_WARNING("Asset metadata in file: {}: `Null` is not a valid metadata field.", metadataPath);
                    break;
                case rapidjson::kFalseType:
                    if (field.GetType() == AssetMetadataType::Bool)
                    {
                        field.SetValue<AssetMetadataType::Bool>(false);
                        fieldSet = true;
                    }
                    break;
                case rapidjson::kTrueType:
                    if (field.GetType() == AssetMetadataType::Bool)
                    {
                        field.SetValue<AssetMetadataType::Bool>(true);
                        fieldSet = true;
                    }
                    break;
                case rapidjson::kObjectType:
                    FE_LOG_WARNING("Asset metadata in file: {}: `Object` is not a valid metadata field.", metadataPath);
                    break;
                case rapidjson::kArrayType:
                    {
                        if (field.GetType() == AssetMetadataType::Vector4)
                        {
                            Vector4F vec4;
                            if (TryConvertArrayToVector<AssetMetadataType::Vector4>(value, vec4))
                            {
                                field.SetValue<AssetMetadataType::Vector4>(vec4);
                                fieldSet = true;
                            }
                        }
                        else
                        {
                            Vector3F vec3;
                            if (TryConvertArrayToVector<AssetMetadataType::Vector3>(value, vec3))
                            {
                                field.SetValue<AssetMetadataType::Vector3>(vec3);
                                fieldSet = true;
                            }
                        }
                        if (!fieldSet)
                        {
                            FE_LOG_WARNING("Asset metadata in file: {}: `Array` is not a valid metadata field.", metadataPath);
                        }
                    }
                    break;
                case rapidjson::kStringType:
                    {
                        StringSlice s = value.GetString();
                        auto uuid = s.Parse<UUID>();
                        if (uuid && field.GetType() == AssetMetadataType::UUID)
                        {
                            field.SetValue<AssetMetadataType::UUID>(uuid.Unwrap());
                            fieldSet = true;
                        }
                        else if (field.GetType() == AssetMetadataType::String)
                        {
                            field.SetValue<AssetMetadataType::String>(s);
                            fieldSet = true;
                        }
                    }
                    break;
                case rapidjson::kNumberType:
                    if (field.GetType() == AssetMetadataType::UInt)
                    {
                        field.SetValue<AssetMetadataType::UInt>(value.GetUint64());
                        fieldSet = true;
                    }
                    else if (field.GetType() == AssetMetadataType::Int)
                    {
                        field.SetValue<AssetMetadataType::Int>(value.GetInt64());
                        fieldSet = true;
                    }
                    else if (field.GetType() == AssetMetadataType::Int)
                    {
                        field.SetValue<AssetMetadataType::Float>(value.GetFloat());
                        fieldSet = true;
                    }
                    break;
                default:
                    break;
                }
            }

            if (fieldSet)
            {
                doc.RemoveMember(field.GetKey().Data());
            }
            else
            {
                FE_EXPECT_OR_FATAL_ERROR(field.IsOptional(),
                                         "A required parameter `{}` not found in asset metadata in file: {}",
                                         field.GetKey(),
                                         metadataPath);
                continue;
            }
        }

        eastl::vector<StringSlice> unusedMembers;
        for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it)
        {
            unusedMembers.push_back(it->name.GetString());
        }

        if (!unusedMembers.empty())
        {
            FE_LOG_WARNING(
                "These fields in asset metadata file {} where unused: ", metadataPath, String::Join(", ", unusedMembers));
        }

        return metadata;
    }

    void ProcessOneFile(const StringSlice& file, IO::IStream* outStream,
                        const ArraySlice<std::pair<String, String>>& additionalMetadata)
    {
        IAssetLoader* loader = nullptr;

        FE_ASSERT(additionalMetadata.Empty()); // TODO

        auto metadata = ReadMetadataForAsset(file, &loader);

        Rc fileHandle = Rc<IO::FileHandle>::DefaultNew();
        Rc stream = Rc<IO::FileStream>::DefaultNew(fileHandle);
        FE_IO_ASSERT(stream->Open(file, IO::OpenMode::ReadOnly));

        Rc<AssetStorage> storage = loader->CreateStorage();
        storage->ReleaseStrongRef();
        loader->LoadRawAsset(metadata, storage.Get(), stream.Get());

        loader->SaveAsset(storage.Get(), outStream);
    }

    int CompileCommand(const ArraySlice<StringSlice>& args)
    {
        eastl::vector<String> files;
        eastl::vector<std::pair<String, String>> additionalMetadata;
        for (auto& arg : args)
        {
            if (arg.StartsWith("-D"))
            {
                const auto split = arg(2, arg.Size()).Split('=');
                const StringSlice key = split[0];
                const StringSlice value = split[1];
                additionalMetadata.push_back(std::make_pair(key, value));
            }
            else
            {
                files.push_back(IO::Directory::GetCurrentDirectory() / arg);
            }
        }

        for (auto& file : files)
        {
            Rc fileHandle = Rc<IO::FileHandle>::DefaultNew();
            Rc stream = Rc<IO::FileStream>::DefaultNew(fileHandle);
            FE_IO_ASSERT(stream->Open(Fmt::Format("{}.compiled.pak", file), IO::OpenMode::Create));

            ProcessOneFile(file, stream.Get(), additionalMetadata);
        }

        return 0;
    }

    int BuildCommand(const ArraySlice<StringSlice>& args)
    {
        FE_LOG_ERROR("Building not implemented!");
        Console::PrintToStdout(String::Join("\n", args));
        return 1;
    }

    int RunCompiler(int argc, char** argv)
    {
        eastl::vector<StringSlice> args;
        args.reserve(argc - 1);
        for (USize i = 1; i < argc; ++i)
        {
            args.push_back(argv[i]);
        }

        const auto containsArg = [&args](StringSlice arg) {
            return eastl::find(args.begin(), args.end(), arg) != args.end();
        };

        if (args.size() == 0 || containsArg("--help") || containsArg("-h"))
        {
            PrintHeader();
            Console::PrintToStdout(HelpMessage);
            return 0;
        }

        if (containsArg("--version") || containsArg("-v"))
        {
            PrintHeader();
            Console::PrintToStdout(CopyrightMessage);
            return 0;
        }

        if (args[0] == "compile")
        {
            return CompileCommand(ArraySlice(args)(1, args.size()));
        }
        else if (args[0] == "build")
        {
            return BuildCommand(ArraySlice(args)(1, args.size()));
        }

        FE_LOG_ERROR("Invalid command: {}", args[0]);
        return 1;
    }
} // namespace FE::Assets

class CompilerApplication : public FE::ApplicationFramework
{
    int m_Argc;
    char** m_Argv;

protected:
    inline void PollSystemEvents() override {}

    inline bool CloseEventReceived() override
    {
        return false;
    }

private:
    inline void Tick(const FE::FrameEventArgs& frameEventArgs) override
    {
        ApplicationFramework::Tick(frameEventArgs);
        Stop(FE::Assets::RunCompiler(m_Argc, m_Argv));
    }

    inline void GetFrameworkDependencies(eastl::vector<FE::Rc<FE::IFrameworkFactory>>& dependencies) override
    {
        dependencies.push_back(FE::Osmium::OsmiumAssetsModule::CreateFactory());
    }

public:
    inline CompilerApplication(int argc, char** argv)
        : m_Argc(argc)
        , m_Argv(argv)
    {
    }

    void Initialize(const FE::ApplicationDesc& desc) override
    {
        ApplicationFramework::Initialize(desc);
        auto assetsModule = FE::ServiceLocator<FE::Osmium::OsmiumAssetsModule>::Get();
        assetsModule->Initialize(FE::Osmium::OsmiumAssetsModuleDesc{});
    }
};

FE_APP_MAIN()
{
    using namespace FE;
    Rc app = Rc<CompilerApplication>::DefaultNew(argc, argv);
    app->Initialize(ApplicationDesc("Ferrum3D Asset compiler", Debug::LogMessageType::Warning | Debug::LogMessageType::Error));
    return app->RunMainLoop();
}
