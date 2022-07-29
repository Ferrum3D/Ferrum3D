using Ferrum.Core.Components;
using Ferrum.Core.Entities;
using Ferrum.Core.Framework;
using Ferrum.Core.Utils;
using Ferrum.GameFramework;
using Ferrum.Osmium.Components;

namespace Ferrum.Samples.Scene3D
{
    internal class ExampleApplication : GameApplication
    {
        public const string ApplicationName = "Ferrum3D - 3D Scene";

        public override void Initialize(Desc desc)
        {
            base.Initialize(desc);
            var componentTypes = ComponentType.CreateList<Position3DComponent, LocalToWorldComponent, RenderMeshComponent>();
            var entity = EntityRegistry.CreateEntity(componentTypes);
            EntityRegistry.SetComponent(entity, new Position3DComponent(1, 2, 3));
            EntityRegistry.SetComponent(entity, new RenderMeshComponent
            {
                MeshAssetId = Uuid.Parse("884FEDDD-141D-49A0-92B2-38B519403D0A")
            });
        }
    }

    internal static class Program
    {
        private static int Main()
        {
            using var app = new ExampleApplication();
            app.Initialize(new ApplicationFramework.Desc(ExampleApplication.ApplicationName,
                "../../../Ferrum.Samples.Models/Assets", 1280, 720));
            return app.RunMainLoop();
        }
    }
}
