#pragma once
#pragma once
#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H
#include<thread>
#include<mutex>
#include<future>
#include<condition_variable>
#include<vector>
#include<queue>
#include<functional>
#include<utility>

namespace thread_pool {

	class ThreadPool {
	public:
		explicit ThreadPool(const size_t& thread_count = (std::thread::hardware_concurrency() - 1));

		template<class Func,typename... Args>
		auto executor(Func&& f, Args&&... args)
			->std::future<typename std::result_of<Func(Args...)>::type>;
		void close();
		~ThreadPool();
	protected:
		void _scheduler();
	private:
		// thread-manager
		std::vector<std::thread> threads;
		std::queue<std::function<void()>> tasks;
		std::mutex queue_mtx;
		std::condition_variable cond_var;
		bool closed;
	};


	template<class Func, typename ...Args>
	inline auto ThreadPool::executor(Func && f, Args && ...args) -> std::future<typename std::result_of<Func(Args ...)>::type>
	{
		
	}

}
#endif // !_THREAD_POOL_H
