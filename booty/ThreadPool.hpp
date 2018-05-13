#ifndef BOOTY_THREAD_POOL_H
#define BOOTY_THREAD_POOL_H

#include<thread>
#include<future>
#include<condition_variable>
#include<vector>
#include<tuple>
#include<functional>
#include<utility>
#include<memory>
#include<deque>
//#include"concurrency/UnboundedLockQueue.hpp"

namespace booty {

	using AtomicBool = std::atomic<bool>;

	class ThreadPool {
	private:
		static size_t core_threshold;
		// threshold of maximum working threads == kThresholdFactor * hardware-threads
		static constexpr float kThresholdFactor = 1.5;
		// when num of current working threads * 3 < size of tasks, then launch new thread.
		static constexpr size_t kLaunchNewByTaskRate = 3;
		size_t max_thread_count_;
		// threads-manager
		std::vector<std::thread> threads_;
		// tasks-queue
		std::deque<std::function<void()>> tasks_;
		// for synchronization
		std::mutex pause_mtx_;
		std::mutex queue_mtx_;
		std::condition_variable cond_var_;
		AtomicBool paused_;
		AtomicBool closed_;
	public:
		ThreadPool()
			: max_thread_count_(core_threshold) {
			paused_.store(false, std::memory_order_relaxed);
			closed_.store(false, std::memory_order_relaxed);

			// pre-launch some threads.
			for (size_t i = 0; i < max_thread_count_ / 2; ++i) {
				launchNew();
			}
			// lanuch sheduler and running background.
			std::thread scheduler = std::thread(&ThreadPool::scheduler, this);
			scheduler.detach();
		}

		explicit ThreadPool(const size_t& max_threads)
			:max_thread_count_((max_threads > core_threshold ? core_threshold : max_threads)) {
			paused_.store(false, std::memory_order_relaxed);
			closed_.store(false, std::memory_order_relaxed);

			// pre-launch some threads.
			for (size_t i = 0; i < max_thread_count_ / 2; ++i) {
				launchNew();
			}
			// lanuch sheduler and running background.
			std::thread scheduler = std::thread(&ThreadPool::scheduler, this);
			scheduler.detach();
		}

		template<class CondFunc, typename... Args>
		inline decltype(auto) submitTask(CondFunc&& func, Args&&... args) {
			using return_type = typename std::invoke_result_t<CondFunc, Args...>;

			auto task = std::make_shared<std::packaged_task<return_type()>>(
				[func = std::forward<CondFunc>(func),
				args = std::make_tuple(std::forward<Args>(args)...)]()->return_type{
				return std::apply(func, args);
			}
			);

			auto fut = task->get_future();
			if (closed_.load(std::memory_order_relaxed) || paused_.load(std::memory_order_relaxed))
				throw std::runtime_error("Do not allow executing tasks_ after closed_ or paused_.");

			tasks_.emplace_back([task]() {  // `=` mode instead of `&` to avoid ref-dangle.
				(*task)();
			});
			return fut;
		}

		void pause() {
			paused_.store(true);
		}

		void unpause() {
			paused_.store(false);
			cond_var_.notify_all();
		}

		void close() {
			if (!closed_.load()) {
				closed_.store(true);
				cond_var_.notify_all();  // notify all threads to trigger `return`.
				for (auto& thread : threads_)
					thread.join();
			}
		}

		bool isClosed() const {
			return closed_.load();
		}

		~ThreadPool() {
			close();
		}

	private:
		void scheduler() {
			// find new task and notify one free thread to execute.
			while (!closed_.load(std::memory_order_relaxed)) {  // exit when close.
				if (paused_.load(std::memory_order_relaxed)) {
					std::unique_lock<std::mutex> pause_lock(pause_mtx_);
					cond_var_.wait(pause_lock, [this] {
						return !paused_.load(std::memory_order_relaxed);
					});
				}

				if (threads_.size() * kLaunchNewByTaskRate < tasks_.size()) {
					cond_var_.notify_one();
				}
				else {
					launchNew();
					cond_var_.notify_one();
				}
			}
		}

		void launchNew() {
			if (threads_.size() < max_thread_count_) {
				threads_.emplace_back([this] {
					while (true) {
						if (paused_.load(std::memory_order_relaxed)) {
							std::unique_lock<std::mutex> pause_lock(pause_mtx_);
							cond_var_.wait(pause_lock, [this] {
								return !paused_.load(std::memory_order_relaxed);
							});
						}
						std::function<void()> task;
						{
							std::unique_lock<std::mutex> lock(queue_mtx_);
							cond_var_.wait(lock, [this] {
								return !tasks_.empty() || closed_.load(std::memory_order_relaxed);
							});
							if (closed_.load(std::memory_order_relaxed)) {
								lock.unlock();
								return;
							}
							task = std::move(tasks_.front());
							tasks_.pop_front();
						}
						task();  // execute task.
					}
				}
				);
			}
		}
	};

	/// To limit number of working threads, set threshold as (2 * hardware threads).
	size_t ThreadPool::core_threshold = ThreadPool::kThresholdFactor * std::thread::hardware_concurrency();
}

#endif // !BOOTY_THREAD_POOL_H
