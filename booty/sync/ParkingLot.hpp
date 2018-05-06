/*
 * This is a derivative snippet of Facebook::folly, under Apache Lisence.
 * Indention:
 * - Recurrent the design idea of seniors and rewrite some details to adapt
 *   personal considerations as components of booty.
 *
 * @Simoncqk - 2018.05.03
 *
 */
#ifndef BOOTY_SYNC_PARKINGLOT_HPP
#define BOOTY_SYNC_PARKINGLOT_HPP

#include<atomic>
#include<condition_variable>
#include<mutex>
#include<array>
#include<cassert>

#include"../Unit.h"

namespace booty {

	namespace parking_lot_detail {

		struct WaitNodeBase {
			const uint64_t key_;
			const uint64_t lot_id_;
			WaitNodeBase* next_{ nullptr };
			WaitNodeBase* prev_{ nullptr };

			// tricky: hold both bucket and node mutex to write, either to read
			bool signaled_;
			std::mutex mutex_;
			std::condition_variable cond_;

			WaitNodeBase(uint64_t key, uint64_t lotid)
				: key_(key), lot_id_(lotid), signaled_(false) {}

			template <typename Clock, typename Duration>
			std::cv_status wait(std::chrono::time_point<Clock, Duration> deadline) {
				std::cv_status status = std::cv_status::no_timeout;
				std::unique_lock<std::mutex> node_lock(mutex_);
				while (!signaled_ && status != std::cv_status::timeout) {
					if (deadline != std::chrono::time_point<Clock, Duration>::max()) {
						status = cond_.wait_until(node_lock, deadline);
					}
					else {
						cond_.wait(node_lock);
					}
				}
				return status;
			}

			void wake() {
				std::lock_guard<std::mutex> nodeLock(mutex_);
				signaled_ = true;
				cond_.notify_one();
			}

			bool isSignaled() const {
				return signaled_;
			}
		};

		extern std::atomic<uint64_t> id_allocator{ 0 };

		// Our emulated futex uses 4096 lists of wait nodes.  There are two levels
		// of locking: a per-list mutex that controls access to the list and a
		// per-node mutex, condition var, and bool that are used for the actual wakeups.
		// The per-node mutex allows us to do precise wakeups without thundering herds.
		struct Bucket {
			std::mutex mutex_;
			WaitNodeBase* head_;
			WaitNodeBase* tail_;
			std::atomic<uint64_t> count_;

			static constexpr size_t  kNumBuckets = 4096;

			static Bucket& bucketFor(uint64_t key) {
				// Statically allocating this lets us use this in allocation-sensitive
				// contexts. This relies on the assumption that std::mutex won't dynamically
				// allocate memory, which we assume to be the case on Linux and iOS.
				static std::array<Bucket, kNumBuckets> gBuckets;
				return gBuckets[key % kNumBuckets];
			}

			void push_back(WaitNodeBase* node) {
				if (tail_) {
					assert(head_ != nullptr);
					node->prev_ = tail_;
					tail_->next_ = node;
					tail_ = node;
				}
				else {  // bucket is empty.
					tail_ = head_ = node;
				}
				//  no count_.fetch_add() here. The corresponding op is done in 
				//  ParkingLot::park_until() down below.
			}

			void erase(WaitNodeBase* node) {
				assert(count_.load(std::memory_order_relaxed) >= 1);
				if (head_ == node && tail_ == node) {
					// node is the only one element in bucket
					assert(node->prev_ == nullptr);
					assert(node->next_ == nullptr);
					head_ = tail_ = nullptr;
				}
				else if (head_ == node) {
					assert(node->prev_ == nullptr);
					assert(node->next_ != nullptr);
					head_ = node->next_;
					head_->prev_ = nullptr;
				}
				else if (tail_ == node) {
					assert(node->next_ == nullptr);
					assert(node->prev_ != nullptr);
					tail_ = node->prev_;
					tail_->next_ = nullptr;
				}
				else {
					assert(node->next_ != nullptr);
					assert(node->prev_ != nullptr);
					node->next_->prev_ = node->prev_;
					node->prev_->next_ = node->next_;
				}
				count_.fetch_sub(1, std::memory_order_relaxed);
			}
		};

	} // namespace parking_lot_detail

	enum class UnparkControl {
		RetainContinue,
		RemoveContinue,
		RetainBreak,
		RemoveBreak,
	};

	enum class ParkResult {
		Skip,
		Unpark,
		Timeout,
	};

	/*
	* ParkingLot provides an interface that is similar to Linux's futex
	* system call, but with additional functionality.  It is implemented
	* in a portable way on top of std::mutex and std::condition_variable.
	*
	* Additional reading:
	* https://webkit.org/blog/6161/locking-in-webkit/
	* https://github.com/WebKit/webkit/blob/master/Source/WTF/wtf/ParkingLot.h
	* https://locklessinc.com/articles/futex_cheat_sheet/
	*
	* The main difference from futex is that park/unpark take lambdas,
	* such that nearly anything can be done while holding the bucket
	* lock.  Unpark() lambda can also be used to wake up any number of
	* waiters.
	*
	* ParkingLot is templated on the data type, however, all ParkingLot
	* implementations are backed by a single static array of buckets to
	* avoid large memory overhead.  Lambdas will only ever be called on
	* the specific ParkingLot's nodes.
	*/
	template <typename Data = Unit>
	class ParkingLot {
		const uint64_t lot_id_;
		ParkingLot(const ParkingLot&) = delete;

		struct WaitNode : public parking_lot_detail::WaitNodeBase {
			const Data data_;

			template <typename D>
			WaitNode(uint64_t key, uint64_t lotid, D&& data)
				: WaitNodeBase(key, lotid), data_(std::forward<Data>(data)) {}
		};

	public:
		ParkingLot()
			: lot_id_(parking_lot_detail::id_allocator++) {}

		/* Park API
		*
		* Key is almost always the address of a variable.
		*
		* ToPark runs while holding the bucket lock: usually this
		* is a check to see if we can sleep, by checking waiter bits.
		*
		* PreWait is usually used to implement condition variable like
		* things, such that you can unlock the condition variable's lock at
		* the appropriate time.
		*/
		template <typename Key, typename D, typename ToPark, typename PreWait>
		ParkResult park(const Key key, D&& data, ToPark&& toPark, PreWait&& preWait) {
			return park_until(
				key,
				std::forward<D>(data),
				std::forward<ToPark>(toPark),
				std::forward<PreWait>(preWait),
				std::chrono::steady_clock::time_point::max());
		}

		template <typename Key, typename D, typename ToPark, typename PreWait,
			typename Rep, typename Period>
			ParkResult park_for(const Key key, D&& data, ToPark&& toPark, PreWait&& preWait,
				std::chrono::duration<Rep, Period>& timeout) {
			return park_until(
				key,
				std::forward<D>(data),
				std::forward<ToPark>(toPark),
				std::forward<PreWait>(preWait),
				timeout + std::chrono::steady_clock::now());
		}

		template <typename Key, typename D, typename ToPark, typename PreWait,
			typename Clock, typename Duration>
			ParkResult park_until(const Key bits, D&& data, ToPark&& toPark, PreWait&& preWait,
				std::chrono::time_point<Clock, Duration> deadline) {
			auto key = std::hash<uint64_t>()(uint64_t(bits));
			auto& bucket = parking_lot_detail::Bucket::bucketFor(key);
			WaitNode node(key, lot_id_, std::forward<D>(data));

			{
				// A: Must be seq_cst.  Matches B.
				bucket.count_.fetch_add(1, std::memory_order_seq_cst);

				std::unique_lock<std::mutex> bucket_lock(bucket.mutex_);

				if (!std::forward<ToPark>(toPark)()) {
					bucket_lock.unlock();
					bucket.count_.fetch_sub(1, std::memory_order_relaxed);
					return ParkResult::Skip;
				}

				bucket.push_back(&node);
			} // bucket_lock scope

			std::forward<PreWait>(preWait)();

			auto status = node.wait(deadline);

			if (status == std::cv_status::timeout) {
				// it's not really a timeout until we unlink the unsignaled node
				std::lock_guard<std::mutex> bucketLock(bucket.mutex_);
				if (!node.isSignaled()) {
					bucket.erase(&node);
					return ParkResult::Timeout;
				}
			}

			return ParkResult::Unpark;
		}

		/*
		* Unpark API
		*
		* Key is the same unique address used in park(), and is used as a
		* hash key for lookup of waiters.
		*
		* Unparker is a function that is given the Data parameter, and
		* returns an UnparkControl.  The Remove* results will remove and
		* wake the waiter, the Ignore/Stop results will not, while stopping
		* or continuing iteration of the waiter list.
		*/
		template <typename Key, typename Unparker>
		void unpark(const Key bits, Unparker&& func) {
			auto key = std::hash<uint64_t>()(uint64_t(bits));
			auto& bucket = parking_lot_detail::Bucket::bucketFor(key);
			// B: Must be seq_cst.  Matches A.  If true, A *must* see in seq_cst
			// order any atomic updates in toPark() (and matching updates that
			// happen before unpark is called)
			if (bucket.count_.load(std::memory_order_seq_cst) == 0) {
				return;
			}

			std::lock_guard<std::mutex> bucketLock(bucket.mutex_);

			for (auto iter = bucket.head_; iter;) {
				auto node = static_cast<WaitNode*>(iter);
				iter = iter->next_;
				if (node->key_ == key && node->lot_id_ == lot_id_) {
					auto result = std::forward<Func>(func)(node->data_);
					if (result == UnparkControl::RemoveBreak ||
						result == UnparkControl::RemoveContinue) {
						// we unlink, but waiter destroys the node
						bucket.erase(node);

						node->wake();
					}
					if (result == UnparkControl::RemoveBreak ||
						result == UnparkControl::RetainBreak) {
						return;
					}
				}
			}
		}
	};
} // !booty

#endif // !BOOTY_SYNC_PARKINGLOT_HPP

