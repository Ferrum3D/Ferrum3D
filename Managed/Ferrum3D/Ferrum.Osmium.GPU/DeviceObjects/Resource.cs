using System;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public abstract class Resource : UnmanagedObject
    {
        protected Resource(IntPtr handle) : base(handle)
        {
        }
    }
}
