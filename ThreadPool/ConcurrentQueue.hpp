#pragma once
#ifndef _CONCURRENT_QUEUE_HPP
#define _CONCURRENT_QUEUE_HPP

#include<memory>
#include"./detail/ConcurrentQueue_impl.hpp"

namespace concurrentlib {

	template<typename T>
	class ConcurrentQueue {
	public:
		ConcurrentQueue(const size_t max_elements);

	private:
		std::shared_ptr<ConcurrentQueue_impl<T>> queue;
	};
}

#endif // !_CONCURRENT_QUEUE_HPP
