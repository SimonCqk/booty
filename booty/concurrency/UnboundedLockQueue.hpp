#ifndef BOOTY_CONCURRENCY_UNBOUNDEDLOCKQUEUE_H
#define BOOTY_CONCURRENCY_UNBOUNDEDLOCKQUEUE_H


#include<queue>
#include<shared_mutex>
#include<atomic>
#include<condition_variable>

#include<iostream>

namespace booty {

	namespace concurrency {

		/// UnboundedLockQueue provides thread-safe operations with STL lock.
		/// enqueue/dequeue never blocks. If neccessary, block measures should
		/// be adopted by caller/invoker.

		template<typename T>
		class UnboundedLockQueue {
		public:
			UnboundedLockQueue() {}

			/* enqueue one element(lvalue). */
			void enqueue(const T& ele) {
				std::lock_guard<std::shared_mutex> lock(queue_mtx_);
				queue_.push(ele);
			}

			/*
			  enqueue one element(rvalue).
			*/
			void enqueue(T&& ele) {
				std::lock_guard<std::shared_mutex> lock(queue_mtx_);
				queue_.emplace(std::move(ele));
			}

			/*
			  dequeue one element, pass by reference.
			  return by a local value is not exception-safe, and when queue is empty,
			  it just throw an error by std::queue, but never block to wait for producer,
			  as it mentioned above: block measures should be adopted by caller/invoker.
			*/
			void dequeue(T& recv) {
				std::lock_guard<std::shared_mutex> lock(queue_mtx_);
				recv = std::move(queue_.front());
				queue_.pop();
			}

			/* get size of queue */
			size_t size() const {
				std::shared_lock<std::shared_mutex> lock(queue_mtx_);
				return queue_.size();
			}

			/* judge if queue is empty */
			bool empty() const {
				std::shared_lock<std::shared_mutex> lock(queue_mtx_);
				return queue_.empty();
			}

			/* forbid copy ctor and copy operator */
			UnboundedLockQueue(const UnboundedLockQueue&) = delete;
			UnboundedLockQueue& operator=(const UnboundedLockQueue&) = delete;

		private:
			std::queue<T> queue_;
			std::shared_mutex queue_mtx_;
		};
	}
}

#endif // !BOOTY_CONCURRENCY_UNBOUNDEDLOCKQUEUE_H
