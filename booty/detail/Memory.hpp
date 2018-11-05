/*
 * Memory.h contains some mem-level operations, it's underlying and makes
 * those slight things transparent.
 * @Simoncqk 2018.11.05
 */
#ifndef BOOTY_DETAIL_MEMORY_HPP
#define BOOTY_DETAIL_MEMORY_HPP

#include<memory>

namespace booty {
	namespace detail {

		// util lifter
		template<typename T>
		struct lift_void_to_char {
			using type = T;
		};

		// specialization for lifer
		template<>
		struct lift_void_to_char<void> {
			using type = char;
		};

		/// SysAllocator acts like std::allocator, but wraps std::malloc and std::free.
		template<typename T>
		class SysAllocator {

			using This = SysAllocator<T>;

		public:

			T* allocate(size_t size) {
				using lifted = typename lift_void_to_char<T>::type;
				const auto p = std::malloc(sizeof(lifted)*size);
				// malloc failed
				if (!p) {
					throw std::bad_alloc("allocate failed in SysAllocator");
				}
				return static_cast<T*>(p);
			}

			void deallocate(T* p) {
				std::free(p);
			}

			// all SysAllocator instance acts like the same.
			bool operator==(const This& lhs, const This& rhs) noexcept {
				return true;
			}

			bool operator!=(const This& lhs, const This& rhs)noexcept {
				return false;
			}
		};
	}
}


#endif // !BOOTY_DETAIL_MEMORY_HPP
