using System;
using System.Collections.Generic;

namespace Ferrum.Core.Containers
{
    public class DisposableList<T> : List<T>, IDisposable
        where T : IDisposable
    {
        public void Dispose()
        {
            for (var i = 0; i < Count; i++)
            {
                var elem = this[i];
                elem.Dispose();
            }
        }
    }
}
