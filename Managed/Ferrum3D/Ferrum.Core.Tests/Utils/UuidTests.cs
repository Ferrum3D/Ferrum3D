using Ferrum.Core.Utils;

namespace Ferrum.Core.Tests.Utils;

[TestFixture]
public class UuidTests
{
    [Test]
    public void Parse()
    {
        const string s = "E510C5A1-335C-42D3-865A-B4502AD5B439";

        var uuid = Uuid.Parse(s);
        var guid = Guid.Parse(s);

        Assert.That(guid.ToString().ToUpper(), Is.EqualTo(uuid.ToString()));
        Assert.That(Uuid.FromGuid(guid), Is.EqualTo(uuid));
    }

    [Test]
    public void Empty()
    {
        var uuid = Uuid.Empty;
        var guid = Guid.Empty;

        Assert.That(Uuid.FromGuid(guid), Is.EqualTo(uuid));
        Assert.That(guid.ToString().ToUpper(), Is.EqualTo(uuid.ToString()));
    }
}
