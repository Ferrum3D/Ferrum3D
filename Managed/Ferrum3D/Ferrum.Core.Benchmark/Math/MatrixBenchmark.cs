using System;
using BenchmarkDotNet.Attributes;
using Ferrum.Core.Math;

namespace Ferrum.Core.Benchmark.Math
{
    public class MatrixBenchmark
    {
        public static Matrix4x4F PublicField;
        private readonly Matrix4x4F matrix;

        public MatrixBenchmark()
        {
            var rand = new Random();
            for (var i = 0; i < 4; ++i)
            {
                for (var j = 0; j < 4; ++j)
                {
                    matrix[i, j] = rand.NextFloat();
                }
            }
        }

        [Benchmark]
        public void Managed()
        {
            PublicField = Matrix4x4F.MultiplyManaged(matrix, matrix);
        }

        [Benchmark]
        public void Unmanaged()
        {
            PublicField = matrix * matrix;
        }
    }
}
