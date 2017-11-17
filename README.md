# Thread Pool

> This is a tiny thread pool embraced by C++11/14/17 features. So a c++17-supported complier is required for compling. Enable new features by add `-std=c++17 (g++/clang++)` or `/std:c++17 (vc++)`.

### Create Thread Pool Object
You can pass a specific interger to the ctor.
```cpp
ThreadPool pool(10);
```
Firstly, the pool will launch `core_thread_count` threads running and waiting for tasks, if it can not meet the demand , other new threads will be launched until it reaches `max_thread_count`  .

<br>

### Submit New Tasks
The core operation of Thread Pool is `submitting tasks` into the pool and let it run.<br>
Pass a callable obj as the **1st** arg and pass **other args** by each.<br>
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
If you need get the results of your task, call `.get()` on the obj returned by `.submitTask()`.<br>
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
I use two different simple methods to test the performance of Thread Pool.<br>
- 1. Create a thread , run the tasks , and let it go.
- 2. Create a thread pool, throw tasks into it.

```cpp
// method one.
std::vector<int> answers;
std::vector<std::thread> threads;
for (int i = 0; i < 3000; ++i) {
	threads.emplace_back([i,&answers] {
		std::cout << " process... " << i;
		for (int j = 0; j < 100; ++j);
		answers.push_back(i*i);
	});
}
for (auto& thread : threads) {
	thread.join();
}

// method two.
ThreadPool pool(10);
std::vector< std::future<int> > results;
for (int i = 0; i < 3000; ++i) {
	results.emplace_back(
		pool.submitTask([i] {
		std::cout << " process... " << i;
		for (int j = 0; j < 100; ++j);
		return i*i;
	})
	);
}
```
Select `Debug-mode` in Visual Studio, if you execute it directly, OS'll probably kill it for create-destroy threads too frequently.<br>
Here comes the test result:<br>
![test_result](https://github.com/SimonCqk/ThreadPool/blob/master/performance_test.png?raw=true)