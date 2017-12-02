#include"./ThreadPool.h"

void concurrency::ThreadPool::pause()
{
	_pool->pause();
}

void concurrency::ThreadPool::unpause()
{
	_pool->unpause();
}

void concurrency::ThreadPool::close()
{
	_pool->close();
}

bool concurrency::ThreadPool::isClosed() const
{
	return _pool->isClosed();
}
