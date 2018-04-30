#include"./ThreadPool_impl.h"

size_t concurrentlib::ThreadPool_impl::core_thread_count = std::thread::hardware_concurrency() - 1;

concurrentlib::ThreadPool_impl::ThreadPool_impl(const size_t & max_threads)
	: closed(false), paused(false),
	max_thread_count(2 * std::thread::hardware_concurrency())
{
	if (max_threads <= 0) {
		max_thread_count = core_thread_count;
		throw std::runtime_error("Invalid thread-number passed in.");
	}
	size_t t_count = core_thread_count;
	if (max_threads < core_thread_count) {
		max_thread_count = core_thread_count;
		t_count = max_threads;
	}
	else if (max_threads < max_thread_count)
		// to limit number of working threads,
		// the maximum value can only be (2 * hardware threads).
		max_thread_count = max_threads;

	// pre-launch some threads.
	for (size_t i = 0; i < t_count; ++i) {
		_launchNew();
	}
	// lanuch sheduler and running background.
	std::thread scheduler = std::thread(&ThreadPool_impl::_scheduler, this);
	scheduler.detach();
}

bool concurrentlib::ThreadPool_impl::isClosed() const
{
	return this->closed;
}

void concurrentlib::ThreadPool_impl::pause()
{
	std::mutex mtx;
	std::lock_guard<std::mutex> lock(mtx);
	paused = true;
}

void concurrentlib::ThreadPool_impl::unpause()
{
	{
		std::mutex mtx;
		std::lock_guard<std::mutex> lock(mtx);
		paused = false;
	}
	cond_var.notify_all();
}

void concurrentlib::ThreadPool_impl::close()
{
	if (!closed) {
		{
			std::mutex mtx;
			std::lock_guard<std::mutex> lock(mtx);
			closed = true;
		}
		cond_var.notify_all();  // notify all threads to trigger `return`.
		for (auto& thread : threads)
				thread.join();
	}
}

concurrentlib::ThreadPool_impl::~ThreadPool_impl()
{
	close();
}

void concurrentlib::ThreadPool_impl::_scheduler()
{
	// find new task and notify one free thread to execute.
	while (!this->closed) {  // auto-exit when close.
		if (this->paused) {
			std::unique_lock<std::mutex> pause_lock(this->pause_mtx);
			cond_var.wait(pause_lock, [this] {
				return !this->paused;
			});
		}
		/*
		if (tasks.empty())  // if tasks-size > max_threads , reschedule the execution of threads.
			std::this_thread::yield();
		*/
		if (tasks.size() <= threads.size())
			cond_var.notify_one();
		else if (tasks.size() < max_thread_count) {
			_launchNew();
			cond_var.notify_one();
		}
	}
}

void concurrentlib::ThreadPool_impl::_launchNew()
{
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
				if (this->closed)  // exit when close.
					return;
				tasks.dequeue(task);
				task();  // execute task.
			}
		}
		);
	}
}
