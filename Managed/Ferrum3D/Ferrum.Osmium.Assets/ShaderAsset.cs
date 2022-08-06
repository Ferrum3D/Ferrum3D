using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Assets;
using Ferrum.Core.Containers;
using Ferrum.Osmium.GPU.Shaders;

namespace Ferrum.Osmium.Assets
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct ShaderAsset : IAssetStorage<ShaderAsset>
    {
        public readonly NativeString SourceCode;

        private ShaderAsset(in NativeString sourceCode)
        {
            SourceCode = sourceCode;
        }

        public ShaderAsset WithNativePointer(IntPtr pointer)
        {
            SourceCodeNative(pointer, out var code);
            return new ShaderAsset(in code);
        }

        public ShaderAsset Reset()
        {
            var s = SourceCode;
            s.Dispose();
            return new ShaderAsset();
        }

        public NativeArray<byte> Compile(ShaderCompiler compiler, ShaderStage stage, string entryPoint = "main")
        {
            return compiler.CompileShader(ShaderCompiler.Args.FromSourceCode(stage, SourceCode, "../shader.hlsl", entryPoint));
        }

        [DllImport("OsAssetsBindings", EntryPoint = "ShaderAssetStorage_GetSourceCode")]
        private static extern void SourceCodeNative(IntPtr self, out NativeString sourceCode);
    }
}
