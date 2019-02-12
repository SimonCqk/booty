/*
 * SpinLock is a fine-grained lock for protecting teeny-tiny data.
 * It is a updated version of folly::MicroSpinLock, add some more contention control mechanism.
 * @Simoncqk 2018.11.05
 */
#ifndef BOOTY_CONCURRENCY_MICROSPINLOCK
#define BOOTY_CONCURRENCY_MICROSPINLOCK

#include<type_traits>
#include<cstdint>
#include<cassert>
#include<atomic>
#include<chrono>
#include<thread>
#include"../Asm.h"

namespace booty {

	namespace concurrency {

		/// SpinLock is a fine-grained lock for protecting teeny-tiny data.
		/// keep it a POD type. But do NOT abuse of it.
		struct MicroSpinLock {

			constexpr static int16_t kMaxActiveSpin = (1 << 12);
			// sleep at least 0.1ms when lock contention happens.
			constexpr static std::chrono::microseconds kInitSleepTime = 100 * std::chrono_literals::ns;
			// spin counter increases kSleepIncStep times, sleep time increases 0.1ms
			constexpr static int16_t kSleepIncStep = (1 << 10);

			enum { FREE = 0, LOCKED = 1 };
			// lock_ can't be std::atomic<> to preserve POD-ness.
			uint8_t lock_;
			int16_t spinCnt;

		public:

			// POD doesn't have ctor, so init it manually.
			void init() {
				payload()->store(FREE, std::memory_order_relaxed);
				spinCnt = 0;
			}

			bool try_lock() noexcept {
				CAS(FREE, LOCKED);
			}

			void lock() noexcept {
				// acquire lock failed
				while (!try_lock()) {
					// spin for lock
					while (payload()->load(std::memory_order_relaxed) == LOCKED) {
						spinAndWait();
					}
				}
				assert(payload()->load() == LOCKED);
			}

			void unlock() noexcept {
				assert(payload()->load() == LOCKED);
				payload()->store(FREE, std::memory_order_release);
			}

		private:
			std::atomic<uint8_t>* payload() noexcept {
				return reinterpret_cast<std::atomic<uint8_t>*>(&this->lock_);
			}

			bool CAS(uint8_t oldVal, uint8_t newVal)noexcept {
				return std::atomic_compare_exchange_strong_explicit(
					payload(),
					&oldVal,
					newVal,
					std::memory_order_acquire,
					std::memory_order_relaxed
				);
			}

			void spinAndWait() noexcept {
				++spinCnt;
				int steps = (spinCnt < kMaxActiveSpin) ? kMaxActiveSpin / spinCnt : kMaxActiveSpin / kSleepIncStep;
				if (steps == 0) {
					asm_volatile_pause();
				}
				else {
					std::this_thread::sleep_for(steps*kInitSleepTime);
				}
			}
		};

		static_assert(std::is_pod_v(MicroSpinLock), "MicroSpinLock must be a POD type");
	}

}

#endif // !BOOTY_CONCURRENCY_MICROSPINLOCK
