#include "ThreadPool.h"

void thread_pool::ThreadPool::pause()
{
	_pool->pause();
}

void thread_pool::ThreadPool::unpause()
{
	_pool->unpause();
}

void thread_pool::ThreadPool::close()
{
	_pool->close();
}

bool thread_pool::ThreadPool::isClosed() const
{
	return _pool->isClosed();
}
