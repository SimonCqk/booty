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
		explicit ThreadPool(const size_t& thread_count = (std::thread::hardware_concurrency() - 1));

		template<class Func,typename... Args>
		auto submitTask(Func&& func, Args&&... args)
			->std::future<typename std::result_of<Func(Args...)>::type>;

		bool isClosed();
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
		size_t core_thread_count;
		bool closed;
	};


	template<class Func, typename ...Args>
	inline auto ThreadPool::submitTask(Func&& func, Args&&...args) 
		-> std::future<typename std::result_of<Func(Args...)>::type>
	{
		using return_type = typename std::result_of<Func(Args...)>::type;
		auto task = std::make_shared <std::packaged_task<return_type()>>{
			   [func = std::forward<Func>(func), args = std::forward<Args>(args)...]()->return_type{
        			  return_type res=func(args...);
		              return res;
			}
		};
		auto fut = task -> get_future();
		{
			std::lock_guard<std::mutex> lock(this->queue_mtx);
			tasks.emplace_back([=]() {
				(*task)();
			})
		}
		return fut;
	}

}
#endif // !_THREAD_POOL_H
