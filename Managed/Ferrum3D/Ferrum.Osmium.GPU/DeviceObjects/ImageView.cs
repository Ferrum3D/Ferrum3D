using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class ImageView : UnmanagedObject
    {
        public ImageView(IntPtr handle) : base(handle)
        {
        }
        
        [DllImport("OsmiumBindings", EntryPoint = "IImageView_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }
    }
}
