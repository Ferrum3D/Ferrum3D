using System.Runtime.InteropServices;
using Ferrum.Core.Entities;
using Ferrum.Core.Utils;
using Ferrum.Osmium.AssetStreaming;

namespace Ferrum.Osmium.Components
{
    [StructLayout(LayoutKind.Sequential)]
    [Component("DA02F569-B39A-4D33-BC4A-B55E3DB9E80A")]
    public struct RenderMeshComponent
    {
        public Uuid MeshAssetId;
        public MeshFlags MeshFlags;
        internal AssetStreamer.Handle AssetStreamerHandle;
    }
}
