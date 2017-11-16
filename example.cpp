#include <iostream>
#include <vector>
#include <chrono>

#include "ThreadPool.h"

using namespace thread_pool;

int main()
{
	using namespace std::literals;
	ThreadPool pool(5);
	std::vector< std::future<int> > results;
	for (int i = 0; i < 50; ++i) {
		if (!(i % 10) && i != 0) {
			pool.pause();
			std::cout << "Being paused..." << std::endl;
			std::this_thread::sleep_for(10s);
			pool.unpause();
			std::cout << "Being unpaused..." << std::endl;
		}
		results.emplace_back(
			pool.submitTask([i] {
			std::cout << "hello " << i << std::endl;
			std::this_thread::sleep_for(1s);
			std::cout << "world " << i << std::endl;
			return i*i;
		})
		);
	}

	for (auto && result : results)
		std::cout << result.get() << ' ';
	std::cout << std::endl;

	return 0;
}
