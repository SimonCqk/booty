#include"../ThreadPool/detail/ConcurrentQueue_impl.hpp"

using namespace concurrentlib;

void single_thread() {
	ConcurrentQueue_impl<int> queue;
	queue.enqueue(1);
	std::cout << queue.dequeue() << std::endl;
}

int main() {
	single_thread();
	return 0;
}