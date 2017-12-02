#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include"./detail/ThreadPool_impl.h"

namespace thread_pool {

	class ThreadPool {
	public:
		explicit ThreadPool(const size_t& max_threads)
			:_pool(std::make_shared<ThreadPool_impl>(max_threads)) {}

		template<class Func, typename... Args>
		decltype(auto) submitTask(Func&& func, Args&&... args);
		void pause();
		void unpause();
		void close();
		bool isClosed() const;
		~ThreadPool() = default;

	private:
		// pimpl pattern.
		std::shared_ptr<ThreadPool_impl> _pool;
	};

	template<class Func, typename ...Args>
	inline decltype(auto) ThreadPool::submitTask(Func && func, Args && ...args)
	{
		return _pool->submitTask(std::forward<Func>(func), std::forward<Args>(args)...);
	}
}

#endif // !_THREAD_POOL_H
