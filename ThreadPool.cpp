#include "ThreadPool.h"
#include<exception>

size_t thread_pool::ThreadPool::core_thread_count = std::thread::hardware_concurrency() - 1;

thread_pool::ThreadPool::ThreadPool(const size_t & max_threads)
	: closed(false), max_thread_count(max_threads)
{
	if (max_threads <= 0) {
		max_thread_count = core_thread_count;
		throw std::exception("Invalid thread-number passed in.");
	}
	size_t t_count = core_thread_count;
	if (max_threads < core_thread_count) {
		max_thread_count = core_thread_count;
		t_count = max_threads;
	}
	// launch core-thread-count threads firstly.
	for (size_t i = 0; i < t_count; ++i) {
		_launchNew();
	}
	// lanuch sheduler and running background.
	std::thread scheduler = std::thread(&ThreadPool::_scheduler, this);
	scheduler.detach();
}

bool thread_pool::ThreadPool::isClosed() const
{
	return this->closed;
}

void thread_pool::ThreadPool::close()
{
	if (!closed) {
		{
			std::lock_guard<std::mutex> lock(this->queue_mtx);
			closed = true;
		}
		cond_var.notify_all();
		for (auto& thread : threads)
			if (thread.joinable())
				thread.join();
	}
}

thread_pool::ThreadPool::~ThreadPool()
{
	{
		std::lock_guard<std::mutex> lock(this->queue_mtx);
		closed = true;
	}
	cond_var.notify_all();
	for (auto& thread : threads)
		if (thread.joinable())
			thread.join();
}

void thread_pool::ThreadPool::_scheduler()
{
	// find new task and notify one free thread to execute.
	while (!this->closed) {  // auto-exit when close.
		if (tasks.empty() ||
			tasks.size() > max_thread_count)  // if tasks-size > max_threads , just loop for waiting.
			continue;
		else if (tasks.size() <= threads.size())
			cond_var.notify_one();
		else if (tasks.size() < max_thread_count) {
			_launchNew();
			cond_var.notify_one();
		}
	}
}

void thread_pool::ThreadPool::_launchNew()
{
	if (threads.size() < max_thread_count) {
		threads.emplace_back([this] {
			while (true) {
				std::function<void()> task;
				{
					std::unique_lock<std::mutex> lock(this->queue_mtx);
					cond_var.wait(lock, [this] {
						return this->closed || !this->tasks.empty();
					});
					if (this->closed)  // exit when close.
						return;
					task = std::move(this->tasks.front());
					this->tasks.pop();
				}
				task();
			}
		}
		);
	}
}
