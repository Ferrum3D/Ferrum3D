using System;
using Ferrum.Core.Containers;
using Ferrum.Core.Modules;
using NUnit.Framework;

namespace Ferrum.Core.Tests.Containers
{
    [TestFixture]
    public class NativeArrayTests
    {
        private Engine engine;

        [SetUp]
        public void Setup()
        {
            engine = new Engine();
        }

        [Test]
        [TestCase(new[] { 1, 2, 3 })]
        [TestCase(new[] { 1 })]
        [TestCase(new int[0])]
        public void CreateFromArray(int[] sourceArray)
        {
            using var nativeArray = new NativeArray<int>(sourceArray);
            Assert.AreEqual(sourceArray.Length, nativeArray.LongCount);
            Assert.AreEqual(sourceArray.Length, nativeArray.Count);
            CollectionAssert.AreEqual(sourceArray, nativeArray);
        }

        [Test]
        public void CreateFromCapacity()
        {
            using var nativeArray = new NativeArray<int>(10);
            Assert.AreEqual(10, nativeArray.LongCount);
            Assert.AreEqual(10, nativeArray.Count);
        }

        [Test]
        [TestCase(new[] { 1, 2, 3 })]
        [TestCase(new[] { 1 })]
        public void GetByIndex(int[] sourceArray)
        {
            using var nativeArray = new NativeArray<int>(sourceArray);
            for (var i = 0; i < sourceArray.Length; ++i)
            {
                Assert.AreEqual(nativeArray[i], sourceArray[i]);
            }
        }

        [Test]
        public void SetByIndex()
        {
            using var nativeArray = new NativeArray<int>(2);
            nativeArray[0] = int.MinValue;
            nativeArray[1] = 0;
            nativeArray[2] = int.MinValue;
            Assert.AreEqual(int.MinValue, nativeArray[0]);
            Assert.AreEqual(0, nativeArray[1]);
            Assert.AreEqual(int.MinValue, nativeArray[2]);
        }

        [TearDown]
        public void TearDown()
        {
            engine.Dispose();
        }
    }
}
