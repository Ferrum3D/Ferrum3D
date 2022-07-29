using System;

namespace Ferrum.Core.Assets
{
    public interface IAssetStorage<out T>
    {
        T WithNativePointer(IntPtr pointer);
        T Reset();
    }
}
