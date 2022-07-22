using System.Linq;
using Ferrum.Core.Components;
using Ferrum.Core.Console;
using Ferrum.Core.Entities;
using NUnit.Framework;

namespace Ferrum.Core.Tests.Entities
{
    [TestFixture]
    [SingleThreaded]
    public class EntityRegistryTests
    {
        [Component("2C4B3AFD-5343-4C0E-AD42-CA0CC4101970", Alignment = 4)]
        public struct TestComponent
        {
            public float TestData;
        }

        [Test]
        public void CreateEntity()
        {
            using var registry = new EntityRegistry();
            var entity = registry.CreateEntity();
            Assert.AreEqual(0, entity.Id);
            registry.DestroyEntity(entity);
        }

        [Test]
        public void CreateEntities()
        {
            const int count = 64 * 1024;

            using var registry = new EntityRegistry();
            var entities = registry.CreateEntities(count);
            Assert.AreEqual(count, entities.Count);
            CollectionAssert.AreEqual(Enumerable.Range(0, count), entities.Select(x => x.Id));
            registry.DestroyEntities(entities);
            entities.Dispose();
        }

        [Test]
        public void EntityInvalidAfterDestroy()
        {
            using var registry = new EntityRegistry();
            var entity = registry.CreateEntity();
            Assert.AreEqual(0, registry.GetCurrentEntityVersion(entity));
            Assert.IsTrue(registry.IsValid(entity));
            registry.DestroyEntity(entity);
            Assert.IsFalse(registry.IsValid(entity));
            Assert.AreEqual(1, registry.GetCurrentEntityVersion(entity));
        }

        [Test]
        public void AddComponent()
        {
            using var registry = new EntityRegistry();
            var entity = registry.CreateEntity();
            Assert.IsTrue(registry.AddComponent(entity, new PositionComponent(1, 2)));
            Assert.AreEqual(registry.GetComponent<PositionComponent>(entity), new PositionComponent(1, 2));
            Assert.IsFalse(registry.AddComponent(entity, new PositionComponent(3, 4)));
            Assert.AreEqual(registry.GetComponent<PositionComponent>(entity), new PositionComponent(3, 4));
            registry.DestroyEntity(entity);
        }

        [Test]
        public void RemoveComponent()
        {
            using var registry = new EntityRegistry();
            var entity = registry.CreateEntity();
            Assert.IsTrue(registry.AddComponent(entity, new PositionComponent(1, 2)));
            Assert.IsTrue(registry.HasComponent<PositionComponent>(entity));
            Assert.IsTrue(registry.RemoveComponent<PositionComponent>(entity));
            Assert.IsFalse(registry.HasComponent<PositionComponent>(entity));
        }

        [Test]
        public void MoveArchetype()
        {
            using var registry = new EntityRegistry();
            var entity = registry.CreateEntity();
            registry.AddComponent(entity, new PositionComponent(1, 2, 3));
            Assert.AreEqual(new PositionComponent(1, 2, 3), registry.GetComponent<PositionComponent>(entity));
            registry.AddComponent(entity, new TestComponent { TestData = 4 });
            Assert.AreEqual(new PositionComponent(1, 2, 3), registry.GetComponent<PositionComponent>(entity));
            Assert.AreEqual(4, registry.GetComponent<TestComponent>(entity).TestData);
            registry.RemoveComponent<TestComponent>(entity);
            Assert.AreEqual(new PositionComponent(1, 2, 3), registry.GetComponent<PositionComponent>(entity));
        }

        [Test]
        public void HandleMultipleArchetypeChunks()
        {
            // Create a lot of components, that do not fit into a single chunk
            const int count = 16 * 1024;

            using var registry = new EntityRegistry();
            var entities = registry.CreateEntities(count);
            registry.AddComponent<PositionComponent>(entities);
            registry.AddComponent<TestComponent>(entities);

            for (var i = 0; i < entities.Count; ++i)
            {
                registry.SetComponent(entities[i], new PositionComponent(1, i, i * 2));
                registry.SetComponent(entities[i], new TestComponent { TestData = i * 10 });
            }

            for (var i = 0; i < entities.Count; ++i)
            {
                Assert.IsTrue(registry.HasComponent<PositionComponent>(entities[i]));
                Assert.IsTrue(registry.HasComponent<TestComponent>(entities[i]));
                Assert.AreEqual(i * 10, registry.GetComponent<TestComponent>(entities[i]).TestData);
                Assert.AreEqual(1, registry.GetComponent<PositionComponent>(entities[i]).X);
                Assert.AreEqual(i, registry.GetComponent<PositionComponent>(entities[i]).Y);
                Assert.AreEqual(i * 2, registry.GetComponent<PositionComponent>(entities[i]).Z);
            }

            for (var i = 0; i < entities.Count; ++i)
            {
                Assert.IsTrue(registry.RemoveComponent<TestComponent>(entities[i]));
            }

            for (var i = 0; i < entities.Count; ++i)
            {
                Assert.IsTrue(registry.HasComponent<PositionComponent>(entities[i]));
                Assert.IsFalse(registry.HasComponent<TestComponent>(entities[i]));
                Assert.AreEqual(1, registry.GetComponent<PositionComponent>(entities[i]).X);
                Assert.AreEqual(i, registry.GetComponent<PositionComponent>(entities[i]).Y);
                Assert.AreEqual(i * 2, registry.GetComponent<PositionComponent>(entities[i]).Z);
            }
        }
    }
}
