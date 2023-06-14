// See https://aka.ms/new-console-template for more information


using Ferrum.Core.Console;
using Ferrum.Core.Modules;

using var engine = new Engine();
using var logger = new ConsoleLogger();

ConsoleLogger.LogMessage("Some test message");
ConsoleLogger.LogWarning("Some test warning");
ConsoleLogger.LogError("Some test error");

ConsoleLogger.LogMessage("abcdefg абвгдеё");
