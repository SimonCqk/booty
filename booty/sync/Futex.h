/*
 * This is a derivative snippet of Facebook::folly, under Apache Lisence.
 * Indention:
 * - Recurrent the design idea of seniors and rewrite some details to adapt
 *   personal considerations as components of booty.
 *
 * @Simoncqk - 2018.05.03
 *
 */
#ifndef BOOTY_SYNC_FUTEX_H
#define BOOTY_SYNC_FUTEX_H

#include<atomic>
#include<chrono>

namespace booty {

	namespace sync {

		/// Futex(Fast Userspace muTEXes), as a fast sync(mutex) mechanism has 
		/// existed for a long time. Futex doesn't live only in kernel space, 
		/// but also user space, so a batch of unneccessary system calls can
		/// be avoided, which really boosts performance.
		/// <folly does a high-level encapsulation>
		/**
		* Futex is an atomic 32 bit unsigned integer that provides access to the
		* futex() syscall on that value.
		*
		* If you don't know how to use futex(), you probably shouldn't be using
		* this class.  Even if you do know how, you should have a good reason
		* (and benchmarks to back you up).
		*/
		enum class FutexResult {
			VALUE_CHANGED, /* futex value didn't match expected */
			AWOKEN,        /* wakeup by matching futex wake, or spurious wakeup */
			INTERRUPTED,   /* wakeup by interrupting signal */
			TIMEDOUT,      /* wakeup by expiring deadline */
		};

		template<template<typename> class Atom = std::atomic>
		struct Futex :Atom<uint32_t> {
			Futex()
				:Atom<uint32_t>{}
				explicit constexpr Futex(uint32_t init)
				: Atom<uint32_t>(init) {}
			/** Puts the thread to sleep if this->load() == expected.  Returns true when
			*  it is returning because it has consumed a wake() event, false for any
			*  other return (signal, this->load() != expected, or spurious wakeup). */
			FutexResult futexWait(uint32_t expected, uint32_t waitMask = -1) {
				auto rv = futexWaitImpl(expected, nullptr, nullptr, waitMask);
				assert(rv != FutexResult::TIMEDOUT);
				return rv;
			}

			/** Similar to futexWait but also accepts a deadline until when the wait call
			*  may block.
			*
			*  Optimal clock types: std::chrono::system_clock, std::chrono::steady_clock.
			*  NOTE: On some systems steady_clock is just an alias for system_clock,
			*  and is not actually steady.
			*
			*  For any other clock type, now() will be invoked twice. */
			template <class Clock, class Duration = typename Clock::duration>
			FutexResult futexWaitUntil(
				uint32_t expected,
				std::chrono::time_point<Clock, Duration> const& deadline,
				uint32_t waitMask = -1) {
				using Target = typename std::conditional<
					Clock::is_steady,
					std::chrono::steady_clock,
					std::chrono::system_clock>::type;
				auto const converted = time_point_conv<Target>(deadline);
				return converted == Target::time_point::max()
					? futexWaitImpl(expected, nullptr, nullptr, waitMask)
					: futexWaitImpl(expected, converted, waitMask);
			}

			/** Wakens up to count waiters where (waitMask & wakeMask) !=
			*  0, returning the number of awoken threads, or -1 if an error
			*  occurred.  Note that when constructing a concurrency primitive
			*  that can guard its own destruction, it is likely that you will
			*  want to ignore EINVAL here (as well as making sure that you
			*  never touch the object after performing the memory store that
			*  is the linearization point for unlock or control handoff).
			*  See https://sourceware.org/bugzilla/show_bug.cgi?id=13690 */
			int futexWake(int count = std::numeric_limits<int>::max(),
				uint32_t wakeMask = -1);

		private:
			/** Optimal when TargetClock is the same type as Clock.
			*
			*  Otherwise, both Clock::now() and TargetClock::now() must be invoked. */
			template <typename TargetClock, typename Clock, typename Duration>
			static typename TargetClock::time_point time_point_conv(
				std::chrono::time_point<Clock, Duration> const& time) {
				using std::chrono::duration_cast;
				using TimePoint = std::chrono::time_point<Clock, Duration>;
				using TargetDuration = typename TargetClock::duration;
				using TargetTimePoint = typename TargetClock::time_point;
				if (time == TimePoint::max()) {
					return TargetTimePoint::max();
				}
				else if (std::is_same<Clock, TargetClock>::value) {
					// in place of time_point_cast, which cannot compile without if-constexpr
					auto const delta = time.time_since_epoch();
					return TargetTimePoint(duration_cast<TargetDuration>(delta));
				}
				else {
					// different clocks with different epochs, so non-optimal case
					auto const delta = time - Clock::now();
					return TargetClock::now() + duration_cast<TargetDuration>(delta);
				}
			}

			template <typename Deadline>
			typename std::enable_if<Deadline::clock::is_steady, FutexResult>::type
				futexWaitImpl(
					uint32_t expected,
					Deadline const& deadline,
					uint32_t waitMask) {
				return futexWaitImpl(expected, nullptr, &deadline, waitMask);
			}

			template <typename Deadline>
			typename std::enable_if<!Deadline::clock::is_steady, FutexResult>::type
				futexWaitImpl(
					uint32_t expected,
					Deadline const& deadline,
					uint32_t waitMask) {
				return futexWaitImpl(expected, &deadline, nullptr, waitMask);
			}

			/** Underlying implementation of futexWait and futexWaitUntil.
			*  At most one of absSystemTime and absSteadyTime should be non-null.
			*  Timeouts are separated into separate parameters to allow the
			*  implementations to be elsewhere without templating on the clock
			*  type, which is otherwise complicated by the fact that steady_clock
			*  is the same as system_clock on some platforms. */
			FutexResult futexWaitImpl(
				uint32_t expected,
				std::chrono::system_clock::time_point const* absSystemTime,
				std::chrono::steady_clock::time_point const* absSteadyTime,
				uint32_t waitMask);
		};

		/** A std::atomic subclass that can be used to force Futex to emulate
		*  the underlying futex() syscall.  This is primarily useful to test or
		*  benchmark the emulated implementation on systems that don't need it. */
		template <typename T>
		struct EmulatedFutexAtomic : public std::atomic<T> {
			EmulatedFutexAtomic() noexcept = default;
			constexpr /* implicit */ EmulatedFutexAtomic(T init) noexcept
				: std::atomic<T>(init) {}
			// It doesn't copy or move
			EmulatedFutexAtomic(EmulatedFutexAtomic&& rhs) = delete;
		};

		/* Available specializations, with definitions elsewhere */

		template <>
		int Futex<std::atomic>::futexWake(int count, uint32_t wakeMask);

		template <>
		FutexResult Futex<std::atomic>::futexWaitImpl(
			uint32_t expected,
			std::chrono::system_clock::time_point const* absSystemTime,
			std::chrono::steady_clock::time_point const* absSteadyTime,
			uint32_t waitMask);

		template <>
		int Futex<EmulatedFutexAtomic>::futexWake(int count, uint32_t wakeMask);

		template <>
		FutexResult Futex<EmulatedFutexAtomic>::futexWaitImpl(
			uint32_t expected,
			std::chrono::system_clock::time_point const* absSystemTime,
			std::chrono::steady_clock::time_point const* absSteadyTime,
			uint32_t waitMask);
	};
}
#endif // !BOOTY_SYNC_FUTEX_H
