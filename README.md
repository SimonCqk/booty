# The Old Version README.
# Left to be UPDATED.

# Thread Pool

> This is a tiny thread pool embraced by C++11/14/17 features. So a c++17-supported complier is required for compling. Enable new features by add `-std=c++17 (g++/clang++)` or `/std:c++17 (vc++)`.

###### this thread pool is combined with lock-free concurrent queue, if you wanna see a `std::queue + lock` version, please jump to `lockqueue` version.

### Create Thread Pool Object
You can pass a specific interger to the ctor to specify maximum working threads.
```cpp
ThreadPool pool(10);
```
First of all, the pool will launch `core_thread_count` threads running and waiting for tasks, if demand can not be met , other new threads will be launched until it reaches `max_thread_count`.

<br>

### Submit New Tasks
The core operation of Thread Pool is `submitting tasks` into the pool and let it run.<br>
Pass a callable obj as the **1st** arg and pass **other args** one by one.<br>
```cpp
ThreadPool pool(10);
int SomeFunc(args...);
// pass some function.
pool.submitTask(SomeFunc,args...);
// or lambda directly.
pool.submitTask([=]{
    // do something...
});
```

<br>

### Get the result
If you need get the results of your task, call `.get()` on the obj returned by `.submitTask()`( It's a std::future obj actually ).<br>
```cpp
auto res=pool.submitTask(Func,args...);
std::cout << res.get() << std::endl;
```

<br>

### Pause
Out of some needs, you may want to pause executing tasks, then just invoke `.pause()`,and `.unpause()` to continue.

<br>

### Close
Manually closing the Thread Pool is also supported, call `.close()`.<br>
Or the dtor will do it for you :)

<br>

> All the basical operations are above.

<br>

### Performance Test
I used two different simple methods to test the performance of Thread Pool.<br>
- 1. Create a thread , run the tasks , and let it go.
- 2. Create a thread pool, throw tasks into it.

```cpp
std::mutex mtx;
// method one.
std::vector<std::thread> threads;
for (int i = 0; i < 1000; ++i) {
	threads.emplace_back([i, &mtx] {
		{
			std::lock_guard<std::mutex> lock(mtx);
			std::cout << " process... " << i;
		}
		int test;
		for (int j = 0; j < 100000; ++j) {
			test = j * 100 / 123;
		}
		for (int j = 0; j < 100000; ++j) {
			test *= j * 123 / 100;
		}
		{
			std::lock_guard<std::mutex> lock(mtx);
			std::cout << (test*test) % 1000 << ' ';
		}
	});
}
for (auto& thread : threads) {
	thread.join();
}

// method two.
ThreadPool pool(4);
std::vector<std::future<void>> results;
for (int i = 0; i < 1000; ++i) {
	results.push_back(pool.submitTask([i, &mtx] {
		{
			std::lock_guard<std::mutex> lock(mtx);
			std::cout << " process... " << i;
		}
		int test;
		for (int j = 0; j < 100000; ++j) {
			test = j * 100 / 123;
		}
		for (int j = 0; j < 100000; ++j) {
			test *= j * 123 / 100;
		}
		{
			std::lock_guard<std::mutex> lock(mtx);
			std::cout << (test*test) % 1000 << ' ';
		}
	})
	);
}
for (auto& result : results)
	result.get();
```
REMARK: Test results under `Debug-mode` has no reference meaning .<br>
Here comes the test result:<br>
![test_result](https://github.com/SimonCqk/ThreadPool/blob/master/performance_test.png?raw=true)
<br>

**You can obviously see the performance gap.**

## The next step: Use `ConcurrentQueue` instead of `std::queue`, still under working.
