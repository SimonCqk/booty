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
#include<memory>

namespace thread_pool {

	class ThreadPool {
	public:
		explicit ThreadPool(const size_t& max_threads);

		template<class Func, typename... Args>
		decltype(auto) submitTask(Func&& func, Args&&... args);
		void pause();
		void restart();
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
		std::queue<std::function<void()>> tasks;
		std::mutex queue_mtx;
		std::condition_variable cond_var;
		bool closed;
	};


	template<class Func, typename... Args>
	inline decltype(auto)
		ThreadPool::submitTask(Func&& func, Args&&...args)
	{
		using return_type = typename std::result_of<Func(Args...)>::type;

		auto task = std::make_shared<std::packaged_task<return_type()>>(
			[=]()
			->return_type {
			return func(std::forward<Args>(args)...);
		}
		);

		auto fut = task->get_future();
		{
			std::lock_guard<std::mutex> lock(this->queue_mtx);
			tasks.emplace([=]() {
				(*task)();
			});
		}
		return fut;
	}

}

#endif // !_THREAD_POOL_H
