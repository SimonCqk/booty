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
		static constexpr size_t PRE_ALLOC_NODE_NUM = 512;
		static constexpr size_t NEXT_ALLOC_NODE_NUM = 32;
		static constexpr size_t MAX_CONTEND_TRY_TIME = 64;

		// define free-list structure.
		struct ListNode
		{
			T data;
			aListNode_p next;
			abool is_hold;
			ListNode() {
				next.store(nullptr, std::memory_order_relaxed);
				is_hold.store(false, std::memory_order_relaxed);
			}

			template<typename Ty>
			ListNode(Ty&& val)
				:data(std::forward<Ty>(val)) {
				next.store(nullptr, std::memory_order_relaxed);
				is_hold.store(false, std::memory_order_relaxed);
			}
		};

		// define concurrent linked-list.
		struct ContLinkedList  
		{
			aListNode_p head;
			aListNode_p tail;
			ContLinkedList() {
				head.store(nullptr, std::memory_order_relaxed);
				tail.store(head.load(std::memory_order_relaxed), std::memory_order_relaxed);  // init: head = tail
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

		/*
		  get the ref of head element of this queue.
		*/
		T& front() {
			if (this->empty())
				throw std::exception("No element in queue.");
			auto& cur_queue = sub_queues[_getEnqueueIndex()];
			ListNode* _head = cur_queue.head.load(std::memory_order_relaxed);
			while (!_head || _head->is_hold.load(std::memory_order_acquire)) {
				std::this_thread::yield();
				_head = cur_queue.head.load(std::memory_order_acquire);
			}
			return _head->data;
		}

		/*
	      enqueue one element.
		*/
		void enqueue(const T& data) {
			if (max_elements > 0 && _size.load() >= max_elements) {
				std::unique_lock<std::mutex> lock(mtx_maxsize);
				cond_maxsize.wait(lock, [this] {
					return _size.load(std::memory_order_acquire) < max_elements;
				});
			}
			if (this->empty()) {
				while (!tryEnqueue(data));  // try until succeed.
				cond_empty.notify_one();
			}
			else
				while (!tryEnqueue(data));
		}

		/*
		  enqueue one element(rvalue).
		*/
		void enqueue(T&& data) {
			if (max_elements > 0 && _size.load() >= max_elements) {
				std::unique_lock<std::mutex> lock(mtx_maxsize);
				cond_maxsize.wait(lock, [this] {
					return _size.load(std::memory_order_acquire) < max_elements;
				});
			}
			if (this->empty()) {
				while (!tryEnqueue(std::forward<T>(data)));  // try until succeed.
				cond_empty.notify_one();
			}
			else
				while (!tryEnqueue(std::forward<T>(data)));
			
		}

		/*
		  dequeue one element,  block when it becomes empty after dequeue.
		*/
		void dequeue(T& data) {
			if (this->empty())
				throw std::exception("No elements in queue.");
			while (!tryDequeue(data));  // try until succeed.
			_blockEmptyOrNot();
		}

		/*
		  dequeue one element,  block when it becomes empty after dequeue.
		  NOT THAT SAFE than the former one, because it
		  doesn't provide strong exception-safety-guarantee.
		*/
		T dequeue() {
			if (this->empty())
				throw std::exception("No elements in queue.");
			T data;
			while (!tryDequeue(data));  // try until succeed.
			_blockEmptyOrNot();
			return std::move(data);
		}

		size_t size() const {
			return _size.load(std::memory_order_acquire);
		}
		bool empty() const {
			return _size.load(std::memory_order_acquire) == 0;
		}

		~ConcurrentQueue_impl() {
			for (auto& queue : sub_queues) {
				if (queue.isEmpty())
					continue;
				ListNode* head = queue.head.load(std::memory_order_relaxed);
				ListNode* next = head->next.load(std::memory_order_relaxed);
				while (next) {
					head = next;
					next = next->next.load(std::memory_order_relaxed);
					delete head; head = nullptr;
				}
				queue.tail.store(nullptr, std::memory_order_relaxed);
			}
		}

		// forbid copy ctor & copy operator.
		ConcurrentQueue_impl(const ConcurrentQueue_impl& queue) = delete;
		ConcurrentQueue_impl& operator=(const ConcurrentQueue_impl& queue) = delete;

	private:

		size_t _getEnqueueIndex() {
			return _size.load() % SUB_QUEUE_NUM;
		}

		size_t _getDequeueIndex() {
			return _dequeue_idx.load() % SUB_QUEUE_NUM;
		}

		void _blockEmptyOrNot() {
			if (this->empty()) {
				std::unique_lock<std::mutex> lock(mtx_empty);
				cond_empty.wait(lock, [this] {
					return !this->empty();
				});
			}
		}

		// return the tail of linked list.
		// if nodes are running out, then allocate some new nodes.
		ListNode* acquireOrAlloc(ContLinkedList& list) {
			if (list.isEmpty())
				return nullptr;
			ListNode* _tail = list.tail.load(std::memory_order_relaxed);
			size_t try_time = 0;
			while (!_tail || _tail->is_hold.load(std::memory_order_acquire)) {
				std::this_thread::yield();
				_tail = list.tail.load(std::memory_order_acquire);
				if (++try_time > MAX_CONTEND_TRY_TIME)
					return nullptr;
			}
			_tail->is_hold.store(true, std::memory_order_release);
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
			auto& cur_queue = sub_queues[_getEnqueueIndex()];
			ListNode* tail = acquireOrAlloc(cur_queue);
			if (!tail || !tail->is_hold.load(std::memory_order_acquire)||  // ensure it has been acquired successfully.
				tail!=cur_queue.tail.load(std::memory_order_acquire))  // ensure tail is the current tail of cur_queue.
				return false;
			tail->data = std::forward<Ty>(data);
			tail->is_hold.store(false, std::memory_order_release);
			// use CAS to update tail node.
			while (!cur_queue.tail.compare_exchange_weak(tail, tail->next.load(std::memory_order_acquire)));
			++_size;
			return true;
		}

		bool tryDequeue(T& data) {
			auto& cur_queue = sub_queues[_getDequeueIndex()];
			if (cur_queue.isEmpty())
				return false;
			ListNode* _head = cur_queue.head.load(std::memory_order_relaxed);
			size_t try_time = 0;
			while (!_head || _head->is_hold.load(std::memory_order_acquire)) {
				if (++try_time > MAX_CONTEND_TRY_TIME)
					return false;
				std::this_thread::yield();
				_head = cur_queue.head.load(std::memory_order_acquire);
			}
			if (!_head || _head->is_hold.load(std::memory_order_acquire))  // ensure it has been acquired successfully.
				return false;
			_head->is_hold.store(true, std::memory_order_release);
			data = _head->data;
			if (_head == cur_queue.tail.load(std::memory_order_acquire)) {  // this sub-queue will be empty after dequeue.
				delete _head;
				_head = nullptr;
				cur_queue.tail.store(nullptr, std::memory_order_release);
			}
			else {
				_head->is_hold.store(false, std::memory_order_release);				
				// use CAS to update head.
				while (!cur_queue.head.compare_exchange_weak(_head, _head->next.load(std::memory_order_acquire)));
				delete _head; _head = nullptr;
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

		// std::array perform better than std::vector.
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
				if (queue.isEmpty()) {
					queue.head.store(new ListNode(T()), std::memory_order_release);
					queue.tail.store(queue.head.load(std::memory_order_acquire), std::memory_order_release);
				}
				else {
					queue.tail.load(std::memory_order_acquire)->next.store(new ListNode(T()), std::memory_order_release);
					queue.tail.store(queue.tail.load(std::memory_order_acquire)->next.load(std::memory_order_acquire)
						, std::memory_order_release);
				}
			}
			queue.tail.store(queue.head.load(std::memory_order_relaxed),std::memory_order_relaxed);
		}
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
					queue.head.store(new ListNode(T()), std::memory_order_release);
					queue.tail.store(queue.head.load(std::memory_order_acquire), std::memory_order_release);
				}
				else {
					queue.tail.load(std::memory_order_acquire)->next.store(new ListNode(T()), std::memory_order_release);
					queue.tail.store(queue.tail.load(std::memory_order_acquire)->next.load(std::memory_order_acquire)
						, std::memory_order_release);
				}
			}
			queue.tail.store(queue.head.load(std::memory_order_relaxed), std::memory_order_relaxed);
		}
	}

} // namespace concurrentlib

#endif // !_CONCURRENT_QUEUE_IMPL_HPP
