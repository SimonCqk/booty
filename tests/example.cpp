#include <iostream>
#include <vector>
#include <chrono>

#include"../ThreadPool/ThreadPool.hpp"

using namespace concurrentlib;

int main()
{
	std::mutex mtx;
	using namespace std::literals;
	using namespace std::chrono;
	auto start1 = system_clock::now();
	/*std::vector<std::thread> threads;
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
	}*/
	auto end1 = system_clock::now();
	auto start2 = system_clock::now();
	ThreadPool pool(8);
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
	auto end2 = system_clock::now();
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
