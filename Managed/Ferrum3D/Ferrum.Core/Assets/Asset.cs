using System;
using Ferrum.Core.Modules;

namespace Ferrum.Core.Assets
{
    public abstract class Asset : UnmanagedObject
    {
        protected Asset() : base(IntPtr.Zero)
        {
        }

        public static TAsset Load<TAsset>(Guid assetId)
            where TAsset : Asset, new()
        {
            var result = new TAsset();
            result.Handle = result.LoadByIdImpl(AssetManager.Handle, assetId);
            result.Initialize();
            return result;
        }

        protected abstract IntPtr LoadByIdImpl(IntPtr manager, Guid assetId);

        protected virtual void Initialize()
        {
        }
    }
}
