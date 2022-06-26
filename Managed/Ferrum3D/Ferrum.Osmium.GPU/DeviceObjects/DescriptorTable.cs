using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class DescriptorTable : UnmanagedObject
    {
        public DescriptorTable(IntPtr handle) : base(handle)
        {
        }

        public void Update(int binding, Sampler sampler)
        {
            Update(new DescriptorWriteSampler(sampler, binding));
        }

        public void Update(DescriptorWriteSampler descriptorWriteSampler)
        {
            var nativeWrite = new DescriptorWriteSampler.Native(descriptorWriteSampler);
            UpdateNative(Handle, ref nativeWrite);
        }

        public void Update(int binding, ImageView imageView)
        {
            Update(new DescriptorWriteImage(imageView, binding));
        }

        public void Update(DescriptorWriteImage descriptorWriteImage)
        {
            var nativeWrite = new DescriptorWriteImage.Native(descriptorWriteImage);
            UpdateNative(Handle, ref nativeWrite);
        }

        public void Update(int binding, Buffer buffer)
        {
            Update(new DescriptorWriteBuffer(buffer, binding));
        }

        public void Update(DescriptorWriteBuffer descriptorWriteBuffer)
        {
            var nativeWrite = new DescriptorWriteBuffer.Native(descriptorWriteBuffer);
            UpdateNative(Handle, ref nativeWrite);
        }

        [DllImport("OsmiumBindings", EntryPoint = "IDescriptorTable_Destruct")]
        private static extern void DestructNative(IntPtr handle);

        [DllImport("OsmiumBindings", EntryPoint = "IDescriptorTable_UpdateSampler")]
        private static extern void UpdateNative(IntPtr handle, ref DescriptorWriteSampler.Native writeSampler);

        [DllImport("OsmiumBindings", EntryPoint = "IDescriptorTable_UpdateImage")]
        private static extern void UpdateNative(IntPtr handle, ref DescriptorWriteImage.Native writeImage);

        [DllImport("OsmiumBindings", EntryPoint = "IDescriptorTable_UpdateBuffer")]
        private static extern void UpdateNative(IntPtr handle, ref DescriptorWriteBuffer.Native writeBuffer);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }
    }
}
