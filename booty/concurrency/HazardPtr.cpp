/*
 * This is a derivative snippet of Facebook::folly, under Apache Lisence.
 * Indention:
 * - Recurrent the design idea of seniors and rewrite some details to adapt
 *   personal considerations as components of booty.
 *
 * @Simoncqk - 2018.05.02
 *
 */
#include<memory>

#include"HazardPtr.h"

namespace booty {

	namespace concurrency {




		constexpr hazptr_domain::hazptr_domain(std::pmr::memory_resource * mr) noexcept
			:mr_(mr) {}

		hazptr_domain::~hazptr_domain() {
		}

		void hazptr_domain::cleanup() {
		}

		void hazptr_domain::tryTimedCleanup() {
		}

		void hazptr_domain::objRetire(hazptr_obj *) {
		}

		hazptr_rec * hazptr_domain::hazptrAcquire() {
			return nullptr;
		}

		void hazptr_domain::hazptrRelease(hazptr_rec *) noexcept {
		}

		int hazptr_domain::pushRetired(hazptr_obj * head, hazptr_obj * tail, int count) {
			return 0;
		}

		bool hazptr_domain::reachedThreshold(int rcount) {
			return false;
		}

		void hazptr_domain::tryBulkReclaim() {
		}

		void hazptr_domain::bulkReclaim() {
		}

		inline void hazptr_rec::set(const void * p) noexcept {
			hazptr_.store(p, std::memory_order_release);
		}

		inline const void * hazptr_rec::get() const noexcept {
			return hazptr_.load(std::memory_order_acquire);
		}

		inline void hazptr_rec::clear() noexcept {
			hazptr_.store(nullptr, std::memory_order_release);
		}

		inline bool hazptr_rec::isActive() noexcept {
			return active_.load(std::memory_order_acquire);
		}

		inline bool hazptr_rec::tryAcquire() noexcept {
			bool active = isActive();
			if (!active&&
				active_.compare_exchange_strong(
					active, true, std::memory_order_release, std::memory_order_relaxed)) {
				return true;
			}
			return false;
		}

		inline void hazptr_rec::release() noexcept {
			active_.store(false, std::memory_order_release);
		}

	} // concurrency

} // booty