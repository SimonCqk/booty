#pragma once
#ifndef _CONCURRENT_QUEUE_IMPL_HPP
#define _CONCURRENT_QUEUE_IMPL_HPP

#include<array>
#include<chrono>
#include<thread>
#include<atomic>
#include<condition_variable>

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

		// define free-list structure.
		struct ListNode
		{
			T data;
			std::atomic<ListNode*> next;
			ListNode() {
				next.store(nullptr, std::memory_order_relaxed);
			}

			template<typename Ty>
			ListNode(Ty&& val)
				:data(std::forward<Ty>(val)) {
				next.store(nullptr, std::memory_order_relaxed);
			}
		};

		struct ContLinkedList
		{
			aListNode_p head;
			aListNode_p tail;
			ContLinkedList() {
				head.store(nullptr);
				tail.store(head.load(std::memory_order_relaxed), std::memory_order_relaxed);  // init: head = tail
			}
		};

	public:
		ConcurrentQueue_impl();
		ConcurrentQueue_impl(const size_t& num_elements = 1000);



		size_t size() const;
		bool empty() const;

		~ConcurrentQueue_impl();

		// forbid copy ctor & copy operator.
		ConcurrentQueue_impl(const ConcurrentQueue_impl& queue) = delete;
		ConcurrentQueue_impl& operator=(const ConcurrentQueue_impl& queue) = delete;

	private:
		size_t _choose();


		std::condition_variable cond_empty;
		std::array<ContLinkedList, SUB_QUEUE_NUM> sub_queues;
		asize_t _size;
	};

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
		using namespace std::chrono;
		return (_size*(system_clock::now().time_since_epoch().count() >> 31)) % SUB_QUEUE_NUM;
	}

}

#endif // !_CONCURRENT_QUEUE_IMPL_HPP
