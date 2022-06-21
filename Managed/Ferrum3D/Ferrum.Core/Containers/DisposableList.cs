using System;
using System.Collections.Generic;

namespace Ferrum.Core.Containers
{
    public class DisposableList<T> : List<T>, IDisposable
        where T : IDisposable
    {
        public void Dispose()
        {
            foreach (var elem in this)
            {
                elem.Dispose();
            }
        }
    }
}
