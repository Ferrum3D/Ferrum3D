using System;
using Ferrum.Core.Utils;
using NUnit.Framework;

namespace Ferrum.Core.Tests.Utils
{
    [TestFixture]
    public class UuidTests
    {
        [Test]
        public void Parse()
        {
            const string s = "E510C5A1-335C-42D3-865A-B4502AD5B439";

            var uuid = Uuid.Parse(s);
            var guid = Guid.Parse(s);

            Assert.AreEqual(uuid.ToString(), guid.ToString().ToUpper());
            Assert.AreEqual(uuid, guid);
        }

        [Test]
        public void Empty()
        {
            var uuid = Uuid.Empty;
            var guid = Guid.Empty;

            Assert.AreEqual(uuid, guid);
            Assert.AreEqual(uuid.ToString(), guid.ToString().ToUpper());
        }
    }
}
