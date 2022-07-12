using Ferrum.Core.Math;
using NUnit.Framework;

namespace Ferrum.Core.Tests.Math
{
    [TestFixture]
    public class MathTests
    {
        [Test]
        [TestCase(100, 0, 1)]
        [TestCase(123, 1, 123)]
        [TestCase(2, 10, 1024)]
        [TestCase(-2, 3, -8)]
        public void TestIntegralPow(int x, int y, int result)
        {
            Assert.AreEqual(result, MathF.Pow(x, y));
            if (x > 0)
            {
                Assert.AreEqual((ulong)result, MathF.Pow((ulong)x, (ulong)y));
            }
        }
    }
}
