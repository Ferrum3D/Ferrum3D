using Ferrum.Core.Components;
using Ferrum.Core.Console;
using Ferrum.Core.Entities;

namespace Ferrum.Osmium.Systems
{
    public class DrawPacketSystem : ComponentSystem
    {
        private EntityQuery query;

        public override void OnUpdate()
        {
            query = EntityRegistry.ForEach((ref LocalToWorldComponent localToWorld) =>
            {
                // ConsoleLogger.LogMessage($"{localToWorld.Matrix} --- DT = {DeltaTime} --- ID = {FrameIndex}");
            }, query);
        }

        public override void OnDestroy()
        {
            ConsoleLogger.LogMessage(FrameIndex.ToString());
            query.Dispose();
        }
    }
}
