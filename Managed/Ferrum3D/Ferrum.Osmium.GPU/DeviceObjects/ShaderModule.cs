using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Containers;
using Ferrum.Core.Modules;
using Ferrum.Osmium.GPU.Shaders;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class ShaderModule : UnmanagedObject
    {
        internal ShaderModule(IntPtr handle) : base(handle)
        {
        }

        [DllImport("OsGPUBindings", EntryPoint = "IShaderModule_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        internal readonly struct DescNative
        {
            private readonly IntPtr Bytecode;
            private readonly ulong BytecodeSize;
            private readonly string EntryPoint;
            private readonly int Stage;

            public DescNative(Desc desc)
            {
                Bytecode = desc.Bytecode.DataPointer;
                BytecodeSize = (ulong)desc.Bytecode.LongCount;
                EntryPoint = desc.EntryPoint;
                Stage = (int)desc.Stage;
            }
        }

        public readonly struct Desc
        {
            public readonly NativeArray<byte> Bytecode;
            public readonly string EntryPoint;
            public readonly ShaderStage Stage;

            public Desc(ShaderStage stage, NativeArray<byte> bytecode)
            {
                Stage = stage;
                Bytecode = bytecode;
                EntryPoint = "main";
            }
        }
    }
}
