#include"../ThreadPool/detail/ConcurrentQueue_impl.hpp"
#include"../ThreadPool/ConcurrentQueue.hpp"
#include<queue>
#include<chrono>
#include<mutex>
#include<functional>

using namespace concurrentlib;
using namespace std;
using namespace std::chrono;

void single_thread() {
	auto start1 = system_clock::now();
	ConcurrentQueue<int> queue;
	for (int i = 0; i < 100; ++i)
		queue.enqueue(i);
	
	for (int i = 0; i < 100; ++i) {
		int data;
		queue.dequeue(data);
		std::cout << data << ' ';
	}
	
	std::cout << std::endl;
	auto end1 = system_clock::now();
	auto start2 = system_clock::now();
	std::queue<function<void()>> queue1;
	for (int i = 0; i < 100000; ++i) {
		mutex mtx;
		lock_guard<mutex> lock(mtx);
		queue1.emplace([i] {
			int a = i + 1;
			cout << a << endl;
		});
	}
	
	for (int i = 0; i < 100000; ++i) {
		mutex mtx;
		lock_guard<mutex> lock(mtx);
		function<void()> data = queue1.front();
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
	for (int n = 0; n < 500; ++n)
		threads.emplace_back([&queue] {
		for (int i = 0; i < 10; ++i) {
			cout << "start to enqueue." << endl;
			queue.enqueue(i);
			cout<< "enqueue successfully." << endl;
		}
	});
	/*vector<thread> other_threads;
	for (int n = 0; n < 100; ++n)
		other_threads.emplace_back([&queue] {
		for (int i = 0; i < 10; ++i)
			queue.dequeue();
	});*/
	for (auto& thread : threads)
		thread.join();
	/*for (auto& thread : other_threads)
		thread.join();*/
}

int main() {
	cout << "START!!!!" << endl;
	single_thread();
	cout << "FINISH!!!!" << endl;
	return 0;
}