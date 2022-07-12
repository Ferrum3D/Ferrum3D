using System;
using System.Collections.Generic;
using Ferrum.Core.Modules;
using Ferrum.Osmium.GPU.DeviceObjects;
using Buffer = Ferrum.Osmium.GPU.DeviceObjects.Buffer;

namespace Ferrum.Osmium.FrameGraph.Resources
{
    /// <summary>
    ///     Tracks memory barriers for resource aliasing. This class is used by the transient resource system
    ///     to add barriers on overlapping resources.
    /// </summary>
    public class AliasedResourceTracker
    {
        private readonly List<AliasedResourceDesc> resources = new();
        private readonly HashSet<(AliasedResourceDesc, AliasedResourceDesc)> barriers = new();

        public void Reset()
        {
            resources.Clear();
            barriers.Clear();
        }

        public void Add(in AliasedResourceDesc newResource)
        {
            for (var i = 0; i < resources.Count; i++)
            {
                var oldResource = resources[i];
                var intersection = oldResource.Intersect(newResource);

                switch (intersection)
                {
                    case AliasedResourceDesc.Intersection.Full:
                        AddBarrierIfNeeded(in oldResource, in newResource);
                        resources.RemoveAt(i);
                        --i;
                        break;
                    case AliasedResourceDesc.Intersection.Partial:
                        AddBarrierIfNeeded(oldResource, newResource);
                        var recycledOld = false;

                        if (oldResource.HeapOffsetMin < newResource.HeapOffsetMin)
                        {
                            oldResource.HeapOffsetMax = newResource.HeapOffsetMin - 1;
                            recycledOld = true;
                        }

                        if (oldResource.HeapOffsetMax > newResource.HeapOffsetMax)
                        {
                            if (!recycledOld)
                            {
                                oldResource.HeapOffsetMin = newResource.HeapOffsetMax + 1;
                            }

                            else
                            {
                                var right = oldResource;
                                right.HeapOffsetMin = newResource.HeapOffsetMax + 1;
                                resources.Insert(i + 1, right);
                                ++i;
                            }
                        }
                        break;
                    case AliasedResourceDesc.Intersection.None:
                        // The barrier is not needed if the resources do not overlap
                        break;
                    default:
                        throw new ArgumentOutOfRangeException();
                }
            }

            var wasInserted = false;
            for (var i = 0; i < resources.Count; i++)
            {
                if (resources[i].HeapOffsetMin > newResource.HeapOffsetMin)
                {
                    resources.Insert(i, newResource);
                    wasInserted = true;
                    break;
                }
            }

            if (!wasInserted)
            {
                resources.Add(newResource);
            }
        }

        private void AddBarrierIfNeeded(in AliasedResourceDesc oldResource, in AliasedResourceDesc newResource)
        {
            if (barriers.Contains((oldResource, newResource)))
            {
                return;
            }

            barriers.Add((oldResource, newResource));

            const ImageBindFlags imageWrite = ImageBindFlags.UnorderedAccess | ImageBindFlags.Color | ImageBindFlags.DepthStencil | ImageBindFlags.TransferWrite;
            
            switch (oldResource.Resource)
            {
                case Buffer buffer:
                    break;
                case Image image:
                    if ((image.BindFlags & imageWrite) != 0)
                    {
                        
                    }
                    break;
                default:
                    throw new Exception("Unknown resource type");
            }
            
            // TODO: it is possible that the render passes use different hardware queues we need to synchronize
            
        }
    }
}
