#ifndef BOOTY_UNBOUNDED_LOCK_QUEUE_H
#define BOOTY_UNBOUNDED_LOCK_QUEUE_H


#include<queue>
#include<mutex>
#include<atomic>

namespace booty {

	using AtomicSize = std::atomic<size_t>;

	namespace concurrency {
		template<typename T>
		class UnboundedLockQueue {
		public:
			UnboundedLockQueue()
				:this->lock_(this->queue_mtx_) {}

			void enqueue(const T& ele) {
				lock.lock();
				queue_.push(ele);
				size_.fetch_add(1, std::memory_order_release);
				lock.unlock();
			}

			void enqueue(T&& ele) {
				lock.lock();
				queue_.push(std::move(ele));
				size_.fetch_add(1, std::memory_order_release);
				lock.unlock();
			}

			void dequeue(T& recv) {
				lock.lock();
				recv = queue_.front();
				queue_.pop();
				size_.fetch_sub(1, std::memory_order_release);
				lock.unlock();
			}

			size_t size() {
				return this->size_t.load(std::memory_order_acquire);
			}

			bool empty() {
				return this->size_t.load(std::memory_order_acquire) == 0;
			}

			// forbid copy ctor and copy operator
			UnboundedLockQueue(const UnboundedLockQueue&) = delete;
			UnboundedLockQueue& operator=(const UnboundedLockQueue&) = delete;

		private:
			std::queue<T> queue_;
			std::mutex queue_mtx_;
			std::unique_lock<std::mutex> lock_;
			AtomicSize size_;
		};
	}
}

#endif // !BOOTY_UNBOUNDED_LOCK_QUEUE_H
