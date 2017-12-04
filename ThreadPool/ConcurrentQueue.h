#pragma once
#ifndef _CONCURRENT_QUEUE_H
#define _CONCURRENT_QUEUE_H

#include"./detail/ConcurrentQueue_impl.h"

namespace concurrentlib {
	class ConcurrentQueue {
	public:
		ConcurrentQueue(const size_t max_elements);

	private:

	};
}

#endif // !_CONCURRENT_QUEUE_H
