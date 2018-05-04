/*
* This is a derivative snippet of Facebook::folly, under Apache Lisence.
* Indention:
* - Recurrent the design idea of seniors and rewrite some details to adapt
*   personal considerations as components of booty.
*
* @Simoncqk - 2018.05.02
*/
#ifndef BOOTY_CONCURRENCY_SPIN_H
#define BOOTY_CONCURRENCY_SPIN_H

#include <algorithm>
#include <chrono>
#include <thread>

extern void _mm_pause(void);

namespace booty {

	inline void asm_volatile_pause() {
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
		::_mm_pause();
#elif defined(__i386__) || FOLLY_X64
		asm volatile("pause");
#elif FOLLY_AARCH64 || defined(__arm__)
		asm volatile("yield");
#elif FOLLY_PPC64
		asm volatile("or 27,27,27");
#endif
	}

	namespace concurrency {

		/// WaitOptions
		///
		/// Various synchronization primitives as well as various concurrent data
		/// structures built using them have operations which might wait. This type
		/// represents a set of options for controlling such waiting.
		class WaitOptions {
		public:
			struct Defaults {
				/// spin_max
				///
				/// If multiple threads are actively using a synchronization primitive,
				/// whether indirectly via a higher-level concurrent data structure or
				/// directly, where the synchronization primitive has an operation which
				/// waits and another operation which wakes the waiter, it is common for
				/// wait and wake events to happen almost at the same time. In this state,
				/// we lose big 50% of the time if the wait blocks immediately.
				///
				/// We can improve our chances of being waked immediately, before blocking,
				/// by spinning for a short duration, although we have to balance this
				/// against the extra cpu utilization, latency reduction, power consumption,
				/// and priority inversion effect if we end up blocking anyway.
				///
				/// We use a default maximum of 2 usec of spinning. As partial consolation,
				/// since spinning as implemented in folly uses the pause instruction where
				/// available, we give a small speed boost to the colocated hyperthread.

				static constexpr std::chrono::nanoseconds spin_max =
					std::chrono::microseconds(2);
			};

			std::chrono::nanoseconds spin_max() const {
				return spin_max_;
			}

			/* set new spin max duration and return WaitOptions instance. */
			WaitOptions& setSpinMax(std::chrono::nanoseconds dur) {
				spin_max_ = dur;
				return *this;
			}

		private:
			std::chrono::nanoseconds spin_max_ = Defaults::spin_max;
		};

		enum class spin_result {
			success, // condition passed
			timeout, // exceeded deadline
			advance, // exceeded current wait-options component timeout
		};

		template <typename Clock, typename Duration, typename CondFunc>
		spin_result spin_pause_until(
			std::chrono::time_point<Clock, Duration> const& deadline,
			WaitOptions const& opt,	CondFunc cond_func) {
			if (opt.spin_max() <= opt.spin_max().zero()) {
				return spin_result::advance;
			}

			auto tbegin = Clock::now();
			while (true) {
				if (cond_func()) {
					return spin_result::success;
				}

				auto const tnow = Clock::now();
				if (tnow >= deadline) {
					return spin_result::timeout;
				}

				//  Backward time discontinuity in Clock? revise pre_block starting point
				tbegin = std::min(tbegin, tnow);
				if (tnow >= tbegin + opt.spin_max()) {
					return spin_result::advance;
				}

				//  original folly uses asm instructions to achieve <Pause and Donate
				//  Full Capabilities of Current Core to Its Other Hyperthreads>, while
				//  it is too obscure. Now yiled() release current CPU timeslice and 
				//  reschedule tasks.

				//  PAUSE instruction is x86 specific. It's sole use is in spin-lock wait loops.
				//  PAUSE does not release the processor to allow another thread to run. 
				//  It is not a "low-level" std::this_thread::yield().
				asm_volatile_pause();
			}
		}

		template <typename Clock, typename Duration, typename CondFunc>
		spin_result spin_yield_until(
			std::chrono::time_point<Clock, Duration> const& deadline,
			CondFunc cond_func) {
			while (true) {
				if (cond_func()) {
					return spin_result::success;
				}

				auto const max = std::chrono::time_point<Clock, Duration>::max();
				if (deadline != max && Clock::now() >= deadline) {
					return spin_result::timeout;
				}

				std::this_thread::yield();
			}
		}

	}
}

#endif // !BOOTY_CONCURRENCY_SPIN_H
