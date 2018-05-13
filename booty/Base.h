/*
 * This file includes some base classes with specific features.
 * They are used to inherited and expand.
 * @Simoncqk - 2018.05.10
 */

#ifndef BOOTY_BASE_H
#define BOOTY_BASE_H

#include<cassert>
#include<atomic>

#include"Portability.h"

namespace booty {

	/* Defination of NonCopyable class */
	class NonCopyable {
		// forbid copy ctor and copy assignment operator.
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;
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
		//void retire();
	private:
		D deleter_;
	};

	/* Defination of hazptr_obj_base_refcounted */
	template<typename T,typename D=std::default_delete<T>>
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
		void acquire_ref() {
			refcount_.fetch_add(1);
		}
		void acquire_ref_safe() {
			auto old_val = refcount_.load(std::memory_order_acquire);
			assert(old_val >= 0);
			refcount_.store(old_val + 1, std::memory_order_release);
		}
		bool release_ref() {
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
	private:
		void preRetire(D deleter) {
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

		std::atomic<uint32_t> refcount_{ 0 };
		D deleter_;
	};

} // booty

#endif // !BOOTY_BASE_H
