using System;
using Ferrum.Core.Modules;

namespace Ferrum.Core.Entities
{
    public class EntityQuery : UnmanagedObject
    {
        public EntityQuery(IntPtr handle) : base(handle)
        {
        }

        protected override void ReleaseUnmanagedResources()
        {
            throw new NotImplementedException();
        }
    }
}
