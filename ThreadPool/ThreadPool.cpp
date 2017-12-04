#include"./ThreadPool.h"

void concurrentlib::ThreadPool::pause()
{
	_pool->pause();
}

void concurrentlib::ThreadPool::unpause()
{
	_pool->unpause();
}

void concurrentlib::ThreadPool::close()
{
	_pool->close();
}

bool concurrentlib::ThreadPool::isClosed() const
{
	return _pool->isClosed();
}
