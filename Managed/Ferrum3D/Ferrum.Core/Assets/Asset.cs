using System;
using Ferrum.Core.Modules;
using Ferrum.Core.Utils;

namespace Ferrum.Core.Assets
{
    public abstract class Asset : UnmanagedObject
    {
        protected Asset() : base(IntPtr.Zero)
        {
        }

        public static TAsset Load<TAsset>(in Uuid assetId)
            where TAsset : Asset, new()
        {
            var result = new TAsset();
            result.Handle = result.LoadByIdImpl(AssetManager.Handle, in assetId);
            result.Initialize();
            return result;
        }

        protected abstract IntPtr LoadByIdImpl(IntPtr manager, in Uuid assetId);

        protected virtual void Initialize()
        {
        }
    }
}
