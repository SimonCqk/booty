## Booty

#### @Simoncqk

> `booty` is a effective & high performance & applied mordern c++ components library. It draws on others' successful experience and integrate them together.


### Finished Part

- **Thread Pool**: a thread pool implemented by mordern c++ features(>=c++17), and acheive a satisfying performance improvements. The underlying task queue is made by `std::queue + lock`, you may substitute it with lock-free queue to gain a step more improvement, but DO BE CAREFUL.

- **Signal-Slot**: a signal-slot model implementation with easy-to-use interfaces, you can easily add `callbacks` and drive the app with more convenience.

- **TimeStamp**: simple enough but also effective time stamp impl.

- **Unbounded Lock Queue**: simple concurrent queue with `std::queue + lock`.

- **Futex**: (Fast Userspace muTEXes), a high-level encapsulation of mutex, exists not only in kernel space but also user space, so it can be alive for a long time and perform better than `mutex`.

- **Saturing Semaphore**: Saturating Semaphore is a flag that allows concurrent posting by multiple posters and concurrent non-destructive waiting by multiple waiters.


### Under Working

- **Hazard Pointer**: `hazard pointer` is a lock-free data structure used in concurrency scene. It's used to protect objects who are intend to be visited by multi-threads, see [Hazard Pointer](http://www.drdobbs.com/lock-free-data-structures-with-hazard-po/184401890) for more details. Referenced by `facebook::folly`

- **Concurrent Lock-Free Queue**: a high performance & lock-free implementation of concurrent queue, single/multiple producers and signle/multiple consumers are supported, elegent and extraordinary, extracted from `facebook::folly`.

- **Log**: a high performance & well organized logging module, it uses fine-grained lock so performance in concurrency is great.
