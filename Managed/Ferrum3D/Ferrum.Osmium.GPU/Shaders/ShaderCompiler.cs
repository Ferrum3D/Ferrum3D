using System;
using System.IO;
using System.Runtime.InteropServices;
using Ferrum.Core.Containers;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.Shaders
{
    public class ShaderCompiler : UnmanagedObject
    {
        internal ShaderCompiler(IntPtr handle) : base(handle)
        {
        }

        public NativeArray<byte> CompileShader(Args args)
        {
            CompileShaderNative(Handle, ref args, out var handle);
            return NativeArray<byte>.FromHandle(handle);
        }

        [DllImport("OsGPUBindings", EntryPoint = "IShaderCompiler_CompileShader")]
        private static extern IntPtr CompileShaderNative(IntPtr self, ref Args args, out ByteBuffer.Native result);

        [DllImport("OsGPUBindings", EntryPoint = "IShaderCompiler_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public readonly struct Args : IDisposable
        {
            public readonly HlslVersion Version;
            public readonly ShaderStage Stage;
            public readonly NativeString SourceCode;
            public readonly NativeString EntryPoint;
            public readonly NativeString FullPath;

            private Args(HlslVersion version, ShaderStage stage, NativeString sourceCode, string entryPoint, string fullPath)
            {
                Version = version;
                Stage = stage;
                SourceCode = sourceCode;
                EntryPoint = new NativeString(entryPoint);
                FullPath = new NativeString(fullPath);
            }

            public static Args FromSourceCode(ShaderStage stage, NativeString sourceCode, string fullPath,
                string entryPoint = "main")
            {
                return new Args(HlslVersion.V6, stage, sourceCode, entryPoint, fullPath);
            }

            public static Args FromFile(ShaderStage stage, string fullPath, string entryPoint = "main")
            {
                var code = new NativeString(File.ReadAllText(fullPath));
                return new Args(HlslVersion.V6, stage, code, entryPoint, fullPath);
            }

            public static Args FromFile(ShaderStage stage, string fullPath, HlslVersion version,
                string entryPoint = "main")
            {
                var code = new NativeString(File.ReadAllText(fullPath));
                return new Args(version, stage, code, entryPoint, fullPath);
            }

            public void Dispose()
            {
                var s = SourceCode;
                s.Dispose();
                var e = EntryPoint;
                e.Dispose();
                var f = FullPath;
                f.Dispose();
            }
        }
    }
}
