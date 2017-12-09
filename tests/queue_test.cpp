#include"../ThreadPool/detail/ConcurrentQueue_impl.hpp"
#include"../ThreadPool/ConcurrentQueue.hpp"

using namespace concurrentlib;

void single_thread() {
	ConcurrentQueue<int> queue;
	queue.enqueue(1);
	queue.enqueue(2);
	std::cout << queue.dequeue() << std::endl;
	std::cout << queue.dequeue() << std::endl;
}


int main() {
	single_thread();
	return 0;
}