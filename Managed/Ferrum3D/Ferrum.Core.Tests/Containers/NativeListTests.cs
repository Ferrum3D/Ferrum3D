﻿using System.Collections.Generic;
using System.Linq;
using Ferrum.Core.Containers;
using Ferrum.Core.Modules;
using NUnit.Framework;

namespace Ferrum.Core.Tests.Containers
{
    [TestFixture]
    public class NativeListTests
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
            using var nativeArray = new NativeList<int>(sourceArray);
            Assert.AreEqual(sourceArray.Length, nativeArray.LongCount);
            Assert.AreEqual(sourceArray.Length, nativeArray.Count);
            CollectionAssert.AreEqual(sourceArray, nativeArray);
        }

        [Test]
        public void CreateFromEnumerable()
        {
            var sourceArray = Enumerable.Range(0, 100).ToArray();
            using var nativeArray = new NativeList<int>(Enumerable.Range(0, 100));
            CollectionAssert.AreEqual(sourceArray, nativeArray);
        }

        [Test]
        [TestCase(new int[0], 0, 123)]
        [TestCase(new[] { 1, 2, 3 }, 0, 123)]
        [TestCase(new[] { 1, 2, 3 }, 1, 123)]
        [TestCase(new[] { 1, 2, 3 }, 3, 123)]
        public void Insert(int[] sourceArray, int index, int item)
        {
            var list = sourceArray.ToList();
            list.Insert(index, item);
            using var nativeList = new NativeList<int>(sourceArray);
            nativeList.Insert(index, item);
            CollectionAssert.AreEqual(list, nativeList);
        }

        [Test]
        [TestCase(new int[0], 123, false)]
        [TestCase(new[] { 1 }, 1, true)]
        [TestCase(new[] { 1 }, 123, false)]
        [TestCase(new[] { 1, 2, 3 }, 123, false)]
        [TestCase(new[] { 1, 2, 3 }, 2, true)]
        public void Contains(int[] sourceArray, int item, bool result)
        {
            using var nativeList = new NativeList<int>(sourceArray);
            Assert.AreEqual(result, nativeList.Contains(item));
        }

        [Test]
        [TestCase(new int[0], 123, -1)]
        [TestCase(new[] { 1 }, 1, 0)]
        [TestCase(new[] { 1 }, 123, -1)]
        [TestCase(new[] { 1, 2, 3 }, 123, -1)]
        [TestCase(new[] { 1, 2, 3 }, 2, 1)]
        public void IndexOf(int[] sourceArray, int item, int result)
        {
            using var nativeList = new NativeList<int>(sourceArray);
            Assert.AreEqual(result, nativeList.IndexOf(item));
        }

        [Test]
        [TestCase(new int[0], 1, new int[0], false)]
        [TestCase(new[] { 1 }, 1, new int[0], true)]
        [TestCase(new[] { 1, 2, 3 }, 1, new[] { 2, 3 }, true)]
        [TestCase(new[] { 1, 2, 1 }, 1, new[] { 2, 1 }, true)]
        [TestCase(new[] { 1, 2, 1 }, 0, new[] { 1, 2, 1 }, false)]
        public void Remove(int[] sourceArray, int item, int[] resultArray, bool result)
        {
            using var nativeList = new NativeList<int>(sourceArray);
            Assert.AreEqual(result, nativeList.Remove(item));
            CollectionAssert.AreEqual(resultArray, nativeList);
        }

        [Test]
        [TestCase(new[] { 1 }, 0, new int[0])]
        [TestCase(new[] { 1, 2, 1 }, 2, new[] { 1, 2 })]
        [TestCase(new[] { 1, 2, 1 }, 0, new[] { 2, 1 })]
        public void RemoveAt(int[] sourceArray, int index, int[] resultArray)
        {
            using var nativeList = new NativeList<int>(sourceArray);
            nativeList.RemoveAt(index);
            CollectionAssert.AreEqual(resultArray, nativeList);
        }

        [Test]
        public void CreateFromCapacity()
        {
            using var nativeArray = new NativeList<int>(10);
            Assert.AreEqual(0, nativeArray.LongCount);
            Assert.AreEqual(0, nativeArray.Count);
            Assert.LessOrEqual(10, nativeArray.LongCapacity);
            Assert.LessOrEqual(10, nativeArray.Capacity);
        }

        [Test]
        [TestCase(new[] { 1, 2, 3 })]
        [TestCase(new[] { 1 })]
        public void GetByIndex(int[] sourceArray)
        {
            using var nativeArray = new NativeList<int>(sourceArray);
            for (var i = 0; i < sourceArray.Length; ++i)
            {
                Assert.AreEqual(nativeArray[i], sourceArray[i]);
            }
        }

        [Test]
        public void SetByIndex()
        {
            using var nativeArray = new NativeList<int>(new[] { 0, 0 });
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