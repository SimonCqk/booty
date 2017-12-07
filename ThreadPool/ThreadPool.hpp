#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include"./detail/ThreadPool_impl.h"

namespace concurrentlib {

	class ThreadPool {
	public:
		explicit ThreadPool(const size_t& max_threads)
			:_pool(std::make_shared<ThreadPool_impl>(max_threads)) {}

		template<class Func, typename... Args>
		decltype(auto) submitTask(Func&& func, Args&&... args) {
			return _pool->submitTask(std::forward<Func>(func), std::forward<Args>(args)...);
		}

		void pause() {
			_pool->pause();
		}

		void unpause() {
			_pool->unpause();
		}

		void close() {
			_pool->close();
		}

		bool isClosed() const {
			return _pool->isClosed();
		}

		~ThreadPool() = default;

	private:
		// pimpl pattern.
		std::shared_ptr<ThreadPool_impl> _pool;
	};

}

#endif // !_THREAD_POOL_H
