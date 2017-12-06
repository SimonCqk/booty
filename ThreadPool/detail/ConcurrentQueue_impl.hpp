#pragma once
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
		constexpr size_t PRE_ALLOC_NODE_NUM = 1000;

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

		size_t size() const;
		bool empty() const;

		~ConcurrentQueue_impl();

		// forbid copy ctor & copy operator.
		ConcurrentQueue_impl(const ConcurrentQueue_impl& queue) = delete;
		ConcurrentQueue_impl& operator=(const ConcurrentQueue_impl& queue) = delete;

	private:
		size_t _choose();
		size_t _getDequeueIndex();

		ListNode* acquireOrAlloc();

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
		for (auto&& queue : sub_queues) {
			for (int i = 0; i < PRE_ALLOC_NODE_NUM / SUB_QUEUE_NUM; ++i) {
				if (!queue.head.load() && !queue.tail.load()) {// which means the linked-list is empty.
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
	}

	template<typename T>
	inline ConcurrentQueue_impl<T>::ConcurrentQueue_impl(const size_t & num_elements)
		:max_elements(num_elements){
		_size.store(0, std::memory_order_relaxed);
		_dequeue_idx.store(-1, std::memory_order_relaxed);

		// pre-allocate PRE_ALLOC_NODE_NUM nodes.
		for (auto&& queue : sub_queues) {
			for (int i = 0; i < max_elements / SUB_QUEUE_NUM; ++i) {
				if (!queue.head.load() && !queue.tail.load()) {// which means the linked-list is empty.
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
	}

	template<typename T>
	inline size_t ConcurrentQueue_impl<T>::size() const
	{
		return _size.load();
	}

	template<typename T>
	inline bool ConcurrentQueue_impl<T>::empty() const
	{
		return _size.load() == 0;
	}

	template<typename T>
	inline size_t ConcurrentQueue_impl<T>::_choose()
	{
		return _size.load() % SUB_QUEUE_NUM;
	}

	template<typename T>
	inline size_t ConcurrentQueue_impl<T>::_getDequeueIndex()
	{
		return ((_size.load() % SUB_QUEUE_NUM) + SUB_QUEUE_NUM - 1) % SUB_QUEUE_NUM;
	}


}

#endif // !_CONCURRENT_QUEUE_IMPL_HPP
