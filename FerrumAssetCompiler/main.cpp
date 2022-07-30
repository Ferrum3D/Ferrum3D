#include <FeCore/Assets/AssetManager.h>
#include <FeCore/Assets/AssetProviderDev.h>
#include <FeCore/Assets/IAssetLoader.h>
#include <FeCore/Console/FeLog.h>
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/Memory/Memory.h>

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
        Console::PrintToStdout(Fmt::Format(HeaderTemplate, FerrumVersion.Major, FerrumVersion.Minor, FerrumVersion.Patch));
    }

    int CompileCommand(const ArraySlice<StringSlice>& args)
    {
        FE_LOG_ERROR("Compilation not implemented!");
        Console::PrintToStdout(String::Join("\n", args));
        return 1;
    }

    int BuildCommand(const ArraySlice<StringSlice>& args)
    {
        FE_LOG_ERROR("Building not implemented!");
        Console::PrintToStdout(String::Join("\n", args));
        return 1;
    }

    int RunCompiler(int argc, char** argv)
    {
        auto logger        = MakeShared<Debug::ConsoleLogger>();
        auto assetManager  = MakeShared<AssetManager>();
        auto assetProvider = MakeShared<AssetProviderDev>();
        auto assetRegistry = MakeShared<AssetRegistry>();
        assetProvider->AttachRegistry(assetRegistry);
        assetManager->AttachAssetProvider(assetProvider);

        List<StringSlice> args;
        args.Reserve(argc - 1);
        for (USize i = 1; i < argc; ++i)
        {
            args.Push(argv[i]);
        }

        if (args.Size() == 0 || args.Contains("--help") || args.Contains("-h"))
        {
            PrintHeader();
            Console::PrintToStdout(HelpMessage);
            return 0;
        }

        if (args.Contains("--version") || args.Contains("-v"))
        {
            PrintHeader();
            Console::PrintToStdout(CopyrightMessage);
            return 0;
        }

        if (args[0] == "compile")
        {
            return CompileCommand(ArraySlice(args)(1, args.Size()));
        }
        else if (args[0] == "build")
        {
            return BuildCommand(ArraySlice(args)(1, args.Size()));
        }

        FE_LOG_ERROR("Invalid command: {}", args[0]);
        return 1;
    }
} // namespace FE::Assets

int main(int argc, char** argv)
{
    FE::Env::CreateEnvironment();
    FE::GlobalAllocator<FE::HeapAllocator>::Init(FE::HeapAllocatorDesc{});
    int code = FE::Assets::RunCompiler(argc, argv);
    FE::Console::PrintToStdout("\n");
    FE::GlobalAllocator<FE::HeapAllocator>::Destroy();
    return code;
}
