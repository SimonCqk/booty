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
		static constexpr float threshold_factor = 1.5;
		size_t max_thread_count;
		// threads-manager
		std::vector<std::thread> threads;
		// tasks-queue
		concurrency::UnboundedLockQueue<std::function<void()>> tasks;
		// for synchronization
		std::mutex queue_mtx;
		std::mutex pause_mtx;
		std::condition_variable cond_var;
		AtomicBool paused;
		AtomicBool closed;
	public:
		ThreadPool()
			: max_thread_count(core_threshold) {
			this->paused.store(false, std::memory_order_relaxed);
			this->closed.store(false, std::memory_order_relaxed);

			// TODO
		}

		explicit ThreadPool(const size_t& max_threads)
			:max_thread_count((max_threads > core_threshold ? core_threshold : max_threads)) {
			this->paused.store(false, std::memory_order_relaxed);
			this->closed.store(false, std::memory_order_relaxed);

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
			if (this->closed || this->paused)
				throw std::runtime_error("Do not allow executing tasks after closed or paused.");

			tasks.enqueue([=]() {  // `=` mode instead of `&` to avoid ref-dangle.
				(*task)();
			});
			return fut;
		}

		void pause() {
			this->paused.store(true);
		}

		void unpause() {
			this->paused.store(false);
			cond_var.notify_all();
		}

		void close() {
			if (!this->closed) {
				this->closed.store(true);
				cond_var.notify_all();  // notify all threads to trigger `return`.
				for (auto& thread : threads)
					thread.join();
			}
		}

		bool isClosed() const {
			return this->closed;
		}

		~ThreadPool() {
			this->close();
		}

	private:
		void scheduler() {
			// find new task and notify one free thread to execute.
			while (!this->closed.load(std::memory_order_relaxed)) {  // auto-exit when close.
				if (this->paused.load(std::memory_order_relaxed)) {
					std::unique_lock<std::mutex> pause_lock(this->pause_mtx);
					cond_var.wait(pause_lock, [this] {
						return !this->paused.load(std::memory_order_relaxed);
					});
				}
				/*
				if (tasks.empty())  // if tasks-size > max_threads , reschedule the execution of threads.
				std::this_thread::yield();
				*/
				if (tasks.size() <= threads.size())
					cond_var.notify_one();
				else if (tasks.size() < max_thread_count) {
					launchNew();
					cond_var.notify_one();
				}
			}
		}

		void launchNew() {
			if (threads.size() < max_thread_count) {
				threads.emplace_back([this] {
					while (true) {
						if (this->paused) {
							std::unique_lock<std::mutex> pause_lock(this->pause_mtx);
							cond_var.wait(pause_lock, [this] {
								return !this->paused;
							});
						}
						std::function<void()> task;
						{
							std::unique_lock<std::mutex> lock(this->queue_mtx);
							cond_var.wait(lock, [this] {
								return this->closed || !this->tasks.empty();  // trigger when close or new task comes.
							});
						}
						if (this->closed.load(std::memory_order_relaxed))  // exit when close.
							return;
						tasks.dequeue(task);
						task();  // execute task.
					}
				}
				);
			}
		}
	};

	/// To limit number of working threads, set threshold as (2 * hardware threads).
	size_t ThreadPool::core_threshold = ThreadPool::threshold_factor * std::thread::hardware_concurrency();
}

#endif // !BOOTY_THREAD_POOL_H
