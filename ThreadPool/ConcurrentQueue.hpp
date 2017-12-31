#pragma once
#ifndef _CONCURRENT_QUEUE_HPP
#define _CONCURRENT_QUEUE_HPP

#include<memory>
#include"./detail/ConcurrentQueue_impl.hpp"

namespace concurrentlib {

	template<typename T>
	class ConcurrentQueue {
	public:
		// ctors.
		ConcurrentQueue()
			:queue(std::make_shared<ConcurrentQueue_impl<T>>()) {}

		// sync interfaces with core implementation class.
		T& front() {
			queue->front();
		}

		void enqueue(const T& data) {
			queue->enqueue(data);
		}

		void enqueue(T&& data) {
			queue->enqueue(std::forward<T>(data));
		}

		void dequeue(T& data) {
			queue->dequeue(data);
		}

		size_t size() {
			return queue->size();
		}

		bool empty() {
			return queue->empty();
		}

		~ConcurrentQueue() = default;

		// forbid copy ctor & copy operator.
		ConcurrentQueue(const ConcurrentQueue& queue) = delete;
		ConcurrentQueue& operator=(const ConcurrentQueue& queue) = delete;
	private:
		std::shared_ptr<ConcurrentQueue_impl<T>> queue;
	};
}

#endif // !_CONCURRENT_QUEUE_HPP
