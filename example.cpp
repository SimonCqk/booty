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
	std::vector<int> answers;
	for (int i = 0; i < 1000; ++i) {
		std::thread th([i,&answers] {
			std::cout << " process... " << i;
			std::this_thread::sleep_for(100ms);
			std::cout << " still process... " << i;
			answers.push_back(i*i);
		});
		th.join();
	}
	for (auto& ans : answers) {
		std::cout << ans << ' ';
	}
	auto end1 = system_clock::now();
	auto start2= system_clock::now();
	ThreadPool pool(10);
	std::vector< std::future<int> > results;
	for (int i = 0; i < 1000; ++i) {
		results.emplace_back(
			pool.submitTask([i] {
			std::cout << " process... " << i;
			std::this_thread::sleep_for(100ms);
			std::cout << " still process... " << i;
			return i*i;
		})
		);
	}
	for (auto && result : results)
		std::cout << result.get() << ' ';
	std::cout << std::endl;
	auto end2=system_clock::now();
    
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
