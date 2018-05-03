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

#include"concurrency/UnboundedLockQueue.hpp"

namespace booty {

	using AtomicBool = std::atomic<bool>;

	class ThreadPool {
	private:
		static size_t core_threshold;
		// threshold of maximum working threads == kThresholdFactor * hardware-threads
		static constexpr float kThresholdFactor = 1.5;
		// when num of current working threads * 3 < size of tasks, then launch new thread.
		static constexpr size_t kLaunchNewByTaskRate = 3;
		size_t max_thread_count;
		// threads-manager
		std::vector<std::thread> threads;
		// tasks-queue
		concurrency::UnboundedLockQueue<std::function<void()>> tasks;
		// for synchronization
		std::mutex pause_mtx;
		std::mutex queue_mtx;
		std::condition_variable cond_var;
		AtomicBool paused;
		AtomicBool closed;
	public:
		ThreadPool()
			: max_thread_count(core_threshold) {
			paused.store(false, std::memory_order_relaxed);
			closed.store(false, std::memory_order_relaxed);

			// TODO
		}

		explicit ThreadPool(const size_t& max_threads)
			:max_thread_count((max_threads > core_threshold ? core_threshold : max_threads)) {
			paused.store(false, std::memory_order_relaxed);
			closed.store(false, std::memory_order_relaxed);

			// pre-launch some threads.
			for (size_t i = 0; i < max_thread_count / 2; ++i) {
				launchNew();
			}
			// lanuch sheduler and running background.
			std::thread scheduler = std::thread(&ThreadPool::scheduler, this);
			scheduler.detach();
		}

		template<class Func, typename... Args>
		inline decltype(auto) submitTask(Func&& func, Args&&... args) {
			using return_type = typename std::invoke_result_t<Func, Args...>;

			auto task = std::make_shared<std::packaged_task<return_type()>>(
				[func = std::forward<Func>(func),
				args = std::make_tuple(std::forward<Args>(args)...)]()->return_type{
				return std::apply(func, args);
			}
			);

			auto fut = task->get_future();
			if (closed.load(std::memory_order_relaxed) || paused.load(std::memory_order_relaxed))
				throw std::runtime_error("Do not allow executing tasks after closed or paused.");

			tasks.enqueue([task]() {  // `=` mode instead of `&` to avoid ref-dangle.
				(*task)();
			});
			return fut;
		}

		void pause() {
			paused.store(true);
		}

		void unpause() {
			paused.store(false);
			cond_var.notify_all();
		}

		void close() {
			if (!closed.load()) {
				closed.store(true);
				cond_var.notify_all();  // notify all threads to trigger `return`.
				for (auto& thread : threads)
					thread.join();
			}
		}

		bool isClosed() const {
			return closed.load();
		}

		~ThreadPool() {
			close();
		}

	private:
		void scheduler() {
			// find new task and notify one free thread to execute.
			while (!closed.load(std::memory_order_relaxed)) {  // exit when close.
				if (paused.load(std::memory_order_relaxed)) {
					std::unique_lock<std::mutex> pause_lock(pause_mtx);
					cond_var.wait(pause_lock, [this] {
						return !paused.load(std::memory_order_relaxed);
					});
				}

				if (threads.size() * kLaunchNewByTaskRate < tasks.size()) {
					cond_var.notify_one();
				}
				else {
					launchNew();
					cond_var.notify_one();
				}
			}
		}

		void launchNew() {
			if (threads.size() < max_thread_count) {
				threads.emplace_back([this] {
					while (true) {
						if (paused.load(std::memory_order_relaxed)) {
							std::unique_lock<std::mutex> pause_lock(pause_mtx);
							cond_var.wait(pause_lock, [this] {
								return !paused.load(std::memory_order_relaxed);
							});
						}
						std::function<void()> task;
						{
							std::unique_lock<std::mutex> lock(queue_mtx);
							cond_var.wait(lock, [this] {
								return !tasks.empty() || closed.load(std::memory_order_relaxed);
							});
							if (closed.load(std::memory_order_relaxed))
								return;
						}
						tasks.dequeue(task);  // queue should block when it's empty  
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
