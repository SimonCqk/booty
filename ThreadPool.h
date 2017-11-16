#pragma once
#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H
#include<thread>
#include<mutex>
#include<future>
#include<condition_variable>
#include<vector>
#include<queue>
#include<tuple>
#include<functional>
#include<utility>
#include<memory>

namespace thread_pool {

	class ThreadPool {
	public:
		explicit ThreadPool(const size_t& max_threads);

		template<class Func, typename... Args>
		decltype(auto) submitTask(Func&& func, Args&&... args);
		void pause();
		void unpause();
		void close();
		bool isClosed() const;
		~ThreadPool();
	protected:
		void _scheduler();
		void _launchNew();
	private:
		static size_t core_thread_count;
		size_t max_thread_count;
		// thread-manager
		std::vector<std::thread> threads;
		// tasks-queue
		std::queue<std::function<void()>> tasks;
		// for synchronization
		std::mutex queue_mtx;
		std::mutex pause_mtx;
		std::condition_variable cond_var;
		bool paused;
		bool closed;
	};


	template<class Func, typename... Args>
	inline decltype(auto)
		ThreadPool::submitTask(Func&& func, Args&&...args)
	{
		if (tasks.size() > max_thread_count) {  // all threads are under-working.
			//  execute it asynchronous and let STL do the load-balancing.
			auto task = std::async(std::launch::async,
				std::forward<Func>(func), std::forward<Args>(args)...);
			return task;
		}

		using return_type = typename std::result_of<Func(Args...)>::type;

		auto task = std::make_shared<std::packaged_task<return_type()>>(
			[func = std::forward<Func>(func),
			args = std::make_tuple(std::forward<Args>(args)...)]()->return_type{
			return std::apply(func, args);
		}
		);

		auto fut = task->get_future();
		{
			std::lock_guard<std::mutex> lock(this->queue_mtx);
			tasks.emplace([=]() {  // `=` mode instead of `&` to avoid ref-dangle.
				(*task)();
			});
		}
		return fut;
	}
}

#endif // !_THREAD_POOL_H
