using Ferrum.Core.Console;
using Ferrum.Core.Modules;
using NUnit.Framework;

[SetUpFixture]
public class GlobalSetup
{
    private Engine engine;
    private ConsoleLogger logger;

    [OneTimeSetUp]
    public void ShowSomeTrace()
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
