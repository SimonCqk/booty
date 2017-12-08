#ifndef _CONCURRENT_QUEUE_IMPL_HPP
#define _CONCURRENT_QUEUE_IMPL_HPP

#include<iostream>
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

		static constexpr size_t SUB_QUEUE_NUM = 8;
		static constexpr size_t PRE_ALLOC_NODE_NUM = 1024;
		static constexpr size_t NEXT_ALLOC_NODE_NUM = 32;
		static constexpr size_t MAX_CONTEND_TRY_TIME = 100;

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
				head.store(nullptr, std::memory_order_relaxed);
				tail.store(head.load(std::memory_order_relaxed), std::memory_order_relaxed);  // init: _head = tail
			}
			bool isEmpty() {
				return head.load(std::memory_order_relaxed) == tail.load(std::memory_order_relaxed)
					&& (!head.load(std::memory_order_relaxed) && !tail.load(std::memory_order_relaxed));
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

		T& front() {
			auto& cur_queue = sub_queues[_getDequeueIndex()];
			ListNode* _head = cur_queue.head.load(std::memory_order_relaxed);
			while (!_head || _head->isHold.load(std::memory_order_acquire)) {
				std::this_thread::yield();
				_head = cur_queue.head.load(std::memory_order_acquire);
			}
			return _head->data;
		}

		void enqueue(const T& data) {
			if (max_elements > 0 && _size.load() >= max_elements) {
				std::lock_guard<std::mutex> lock(mtx_maxsize);
				cond_maxsize.wait(lock, [this] {
					return _size.load() < max_elements;
				});
			}
			if (this->empty())
				cond_empty.notify_one();
			while (!tryEnqueue(data));  // try until succeed.
		}

		void enqueue(T&& data) {
			if (max_elements > 0 && _size.load() >= max_elements) {
				std::unique_lock<std::mutex> lock(mtx_maxsize);
				cond_maxsize.wait(lock, [this] {
					return _size.load() < max_elements;
				});
			}
			if (this->empty())
				cond_empty.notify_one();
			while (!tryEnqueue(std::forward<T>(data)));  // try until succeed.
		}

		T dequeue() {
			T data;
			while (!tryDequeue(data));  // try until succeed.
			return std::move(data);
		}

		void dequeue(T& data) {
			while (!tryDequeue(data));  // try until succeed.
		}

		size_t size() const {
			return _size.load(std::memory_order_acquire);
		}
		bool empty() const {
			return _size.load(std::memory_order_acquire) == 0;
		}

		~ConcurrentQueue_impl() {
			for (auto& queue : sub_queues) {
				ListNode* head = queue.head.load(std::memory_order_relaxed);
				ListNode* next = head->next.load(std::memory_order_relaxed);
				while (next) {
					head = next;
					next = next->next.load(std::memory_order_relaxed);
					delete head;
				}
			}
		}

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
			ListNode* _tail = list.tail.load(std::memory_order_relaxed);
			size_t try_time = 0;
			while (!_tail || _tail->isHold.load(std::memory_order_acquire)) {
				std::this_thread::yield();
				_tail = list.tail.load(std::memory_order_acquire);
				if (++try_time > MAX_CONTEND_TRY_TIME)
					return nullptr;
			}
			_tail->isHold.store(true, std::memory_order_release);
			// if nodes are running out, then allocate some new nodes.
			if (!_tail->next) {
				std::atomic_thread_fence(std::memory_order_release);
				ListNode* tail_copy = _tail;
				for (int i = 0; i < NEXT_ALLOC_NODE_NUM; ++i) {
					tail_copy->next.store(new ListNode(T()), std::memory_order_release);
					tail_copy = tail_copy->next.load(std::memory_order_acquire);
				}
			}
			return _tail;
		}
		/*
		  try to enqueue, return true if succeed.
		*/
		template<typename Ty>
		bool tryEnqueue(Ty&& data) {
			auto& cur_queue = sub_queues[_choose()];
			ListNode* tail = acquireOrAlloc(cur_queue);
			if (!tail || !tail->isHold.load(std::memory_order_acquire))  // ensure it has been acquired successfully.
				return false;
			tail->next.load()->data = std::forward<Ty>(data);
			tail->isHold.store(false, std::memory_order_release);
			tail = tail->next.load(std::memory_order_relaxed);
			++_size;
			return true;
		}

		bool tryDequeue(T& data) {
			auto& cur_queue = sub_queues[_getDequeueIndex()];
			if (cur_queue.isEmpty())
				return false;
			ListNode* _head = cur_queue.head.load(std::memory_order_relaxed);
			size_t try_time = 0;
			std::cout << _head->data << std::endl;
			while (!_head || _head->isHold.load(std::memory_order_acquire)) {
				if (++try_time > MAX_CONTEND_TRY_TIME)
					return false;
				std::this_thread::yield();
				_head = cur_queue.head.load(std::memory_order_acquire);
			}
			if (!_head || _head->isHold.load(std::memory_order_acquire))  // ensure it has been acquired successfully.
				return false;
			_head->isHold.store(true, std::memory_order_release);
			data = _head->data;
			if (_head == cur_queue.tail.load()) {  // this sub-queue will be empty after dequeue.
				delete _head;
				_head = nullptr;
				cur_queue.tail.store(nullptr, std::memory_order_release);

				std::unique_lock<std::mutex> lock(mtx_empty);
				cond_empty.wait(lock, [this] {
					return !this->empty();
				});
			}
			else {
				_head->isHold.store(false, std::memory_order_release);
				ListNode* old_head = _head;
				_head = _head->next.load(std::memory_order_acquire);
				delete old_head; old_head = nullptr; // delete old head node.
			}
			--_size;
			++_dequeue_idx;
			return true;
		}

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
		_dequeue_idx.store(0, std::memory_order_relaxed);
		// pre-allocate PRE_ALLOC_NODE_NUM nodes.
		for (auto& queue : sub_queues) {
			for (int i = 0; i < PRE_ALLOC_NODE_NUM / SUB_QUEUE_NUM; ++i) {
				if (queue.isEmpty()) {// which means the linked-list is empty.
					queue.head.store(new ListNode(T()), std::memory_order_release);
					queue.tail.store(queue.head.load(std::memory_order_relaxed), std::memory_order_relaxed);
					//std::assert(queue.head.load(std::memory_order_relaxed) == queue.tail.load(std::memory_order_relaxed));
				}
				else {
					ListNode* next = queue.tail.load(std::memory_order_acquire)->next.load(std::memory_order_acquire);
					next = new ListNode(T());
					queue.tail.store(next, std::memory_order_release);
				}
			}
			queue.tail.store(queue.head.load(std::memory_order_relaxed));
			//std::assert(queue.head.load(std::memory_order_relaxed) == queue.tail.load(std::memory_order_relaxed));
		}
		/*
		// block when queue is empty
		std::unique_lock<std::mutex> lock(mtx_empty);
		cond_empty.wait(lock, [this] {
			return !this->empty();
		});
		*/
	}

	template<typename T>
	inline ConcurrentQueue_impl<T>::ConcurrentQueue_impl(const size_t & num_elements)
		:max_elements(num_elements) {
		_size.store(0, std::memory_order_relaxed);
		_dequeue_idx.store(0, std::memory_order_relaxed);
		// allocate num_elements nodes.
		for (auto& queue : sub_queues) {
			for (int i = 0; i < max_elements / SUB_QUEUE_NUM; ++i) {
				if (queue.isEmpty()) {// which means the linked-list is empty.
					queue.head.store(new ListNode(T()), std::memory_order_release);
					queue.tail.store(queue.head.load(std::memory_order_relaxed), std::memory_order_relaxed);
					//std::assert(queue.head.load(std::memory_order_relaxed) == queue.tail.load(std::memory_order_relaxed));
				}
				else {
					ListNode* next = queue.tail.load(std::memory_order_acquire)->next.load(std::memory_order_acquire);
					next = new ListNode(T());
					queue.tail.store(next, std::memory_order_release);
				}
			}
			queue.tail.store(queue.head.load(std::memory_order_relaxed));
			//std::assert(queue.head.load(std::memory_order_relaxed) == queue.tail.load(std::memory_order_relaxed));
		}
		/*
		// block when queue is empty
		std::unique_lock<std::mutex> lock(mtx_empty);
		cond_empty.wait(lock, [this] {
			return !this->empty();
		});
		*/
	}

} // namespace concurrentlib

#endif // !_CONCURRENT_QUEUE_IMPL_HPP
