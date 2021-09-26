using BenchmarkDotNet.Running;

namespace Ferrum.Core.Benchmark
{
    internal static class Program
    {
        public static void Main()
        {
            BenchmarkRunner.Run(typeof(Program).Assembly);
        }
    }
}
