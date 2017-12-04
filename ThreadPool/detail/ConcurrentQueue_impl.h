#pragma once
#ifndef _CONCURRENT_QUEUE_IMPL_H
#define _CONCURRENT_QUEUE_IMPL_H

#include<thread>
#include<atomic>
#include<condition_variable>

namespace concurrentlib {

	class ConcurrentQueue_impl {
		//define some type traits & alias.
		using index_t = std::size_t;

	public:
		ConcurrentQueue_impl(const size_t& max_elements);
	private:
		std::condition_variable cond_empty;

	};
}

#endif // !_CONCURRENT_QUEUE_IMPL_H
