using Ferrum.Osmium.FrameGraph.RenderPasses;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.FrameGraph.Resources
{
    public struct AliasedResourceDesc
    {
        public readonly FrameGraphRenderPass Creator;
        public readonly FrameGraphRenderPass LastUser;

        public ulong HeapOffsetMin { get; internal set; }
        public ulong HeapOffsetMax { get; internal set; }

        public readonly Resource Resource;

        public AliasedResourceDesc(FrameGraphRenderPass creator, FrameGraphRenderPass lastUser, ulong heapOffsetMin,
            ulong heapOffsetMax, Resource resource)
        {
            Creator = creator;
            LastUser = lastUser;
            HeapOffsetMin = heapOffsetMin;
            HeapOffsetMax = heapOffsetMax;
            this.Resource = resource;
        }

        public Intersection Intersect(in AliasedResourceDesc newResource)
        {
            if (HeapOffsetMax < newResource.HeapOffsetMin || HeapOffsetMin > newResource.HeapOffsetMax)
            {
                return Intersection.None;
            }

            if (HeapOffsetMin >= newResource.HeapOffsetMin && HeapOffsetMax <= newResource.HeapOffsetMax)
            {
                return Intersection.Full;
            }

            return Intersection.Partial;
        }

        public bool Equals(AliasedResourceDesc other)
        {
            return Creator.Equals(other.Creator) && LastUser.Equals(other.LastUser) && HeapOffsetMin == other.HeapOffsetMin
                   && HeapOffsetMax == other.HeapOffsetMax && Resource.Equals(other.Resource);
        }

        public override bool Equals(object obj)
        {
            return obj is AliasedResourceDesc other && Equals(other);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = Creator.GetHashCode();
                hashCode = (hashCode * 397) ^ LastUser.GetHashCode();
                hashCode = (hashCode * 397) ^ HeapOffsetMin.GetHashCode();
                hashCode = (hashCode * 397) ^ HeapOffsetMax.GetHashCode();
                hashCode = (hashCode * 397) ^ Resource.GetHashCode();
                return hashCode;
            }
        }

        public enum Intersection
        {
            None,
            Full,
            Partial
        }
    }
}
