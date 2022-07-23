namespace Ferrum.Core.Entities
{
    public delegate void EntityQueryDelegate<T1>(ref T1 c1)
        where T1 : unmanaged;

    public delegate void EntityQueryDelegate<T1, T2>(ref T1 c1, ref T2 c2)
        where T1 : unmanaged
        where T2 : unmanaged;

    public delegate void EntityQueryDelegate<T1, T2, T3>(ref T1 c1, ref T2 c2, ref T3 c3)
        where T1 : unmanaged
        where T2 : unmanaged
        where T3 : unmanaged;

    public delegate void EntityQueryDelegate<T1, T2, T3, T4>(ref T1 c1, ref T2 c2, ref T3 c3, ref T4 c4)
        where T1 : unmanaged
        where T2 : unmanaged
        where T3 : unmanaged
        where T4 : unmanaged;
}
