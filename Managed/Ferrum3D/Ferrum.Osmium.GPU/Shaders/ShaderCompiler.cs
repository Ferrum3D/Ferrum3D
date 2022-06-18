using System;
using System.IO;
using System.Runtime.InteropServices;
using Ferrum.Core.Containers;
using Ferrum.Core.Modules;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.GPU.Shaders
{
    public class ShaderCompiler : UnmanagedObject
    {
        public ShaderCompiler(IntPtr handle) : base(handle)
        {
        }

        public ByteBuffer CompileShader(Args args)
        {
            return new ByteBuffer(CompileShaderNative(Handle, ref args));
        }

        [DllImport("OsmiumBindings", EntryPoint = "IShaderCompiler_CompileShader")]
        private static extern IntPtr CompileShaderNative(IntPtr self, ref Args args);

        [DllImport("OsmiumBindings", EntryPoint = "IShaderCompiler_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public readonly struct Args
        {
            public readonly HlslVersion Version;
            public readonly ShaderStage Stage;
            public readonly string SourceCode;
            public readonly string EntryPoint;
            public readonly string FullPath;

            public Args(HlslVersion version, ShaderStage stage, string sourceCode, string entryPoint, string fullPath)
            {
                Version = version;
                Stage = stage;
                SourceCode = sourceCode;
                EntryPoint = entryPoint;
                FullPath = fullPath;
            }

            public static Args FromFile(ShaderStage stage, string filePath, string entryPoint = "main")
            {
                return new Args(HlslVersion.V6, stage, File.ReadAllText(filePath), entryPoint, filePath);
            }

            public static Args FromFile(ShaderStage stage, string filePath, HlslVersion version, string entryPoint = "main")
            {
                return new Args(version, stage, File.ReadAllText(filePath), entryPoint, filePath);
            }
        }
    }
}
