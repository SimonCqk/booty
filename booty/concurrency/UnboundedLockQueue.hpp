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
		template<typename T>
		class UnboundedLockQueue {
		public:
			UnboundedLockQueue()
				:size_(0) {}

			void enqueue(const T& ele) {
				std::lock_guard<std::mutex> lock(this->queue_mtx_);
				queue_.push(ele);
				size_.fetch_add(1, std::memory_order_release);
			}

			void enqueue(T&& ele) {
				std::lock_guard<std::mutex> lock(this->queue_mtx_);
				queue_.emplace(std::move(ele));
				size_.fetch_add(1, std::memory_order_release);
			}

			void dequeue(T& recv) {
				std::lock_guard<std::mutex> lock(this->queue_mtx_);
				recv = std::move(queue_.front());
				queue_.pop();
				size_.fetch_sub(1, std::memory_order_release);
			}

			size_t size() {
				return this->size_.load(std::memory_order_relaxed);
			}

			bool empty() {
				return this->size_.load(std::memory_order_relaxed) == 0;
			}

			// forbid copy ctor and copy operator
			UnboundedLockQueue(const UnboundedLockQueue&) = delete;
			UnboundedLockQueue& operator=(const UnboundedLockQueue&) = delete;

		private:
			std::queue<T> queue_;
			std::mutex queue_mtx_;
			AtomicSize size_;
		};
	}
}

#endif // !BOOTY_CONCURRENCY_UNBOUNDEDLOCKQUEUE_H
