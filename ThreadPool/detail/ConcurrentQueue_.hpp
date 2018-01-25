#ifndef _CONCURRENT_LIST_QUEUE_HPP
#define _CONCURRENT_LIST_QUEUE_HPP

#include<iostream>
#include<array>
#include<chrono>
#include<thread>
#include<atomic>
#include<condition_variable>
#include<mutex>
#include<cassert>
static std::mutex m;

namespace concurrentlib {

	template<typename T>
	class ConcurrentQueue_ {
		struct ListNode;
		struct ContLinkedList;

		//define some type traits and constants.
		using index_t = std::size_t;
		using asize_t = std::atomic_size_t;
		using abool = std::atomic_bool;
		using aListNode_p = std::atomic<ListNode*>;

		static constexpr size_t kPreAllocNodeNum = 1024;
		static constexpr size_t kNextAllocNodeNum = 64;
		static constexpr size_t kMaxContendTryTime = 16;

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
			abool lock;
			ContLinkedList() {
				head.store(nullptr, std::memory_order_relaxed);
				tail.store(head.load(std::memory_order_relaxed), std::memory_order_relaxed);  // init: tail = head
				lock.store(false, std::memory_order_relaxed);
			}
			bool isEmpty() {
				return head.load() == tail.load();
			}

			void allocNewNodes() {
				this->lock.store(true);
				ListNode* _tail = this->tail.load();
				// preallocate a linked list.
				if (!_tail)
					_tail = new ListNode(T());
				ListNode** head_copy = &_tail;
				for (int i = 0; i < kNextAllocNodeNum; ++i) {
					(*head_copy)->next.store(new ListNode(T()), std::memory_order_relaxed);
					auto _next = (*head_copy)->next.load(std::memory_order_relaxed);
					head_copy = &_next;
				}
				// connect new linked list to subqueue.
				this->tail.store(_tail);
				this->lock.store(false);
			}
		};

	public:
		/*
		it is a unbounded queue.
		*/
		ConcurrentQueue_();

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
			/*{
				std::lock_guard<std::mutex> lock(m);

				auto head = queue.head.load();
				int cnt = 0;
				while (head) {
					++cnt;
					head = head->next.load();
				}
				std::printf("%d\n", cnt);

			}*/
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

		~ConcurrentQueue_() {
			ListNode* head = queue.head.load(std::memory_order_relaxed);
			ListNode* next = head->next.load(std::memory_order_relaxed);
			delete head; head = nullptr;
			while (next) {
				head = next;
				next = next->next.load(std::memory_order_relaxed);
				delete head; head = nullptr;
			}
		}

		// forbid copy ctor & copy operator.
		ConcurrentQueue_(const ConcurrentQueue_& queue) = delete;
		ConcurrentQueue_& operator=(const ConcurrentQueue_& queue) = delete;

	private:

		// return the tail of linked list.
		// if nodes are running out, allocate some new nodes.
		ListNode* acquireOrAllocTail() {
			if (queue.isEmpty() || queue.lock.load()) {
				std::this_thread::yield();
				return nullptr;
			}
			ListNode* _tail = queue.tail.load(std::memory_order_acquire);
			// queue-capacity is running out.
			if (!_tail || !_tail->next.load()) {
				std::atomic_thread_fence(std::memory_order_acq_rel);
				if (queue.lock.load())
					return nullptr;
				queue.allocNewNodes();
				return nullptr;
			}
			for (size_t try_time = 0; !_tail || _tail->hold.load(std::memory_order_acquire); ++try_time) {
				std::this_thread::yield();
				_tail = queue.tail.load(std::memory_order_acquire);
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
			ListNode* tail = acquireOrAllocTail();  // now tail->hold should be true.
			if (!tail || !tail->hold.load(std::memory_order_acquire))
				return false;
			tail->data = std::forward<Ty>(data);
			tail->hold.store(false, std::memory_order_release);
			// use CAS to update tail node.
			auto _next = tail->next.load(std::memory_order_seq_cst);
			while (!queue.tail.compare_exchange_weak(tail, _next));
			++_size;
			return true;
		}

		/*
		try to get the head element.
		*/
		ListNode* tryGetFront() {
			if (queue.isEmpty()) {
				std::this_thread::yield();
				return nullptr;
			}
			ListNode* _next = queue.head.load()->next.load(std::memory_order_acquire);
			for (size_t try_time = 0; !_next || _next->hold.load(std::memory_order_acquire); ++try_time) {
				std::this_thread::yield();
				_next = queue.head.load()->next.load(std::memory_order_acquire); std::cout << "====" << std::endl; // TODO: dead-loop when _next is nullptr.
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
			ListNode* _next = tryGetFront();
			if (!_next || !_next->hold.load(std::memory_order_acquire)) {
				return false;
			}
			data = std::move(_next->data);
			//update head and return old head.
			auto old_head = queue.head.exchange(_next, std::memory_order_seq_cst);
			//delete old_head; old_head = nullptr;
			_next->hold.store(false, std::memory_order_release);
			--_size;
			return true;
		}

		// only for blocking when queue is empty.
		std::condition_variable cond_empty;
		std::mutex mtx_empty;

		ContLinkedList queue;
		asize_t _size;
	};

	template<typename T>
	inline ConcurrentQueue_<T>::ConcurrentQueue_() {
		_size.store(0, std::memory_order_relaxed);

		// pre-allocate kPreAllocNodeNum nodes.
		queue.head.store(new ListNode(T()), std::memory_order_relaxed);
		std::cout << (queue.tail.load() == nullptr) << std::endl;
		auto _tail = queue.tail.load(std::memory_order_relaxed);
		for (int i = 0; i < kPreAllocNodeNum; ++i) {
			_tail->next.store(new ListNode(T()), std::memory_order_relaxed);
			_tail = _tail->next.load(std::memory_order_relaxed);
		}
		queue.head.load(std::memory_order_relaxed)->next.store(queue.tail.load(std::memory_order_relaxed), std::memory_order_relaxed);
		
		auto head = queue.head.load();
		int cnt = 0;
		while (head) {
			++cnt;
			head = head->next.load();
		}
		std::printf("%d\n", cnt);

	}

} // namespace concurrentlib

#endif // !_CONCURRENT_LIST_QUEUE_HPP