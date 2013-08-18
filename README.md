RWLock
======

A read/write lock that allows for recurring read/write locks on the writer thread, or recurring read locks on all reader threads.

If the lock is marked as write then recurring read/write locks can occur on the same thread, while read/write lock attempts are blocked on all others. If the lock is marked as read then recurring read locks can occur on all threads, while write lock attempts are blocked on all threads.


This was mostly just for internal use as I needed a lock with this type of locking scheme. It isn't implemented very efficiently nor is relying on this type of locking design in your program very efficient either. The lock is also susceptible to writer-starvation.