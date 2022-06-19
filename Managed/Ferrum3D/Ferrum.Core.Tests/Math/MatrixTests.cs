using System;
using Ferrum.Core.Math;
using NUnit.Framework;

namespace Ferrum.Core.Tests.Math
{
    [TestFixture]
    public class MatrixTests
    {
        [Test]
        [Repeat(1000)]
        public void CheckManagedEqualToUnmanaged()
        {
            var l = CreateRandomMatrix();
            var r = CreateRandomMatrix();
            var r1 = Matrix4x4F.MultiplyManaged(l, r);
            var r2 = l * r;
            Assert.IsTrue(Matrix4x4F.AreApproxEqual(r1, r2));
            Assert.IsFalse(r1 == r2);
        }

        private static Matrix4x4F CreateRandomMatrix()
        {
            var matrix = new Matrix4x4F();
            var rand = new Random();
            for (var i = 0; i < 4; ++i)
            {
                for (var j = 0; j < 4; ++j)
                {
                    matrix[i, j] = rand.NextFloat();
                }
            }

            return matrix;
        }
    }
}
