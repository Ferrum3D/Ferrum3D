using Ferrum.Core.Components;
using Ferrum.Core.Entities;
using NUnit.Framework;

namespace Ferrum.Core.Tests.Entities
{
    [TestFixture]
    [SingleThreaded]
    public class EntityQuery
    {
        [Test]
        public void ForEachTest()
        {
            const int count = 16 * 1024;

            using var registry = new EntityRegistry();
            var entities = registry.CreateEntities(count);
            registry.AddComponent<Position3DComponent>(entities);

            for (var i = 0; i < entities.Count; ++i)
            {
                registry.SetComponent(entities[i], new Position3DComponent(i, i * 2, i * 3));
                if (i < count / 2)
                {
                    registry.AddComponent(entities[i], new TestComponent { TestData = i * 10 });
                }
            }

            using var query1 = registry.CreateQuery();
            query1.All = ComponentType.CreateList<Position3DComponent, TestComponent>();
            query1.Update();

            var entityCount1 = 0;
            var entityCount2 = 0;

            query1.ForEach((ref Position3DComponent pos, ref TestComponent test) =>
            {
                Assert.AreEqual(pos.Y, pos.X * 2);
                Assert.AreEqual(pos.Z, pos.X * 3);
                Assert.AreEqual(test.TestData, pos.X * 10);

                ++entityCount1;
                pos = new Position3DComponent(pos.AsVector3F() * -1);
                test = new TestComponent { TestData = test.TestData * 10 };
            });

            // Query component pairs (pos, test), but use ony the position by reusing the same query object
            query1.ForEach((ref Position3DComponent pos) =>
            {
                Assert.IsTrue(pos.X <= 0);
                Assert.IsTrue(pos.Y <= 0);
                Assert.IsTrue(pos.Z <= 0);
                pos = new Position3DComponent(pos.AsVector3F() * -1);
            });

            registry.ForEach((ref Position3DComponent pos) =>
            {
                ++entityCount2;
                Assert.AreEqual(pos.Y, pos.X * 2);
                Assert.AreEqual(pos.Z, pos.X * 3);
            });

            Assert.AreEqual(count / 2, entityCount1);
            Assert.AreEqual(count, entityCount2);

            registry.DestroyEntities(entities);
            entities.Dispose();
        }
    }
}
