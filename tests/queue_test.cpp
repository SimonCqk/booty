#include"../ThreadPool/detail/ConcurrentQueue_impl.hpp"
#include"../ThreadPool/ConcurrentQueue.hpp"
#include"boost\lockfree\queue.hpp"
#include<queue>
#include<chrono>
#include<mutex>
#include<functional>

using namespace concurrentlib;
using namespace std;
using namespace std::chrono;

void single_thread() {
	auto start1 = system_clock::now();
	ConcurrentQueue<function<void()>> queue;
	for (int i = 0; i < 1000000; ++i) {
		function<void()> func;
		queue.enqueue(func);
	}
	for (int i = 0; i < 100000; ++i) {
		function<void()> data;
		queue.dequeue(data);
		//std::cout << data << ' ';
	}

	std::cout << std::endl;
	auto end1 = system_clock::now();
	auto start2 = system_clock::now();
	mutex mtx;
	std::queue<function<void()>> queue1;
	for (int i = 0; i < 1000000; ++i) {
		{
			lock_guard<mutex> lock(mtx);
			function<void()> func;
			queue1.push(func);
		}
	}

	for (int i = 0; i < 100000; ++i) {
		mutex mtx;
		lock_guard<mutex> lock(mtx);
		queue1.pop();
		//std::cout << data << ' ';
	}
	std::cout << std::endl;
	auto end2 = system_clock::now();
	auto dur1 = duration_cast<microseconds>(end1 - start1);
	auto dur2 = duration_cast<microseconds>(end2 - start2);
	std::cout << "Test one took "
		<< double(dur1.count()) * microseconds::period::num / microseconds::period::den
		<< "s" << std::endl;
	std::cout << "Test two took "
		<< double(dur2.count()) * microseconds::period::num / microseconds::period::den
		<< "s" << std::endl;
}

void multi_thread() {
	ConcurrentQueue<int> queue;
	vector<thread> threads;
	for (int n = 0; n < 5; ++n)
		threads.emplace_back([&queue] {
		for (int i = 0; i < 1000; ++i) {
			queue.enqueue(i);
		}
	});
	for (auto& thread : threads)
		thread.join();
	cout << "Finish enqueue and start dequeue." << endl;
	//vector<thread> other_threads;
	//for (int n = 0; n < 5; ++n)
	//	other_threads.emplace_back([&queue] {
	//	for (int i = 0; i < 100; ++i) {
	//		int m;
	//		queue.dequeue(m);
	//	}
	//});
	//for (auto& thread : other_threads)
	//	thread.join();
}


int main() {
	using namespace std::chrono;
	cout << "START!!!!" << endl;
	multi_thread();
	cout << "FINISH!!!!" << endl;
	return 0;
}