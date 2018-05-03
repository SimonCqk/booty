#ifndef BOOTY_CONCURRENCY_UNBOUNDEDLOCKQUEUE_H
#define BOOTY_CONCURRENCY_UNBOUNDEDLOCKQUEUE_H


#include<queue>
#include<mutex>
#include<atomic>
#include<condition_variable>

#include<iostream>

namespace booty {

	using AtomicSize = std::atomic<size_t>;

	namespace concurrency {

		/// UnboundedLockQueue provides thread-safe operations with STL lock.
		/// enqueue/dequeue never blocks. If neccessary, block measures should
		/// be adopted by caller/invoker.

		template<typename T>
		class UnboundedLockQueue {
		public:
			UnboundedLockQueue()
				:size_(0) {}

			/* enqueue one element(lvalue). */
			void enqueue(const T& ele) {
				std::lock_guard<std::mutex> lock(this->queue_mtx_);
				queue_.push(ele);
				size_.fetch_add(1, std::memory_order_release);
			}

			/*
			  enqueue one element(rvalue).
			*/
			void enqueue(T&& ele) {
				std::lock_guard<std::mutex> lock(this->queue_mtx_);
				queue_.emplace(std::move(ele));
				size_.fetch_add(1, std::memory_order_release);
			}

			/*
			  dequeue one element, pass by reference.
			  return by a local value is not exception-safe, and when queue is empty,
			  it just throw an error by std::queue, but never block to wait for producer,
			  as it mentioned above: block measures should be adopted by caller/invoker.
			*/
			void dequeue(T& recv) {
				std::lock_guard<std::mutex> lock(this->queue_mtx_);
				recv = std::move(queue_.front());
				queue_.pop();
				size_.fetch_sub(1, std::memory_order_release);
			}

			/* get size of queue */
			size_t size() {
				return this->size_.load(std::memory_order_relaxed);
			}

			/* judge if queue is empty */
			bool empty() {
				return this->size_.load(std::memory_order_relaxed) == 0;
			}

			/* forbid copy ctor and copy operator */
			UnboundedLockQueue(const UnboundedLockQueue&) = delete;
			UnboundedLockQueue& operator=(const UnboundedLockQueue&) = delete;

		private:
			std::queue<T> queue_;
			std::mutex queue_mtx_;
			// use atomic var to avoid construct and destruct a lock everytime when
			// size() and empty() was invoked.
			AtomicSize size_;
		};
	}
}

#endif // !BOOTY_CONCURRENCY_UNBOUNDEDLOCKQUEUE_H
