using System;
using Ferrum.Core.Console;
using Ferrum.Core.Math;
using Ferrum.Core.Modules;

namespace TestCsProject
{
    class Program
    {
        static void Main(string[] args)
        {
            Engine.Init();
            ConsoleLogger.Init();
            
            ConsoleLogger.LogMessage("Test unicode. Тестим юникод. 中文考試. Æ ¶ ✅ ♣ ♘");

            var matrix = Matrix4x4F.Identity;
            Console.WriteLine(matrix);
            matrix *= 3;
            Console.WriteLine(matrix);
            Console.WriteLine(matrix.Determinant());
            
            ConsoleLogger.Deinit();
            Engine.Deinit();
        }
    }
}
