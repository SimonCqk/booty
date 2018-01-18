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

		static constexpr size_t kSubQueueNum = 8;
		static constexpr size_t kPreAllocNodeNum = 1024;
		static constexpr size_t kNextAllocNodeNum = 32;
		static constexpr size_t kMaxContendTryTime = 32;

		// define free-list structure.
		struct ListNode
		{
			T data;
			aListNode_p next;
			abool hold;
			ListNode() {
				next.store(nullptr, std::memory_order_relaxed);
				hold.store(false, std::memory_order_relaxed);
			}

			template<typename Ty>
			ListNode(Ty&& val)
				:data(std::forward<Ty>(val)) {
				next.store(nullptr, std::memory_order_relaxed);
				hold.store(false, std::memory_order_relaxed);
			}
		};

		// define concurrent linked-list.
		struct ContLinkedList
		{
			aListNode_p head;
			aListNode_p tail;
			ContLinkedList() {
				head.store(nullptr, std::memory_order_relaxed);
				tail.store(head.load(std::memory_order_relaxed), std::memory_order_relaxed);  // init: tail = head
			}
			bool isEmpty() {
				return head.load() == tail.load();
			}
		};

	public:
		/*
		  it is a unbounded queue.
		*/
		ConcurrentQueue_impl();

		/*
		  enqueue one element.
		*/
		void enqueue(const T& data) {
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
			if (this->empty()) {
				while (!tryEnqueue(std::forward<T>(data)));  // try until succeed.
				cond_empty.notify_one();
			}
			else
				while (!tryEnqueue(std::forward<T>(data)));

		}

		/*
		  dequeue one element,  block when it is empty.
		*/
		void dequeue(T& data) {
			if (this->empty()) {
				std::unique_lock<std::mutex> lock(mtx_empty);
				cond_empty.wait(lock, [this] {
					return !this->empty();
				});
			}
			while (!tryDequeue(data));  // try until succeed.
		}

		size_t size() const {
			return _size.load();
		}
		bool empty() const {
			return _size.load() == 0;
		}

		~ConcurrentQueue_impl() {
			for (auto& queue : sub_queues) {
				ListNode* head = queue.head.load(std::memory_order_relaxed);
				ListNode* next = head->next.load(std::memory_order_relaxed);
				delete head; head = nullptr;
				while (next) {
					head = next;
					next = next->next.load(std::memory_order_relaxed);
					delete head; head = nullptr;
				}
			}
		}

		// forbid copy ctor & copy operator.
		ConcurrentQueue_impl(const ConcurrentQueue_impl& queue) = delete;
		ConcurrentQueue_impl& operator=(const ConcurrentQueue_impl& queue) = delete;

	private:

		size_t _getEnqueueIndex() {
			return _enqueue_idx.load() % kSubQueueNum;
		}

		size_t _getDequeueIndex() {
			return _dequeue_idx.load() % kSubQueueNum;
		}

		// return the tail of linked list.
		// if nodes are running out, allocate some new nodes.
		ListNode* acquireOrAllocTail(ContLinkedList& list) {
			if (list.isEmpty()) {
				std::this_thread::yield();
				return nullptr;
			}
			ListNode* _tail = list.tail.load(std::memory_order_acquire);
			// queue-capacity is running out.
			if (!_tail || !_tail->next.load()) {
				std::atomic_thread_fence(std::memory_order_acq_rel);
				// preallocate a linked list.
				ListNode* new_head = new ListNode(T());
				auto head_copy = new_head;
				for (int i = 0; i < kNextAllocNodeNum; ++i) {
					head_copy->next.store(new ListNode(T()), std::memory_order_relaxed);
					head_copy = head_copy->next.load(std::memory_order_relaxed);
				}
				// connect new linked list to subqueue.
				list.tail.store(new_head);
				return nullptr;
			}
			for (size_t try_time = 0; !_tail || _tail->hold.load(std::memory_order_acquire); ++try_time) {
				std::this_thread::yield();
				_tail = list.tail.load(std::memory_order_acquire);
				if (try_time >= kMaxContendTryTime) 
					return nullptr;
			}
			assert(_tail != nullptr);
			_tail->hold.store(true, std::memory_order_release);
			return _tail;
		}

		/*
		  try to enqueue, return true if succeed.
		*/
		template<typename Ty>
		bool tryEnqueue(Ty&& data) {
			auto& cur_queue = sub_queues[_getEnqueueIndex()];
			ListNode* tail = acquireOrAllocTail(cur_queue);  // now tail->hold should be true.
			if (!tail || !tail->hold.load(std::memory_order_acquire))
				return false;
			++_enqueue_idx;
			tail->data = std::forward<Ty>(data);
			tail->hold.store(false, std::memory_order_release);
			// use CAS to update tail node.
			auto _next = tail->next.load(std::memory_order_seq_cst);
			while (!cur_queue.tail.compare_exchange_weak(tail, _next));
			++_size;
			return true;
		}

		/*
		  try to get the head element.
		*/
		ListNode* tryGetFront(ContLinkedList& list) {
			if (list.isEmpty()) {
				std::this_thread::yield();
				return nullptr;
			}
			ListNode* _next = list.head.load()->next.load(std::memory_order_acquire);
			for (size_t try_time = 0; !_next || _next->hold.load(std::memory_order_acquire); ++try_time) {
				std::this_thread::yield();
				_next = list.head.load()->next.load(std::memory_order_acquire); // TODO: dead-loop when _next is nullptr.
				if (try_time >= kMaxContendTryTime)
					return nullptr;
			}
			assert(_next != nullptr);
			_next->hold.store(true, std::memory_order_release);
			return _next;
		}

		/*
		  try to dequeue one element (pass to data) and destory it from heap.
		  REMARK: return by value is not provided since it's not exception safe.
		*/
		bool tryDequeue(T& data) {
			auto& cur_queue = sub_queues[_getDequeueIndex()];
			ListNode* _next = tryGetFront(cur_queue); 
			if (!_next||!_next->hold.load(std::memory_order_acquire))
				return false;
			++_dequeue_idx;
			data = std::move(_next->data); 
			//update head and return old head.
			auto old_head = cur_queue.head.exchange(_next, std::memory_order_acq_rel);
			_next->hold.store(false, std::memory_order_release);
			delete old_head; old_head = nullptr;
			--_size;
			return true;
		}

		// only for blocking when queue is empty.
		std::condition_variable cond_empty;
		std::mutex mtx_empty;

		// std::array perform better than std::vector.
		std::array<ContLinkedList, kSubQueueNum> sub_queues;
		asize_t _size;
		asize_t _enqueue_idx;
		asize_t _dequeue_idx;
	};

	template<typename T>
	inline ConcurrentQueue_impl<T>::ConcurrentQueue_impl() {
		_size.store(0, std::memory_order_relaxed);
		_enqueue_idx.store(0, std::memory_order_relaxed);
		_dequeue_idx.store(0, std::memory_order_relaxed);
		// pre-allocate kPreAllocNodeNum nodes.
		for (auto& queue : sub_queues) {
			queue.head.store(new ListNode(T()), std::memory_order_relaxed);
			queue.tail.store(new ListNode(T()), std::memory_order_relaxed);
			auto _tail = queue.tail.load(std::memory_order_relaxed);
			for (int i = 0; i < kPreAllocNodeNum / kSubQueueNum; ++i) {
				_tail->next.store(new ListNode(T()), std::memory_order_relaxed);
				_tail = _tail->next.load(std::memory_order_relaxed);
			}
			queue.head.load(std::memory_order_relaxed)->next.store(queue.tail.load(std::memory_order_relaxed), std::memory_order_relaxed);
		}
	}

} // namespace concurrentlib

#endif // !_CONCURRENT_QUEUE_IMPL_HPP