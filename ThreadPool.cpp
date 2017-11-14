#include "ThreadPool.h"

thread_pool::ThreadPool::ThreadPool(const size_t & thread_count = (std::thread::hardware_concurrency() - 1))
	: closed(false)
{
	for (int i = 0; i < thread_count; ++i) {
		threads.emplace_back([this] {
			while (!this->closed) {  // exit when close.
				std::function<void()> task;
				{
					std::unique_lock<std::mutex> lock(this->queue_mtx);
					cond_var.wait(lock, [this] {
						return this->closed||!this->tasks.empty();
					});
					task = std::move(this->tasks.front());
					this->tasks.pop();
				}
			}
		});
	}
	std::thread scheduler = std::thread(&ThreadPool::_scheduler, this);
	scheduler.detach();
}

void thread_pool::ThreadPool::close()
{
	if (!closed) {
		closed = true;
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

}
