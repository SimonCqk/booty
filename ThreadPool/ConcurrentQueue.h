#pragma once
#ifndef _CONCURRENT_QUEUE_H
#define _CONCURRENT_QUEUE_H

#include<memory>
#include"./detail/ConcurrentQueue_impl.h"

namespace concurrentlib {
	class ConcurrentQueue {
	public:
		ConcurrentQueue(const size_t max_elements);

	private:
		std::shared_ptr<ConcurrentQueue_impl> queue;
	};
}

#endif // !_CONCURRENT_QUEUE_H
