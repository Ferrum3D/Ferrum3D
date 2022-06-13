using System;
using System.Runtime.InteropServices;
using System.Threading;
using Ferrum.Core.Math;

namespace Ferrum.Core.Modules
{
    public static class Engine
    {
        [DllImport("FeCoreBindings", EntryPoint = "InitEngine")]
        public static extern void Init();
        
        [DllImport("FeCoreBindings", EntryPoint = "DeinitEngine")]
        public static extern void Deinit();
    }
}
