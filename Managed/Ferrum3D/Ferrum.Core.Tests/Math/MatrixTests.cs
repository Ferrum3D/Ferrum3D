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
        }

        [Test]
        public void VectorTransformation()
        {
            var vector = new Vector3F(0, 1, 0);
            var translation = new Vector3F(0, 0, 1);
            var rotation = Quaternion.CreateRotationX(MathF.PI / 4);
            var matrix = Matrix4x4F.CreateRotation(rotation)
                         * Matrix4x4F.CreateTranslation(translation);
            var transformed = matrix * vector;
            Assert.AreEqual(new Vector3F(0, 0, MathF.Sqrt(2)), transformed);
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
