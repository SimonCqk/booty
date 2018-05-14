/*
 * This is a derivative snippet of Facebook::folly, under Apache Lisence.
 * Indention:
 * - Recurrent the design idea of seniors and rewrite some details to adapt
 *   personal considerations as components of booty.
 *
 * @Simoncqk - 2018.05.02
 *
 */
#ifndef BOOTY_CONCURRENCY_HAZARDPTR_H
#define BOOTY_CONCURRENCY_HAZARDPTR_H

#include<memory_resource>
#include<atomic>
#include<cassert>

#include"../Portability.h"

namespace booty {

	namespace concurrency {

		/// Hazard Pointer: every reading-thread maintain a hazard pointer for
		/// single-writing & multi-reading. 
		/// Effect: By iterating every hazard pointer, we can judge whether the 
		/// pointed content is being visited by reading threads or not, so to
		/// determine is it safe to delete the memory hazard pointer points to.

		/** hazptr_rec: Private class that contains hazard pointers. */
		class alignas(std::hardware_destructive_interference_size) hazptr_rec {
			friend class hazptr_domain;

			std::atomic<const void*> hazptr_{ nullptr };
			hazptr_rec* next_{ nullptr };
			std::atomic<bool> active_{ false };

			void set(const void* p) noexcept;
			const void* get() const noexcept;
			void clear() noexcept;
			bool isActive() noexcept;
			bool tryAcquire() noexcept;
			void release() noexcept;
		};

		/** hazptr_domain: Class of hazard pointer domains. Each domain manages a set
		*  of hazard pointers and a set of retired objects. */
		class hazptr_obj;
		class hazptr_domain {
			std::pmr::memory_resource* mr_;
			std::atomic<hazptr_rec*> hazptrs_ = { nullptr };
			std::atomic<hazptr_obj*> retired_ = { nullptr };
			/* Using signed int for rcount_ because it may transiently be
			* negative.  Using signed int for all integer variables that may be
			* involved in calculations related to the value of rcount_. */
			std::atomic<int> hcount_ = { 0 };
			std::atomic<int> rcount_ = { 0 };

			static constexpr uint64_t syncTimePeriod_{ 2000000000 }; // in ns
			std::atomic<uint64_t> syncTime_{ 0 };

		public:
			constexpr explicit hazptr_domain(
				std::pmr::memory_resource* = std::pmr::get_default_resource()) noexcept;
			~hazptr_domain();

			hazptr_domain(const hazptr_domain&) = delete;
			hazptr_domain(hazptr_domain&&) = delete;
			hazptr_domain& operator=(const hazptr_domain&) = delete;
			hazptr_domain& operator=(hazptr_domain&&) = delete;

			/** Free-function retire.  May allocate memory */
			template <typename T, typename D = std::default_delete<T>>
			void retire(T* obj, D reclaim = {});
			void cleanup();
			void tryTimedCleanup();

		private:
			friend class hazptr_holder;
			template <typename, typename>
			friend class hazptr_obj_base;
			template <typename, typename>
			friend class hazptr_obj_base_refcounted;
			friend class hazptr_priv;

			void objRetire(hazptr_obj*);
			hazptr_rec* hazptrAcquire();
			void hazptrRelease(hazptr_rec*) noexcept;
			int pushRetired(hazptr_obj* head, hazptr_obj* tail, int count);
			bool reachedThreshold(int rcount);
			void tryBulkReclaim();
			void bulkReclaim();
		};

		/* hazptr_obj: Private class for objects protected by hazard pointers. */
		class hazptr_obj {
			template<typename, typename>
			friend class hazptr_obj_base;
			template<typename, typename>
			friend class hazptr_obj_base_refcounted;

			void(*reclaim_)(hazptr_obj*);  // func-ptr
			hazptr_obj* next_;
		public:
			// All constructors set next_ to this in order to catch misuse bugs like
			// double retire.
			hazptr_obj()noexcept
				:next_(this) {}
			hazptr_obj(const hazptr_obj&)noexcept
				:next_(this) {}
			hazptr_obj(hazptr_obj&&)noexcept
				:next_(this) {}

			hazptr_obj& operator=(const hazptr_obj&) {
				return *this;
			}

			hazptr_obj& operator=(hazptr_obj&&) {
				return *this;
			}

		private:
			void set_next(hazptr_obj* next) {
				next_ = next;
			}

			void retireCheckFail() {
				assert(next_, this);
			}

			void retireCheck() {
				// Only for catching misusage bugs like double retire
				if (next_ != this) {
					retireCheckFail();
				}
			}

			const void* getObjPtr() const {
				return this;
			}
		};

		/* Defination of hazptr_obj_base */
		template<typename T, typename D = std::default_delete<T>>
		class hazptr_obj_base :public hazptr_obj {
		public:
			/* Retire a removed object and pass the responsibility for
			   reclaiming it to the hazptr library
			 */
			void retire();
		private:
			D deleter_;
		};

		/* Defination of hazptr_obj_base_refcounted */
		template<typename T, typename D = std::default_delete<T>>
		class hazptr_obj_base_refcounted :public hazptr_obj {
		public:
			/* Retire a removed object and pass the responsibility for
			reclaiming it to the hazptr library
			*/
			//void retire();

			/* aquire_ref() increments the reference count
			*
			* acquire_ref_safe() is the same as acquire_ref() except that in
			* addition the caller guarantees that the call is made in a
			* thread-safe context, e.g., the object is not yet shared. This is
			* just an optimization to save an atomic operation.
			*
			* release_ref() decrements the reference count and returns true if
			* the object is safe to reclaim.
			*/
			void acquire_ref();
			void acquire_ref_safe();
			bool release_ref();
		private:
			void preRetire(D deleter);

			std::atomic<uint32_t> refcount_{ 0 };
			D deleter_;
		};


		template<typename T, typename D>
		inline void hazptr_domain::retire(T * obj, D reclaim){

		}

		template<typename T, typename D>
		inline void hazptr_obj_base_refcounted<T, D>::acquire_ref() {
			refcount_.fetch_add(1);
		}

		template<typename T, typename D>
		inline void hazptr_obj_base_refcounted<T, D>::acquire_ref_safe() {
			auto old_val = refcount_.load(std::memory_order_acquire);
			assert(old_val >= 0);
			refcount_.store(old_val + 1, std::memory_order_release);
		}

		template<typename T, typename D>
		inline bool hazptr_obj_base_refcounted<T, D>::release_ref() {
			auto old_val = refcount_.load(std::memory_order_acquire);
			if (old_val > 0) {
				old_val = refcount_.fetch_sub(1);
			}
			else {
				if (kIsDebug) {
					refcount_.store(-1);
				}
			}
			assert(old_val >= 0);
			return old_val == 0;
		}

		template<typename T, typename D>
		inline void hazptr_obj_base_refcounted<T, D>::preRetire(D deleter) {
			deleter_ = std::move(deleter);
			retireCheck();
			reclaim_ = [](hazptr_obj* p) {
				auto hrobp = static_cast<hazptr_obj_base_refcounted*>(p);
				if (hrobp->release_ref()) {
					auto obj = static_cast<T*>(hrobp);
					hrobp->deleter_(obj);
				}
			};
		}

} // namespace concurrency

} // namespace booty

#endif // !BOOTY_CONCURRENCY_HAZARDPTR_H
