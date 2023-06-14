using Ferrum.Core.Console;
using Ferrum.Core.Modules;

namespace Ferrum.Core.Tests;

[SetUpFixture]
public class GlobalSetup
{
    private Engine engine = null!;
    private ConsoleLogger logger = null!;

    [OneTimeSetUp]
    public void Setup()
    {
        engine = new Engine();
        logger = new ConsoleLogger();
    }

    [OneTimeTearDown]
    public void TearDown()
    {
        logger.Dispose();
        engine.Dispose();
    }
}
