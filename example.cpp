#include <iostream>
#include <vector>
#include <chrono>

#include "ThreadPool.h"

using namespace thread_pool;

int main()
{
	using namespace std::literals;
	using namespace std::chrono;
	auto start1 = system_clock::now();
	std::vector<std::thread> threads;
	for (int i = 0; i < 3000; ++i) {
		threads.emplace_back([i] {
			std::cout << " process... " << i;
			for (int j = 0; j < 100; ++j);
			std::cout << i*i << ' ';
		});
	}
	for (auto& thread : threads) {
		thread.join();
	}
	auto end1 = system_clock::now();
	auto start2= system_clock::now();
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
	for (auto && result : results)
		std::cout << result.get() << ' ';
	auto end2=system_clock::now();
	std::cout << std::endl;
	auto dur1 = duration_cast<microseconds>(end1 - start1);
	auto dur2 = duration_cast<microseconds>(end2 - start2);
	std::cout << "Test one took "
		<< double(dur1.count()) * microseconds::period::num / microseconds::period::den
		<< "s" << std::endl;
	std::cout << "Test two took "
		<< double(dur2.count()) * microseconds::period::num / microseconds::period::den
		<< "s" << std::endl;
    
	return 0;
}
