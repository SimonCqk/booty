#pragma once
#ifndef _THREAD_POOL_IMPL_H
#define _THREAD_POOL_IMPL_H

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

namespace concurrentlib {

	class ThreadPool_impl {
	public:
		explicit ThreadPool_impl(const size_t& max_threads);

		template<class Func, typename... Args>
		decltype(auto) submitTask(Func&& func, Args&&... args);
		void pause();
		void unpause();
		void close();
		bool isClosed() const;
		~ThreadPool_impl();
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
		ThreadPool_impl::submitTask(Func&& func, Args&&...args)
	{

		using return_type = typename std::result_of_t<Func(Args...)>;

		auto task = std::make_shared<std::packaged_task<return_type()>>(
			[func = std::forward<Func>(func),
			args = std::make_tuple(std::forward<Args>(args)...)]()->return_type{
			return std::apply(func, args);
		}
		);

		auto fut = task->get_future();
		{
			std::lock_guard<std::mutex> lock(this->queue_mtx);
			if (this->closed || this->paused)
				throw std::runtime_error("Do not allow executing tasks after closed or paused.");
			tasks.emplace([=]() {  // `=` mode instead of `&` to avoid ref-dangle.
				(*task)();
			});
		}
		return fut;
	}
}

#endif // !_THREAD_POOL_IMPL_H