#ifndef _CONCURRENT_QUEUE_IMPL_HPP
#define _CONCURRENT_QUEUE_IMPL_HPP

#include<array>
#include<chrono>
#include<thread>
#include<atomic>
#include<condition_variable>
#include<mutex>
#include<cassert>

namespace concurrentlib {

	template<typename T>
	class ConcurrentQueue_impl {
		struct ListNode;
		struct ContLinkedList;

		//define some type traits and constants.
		using index_t = std::size_t;
		using asize_t = std::atomic_size_t;
		using abool = std::atomic_bool;
		using aListNode_p = std::atomic<ListNode*>;

		constexpr size_t SUB_QUEUE_NUM = 8;
		constexpr size_t PRE_ALLOC_NODE_NUM = 1024;
		constexpr size_t NEXT_ALLOC_NODE_NUM = 32;

		// define free-list structure.
		struct ListNode
		{
			T data;
			aListNode_p next;
			abool isHold;
			ListNode() {
				next.store(nullptr, std::memory_order_relaxed);
				isHold.store(false, std::memory_order_relaxed);
			}

			template<typename Ty>
			ListNode(Ty&& val)
				:data(std::forward<Ty>(val)) {
				next.store(nullptr, std::memory_order_relaxed);
				isHold.store(false, std::memory_order_relaxed);
			}
		};

		struct ContLinkedList  // concurrent linked-list.
		{
			aListNode_p head;
			aListNode_p tail;
			ContLinkedList() {
				head.store(nullptr);
				tail.store(head.load(std::memory_order_relaxed), std::memory_order_relaxed);  // init: head = tail
			}
			bool isEmpty() {
				return head.load() == tail.load()&&(head.load()&&tail.load());
			}
		};

	public:
		/*
		  if no number of elements passed in.
		  it will be a unbounded queue.
		*/
		ConcurrentQueue_impl();
		/*
		  if a concrete number passed in.
		  The size of queue will be limited under max_elements.
		*/
		ConcurrentQueue_impl(const size_t& num_elements);

		T& front();
		void enqueue(const T& data);
		void enqueue(T&& data);
		T dequeue();
		void dequeue(T& data);

		size_t size() const {
			return _size.load();
		}
		bool empty() const {
			return _size.load() == 0;
		}

		~ConcurrentQueue_impl();

		// forbid copy ctor & copy operator.
		ConcurrentQueue_impl(const ConcurrentQueue_impl& queue) = delete;
		ConcurrentQueue_impl& operator=(const ConcurrentQueue_impl& queue) = delete;

	private:

		size_t _choose() {
			return _size.load() % SUB_QUEUE_NUM;
		}

		size_t _getDequeueIndex() {
			return _dequeue_idx.load() % SUB_QUEUE_NUM;
		}

		// return the tail of linked list.
		ListNode* acquireOrAlloc(ContLinkedList& list) {
			ListNode* _tail = nullptr;
			while (!_tail) {
				_tail = list.tail.load(std::memory_order_acquire);
			}
			// if nodes are running out, then allocate some new nodes.
			if (!_tail->next) {
				std::atomic_thread_fence(std::memory_order_release);
				ListNode* tail_copy = _tail;
				for (int i = 0; i < NEXT_ALLOC_NODE_NUM; ++i) {
					tail_copy->next.store(new ListNode(T()), std::memory_order_release);
					tail_copy.store(tail_copy->next.load(std::memory_order_acquire), std::memory_order_release);
				}
			}
			return _tail;
		}
		template<typename Ty>
		bool tryEnqueue(Ty&& data);
		bool tryDequeue();
		// only for blocking when queue is empty or reach max size.
		std::condition_variable cond_empty;
		std::condition_variable cond_maxsize;
		std::mutex mtx_empty;
		std::mutex mtx_maxsize;

		std::array<ContLinkedList, SUB_QUEUE_NUM> sub_queues;
		asize_t _size;
		asize_t _dequeue_idx;
		size_t max_elements;
	};

	template<typename T>
	inline ConcurrentQueue_impl<T>::ConcurrentQueue_impl()
		:max_elements(-1) {
		_size.store(0, std::memory_order_relaxed);
		_dequeue_idx.store(-1, std::memory_order_relaxed);

		// pre-allocate PRE_ALLOC_NODE_NUM nodes.
		for (auto& queue : sub_queues) {
			for (int i = 0; i < PRE_ALLOC_NODE_NUM / SUB_QUEUE_NUM; ++i) {
				if (queue.isEmpty()) {// which means the linked-list is empty.
					head.store(new ListNode(T()), std::memory_order_release);
					std::assert(head.load() == tail.load());
				}
				else {
					queue.tail->next.store(new ListNode(T()), std::memory_order_release);
					queue.tail.store(queue.tail->next.load(std::memory_order_acquire), std::memory_order_release);
				}
			}
			queue.tail.store(queue.head.load());
			std::assert(head.load() == tail.load());
		}
		// block when queue is empty
		std::unique_lock<std::mutex> lock(mtx_empty);
		cond_empty.wait(lock, [this] {
			return !this->empty();
		});
	}

	template<typename T>
	inline ConcurrentQueue_impl<T>::ConcurrentQueue_impl(const size_t & num_elements)
		:max_elements(num_elements) {
		_size.store(0, std::memory_order_relaxed);
		_dequeue_idx.store(0, std::memory_order_relaxed);

		// allocate num_elements nodes.
		for (auto& queue : sub_queues) {
			for (int i = 0; i < max_elements / SUB_QUEUE_NUM; ++i) {
				if (queue.isEmpty()) {
					head.store(new ListNode(T()), std::memory_order_release);
					std::assert(head.load() == tail.load());
				}
				else {
					queue.tail->next.store(new ListNode(T()), std::memory_order_release);
					queue.tail.store(queue.tail->next.load(std::memory_order_acquire), std::memory_order_release);
				}
			}
			queue.tail.store(queue.head.load());
			std::assert(head.load() == tail.load());
		}
		// block when queue is empty
		std::unique_lock<std::mutex> lock(mtx_empty);
		cond_empty.wait(lock, [this] {
			return !this->empty();
		});
	}


} // namespace concurrentlib

#endif // !_CONCURRENT_QUEUE_IMPL_HPP
