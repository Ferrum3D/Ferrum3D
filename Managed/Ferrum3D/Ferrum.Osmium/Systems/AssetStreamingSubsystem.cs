using Ferrum.Core.Entities;
using Ferrum.Osmium.AssetStreaming;
using Ferrum.Osmium.Components;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.Systems
{
    internal class AssetStreamingSubsystem : ComponentSubsystem<DrawPacketSystem>
    {
        private Device device;
        private CommandQueue queue;
        private readonly EntityQuery entityQuery;

        public AssetStreamingSubsystem(EntityQuery query)
        {
            entityQuery = query;
        }

        public override void OnCreate()
        {
            device = ParentSystem.FrameGraphExecutor.Device;
            queue = ParentSystem.FrameGraphExecutor.TransferQueue;
        }

        public override void OnUpdate()
        {
            TransformUnloaded();
            LoadAssets();
        }

        private void TransformUnloaded()
        {
            entityQuery.ForEach((ref RenderMeshComponent mesh) =>
            {
                if (mesh.AssetStreamerHandle)
                {
                    return;
                }

                mesh.AssetStreamerHandle = ParentSystem.AssetStreamer.AddAsset<Mesh>(mesh.MeshAssetId);
                ParentSystem.AssetStreamer.GetAsset<Mesh>(mesh.AssetStreamerHandle).Flags = mesh.MeshFlags;
                ParentSystem.AssetStreamer.QueueLoad(mesh.AssetStreamerHandle);
            });
        }

        private void LoadAssets()
        {
            if (!ParentSystem.AssetStreamer.HasUpdateWork)
            {
                return;
            }

            using var commandBuffer = device.CreateCommandBuffer(CommandQueueClass.Transfer);
            using var fence = device.CreateFence(Fence.FenceState.Reset);

            using (var builder = commandBuffer.Begin())
            {
                var ctx = new AssetStreamingContext(device, builder);
                ParentSystem.AssetStreamer.Update(in ctx);
            }

            queue.SubmitBuffers(commandBuffer, fence, CommandQueue.SubmitFlags.None);
            fence.WaitOnCpu();
        }
    }
}
