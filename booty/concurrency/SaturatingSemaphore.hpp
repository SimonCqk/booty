/*
* This is a derivative snippet of Facebook::folly, under Apache Lisence.
* Indention:
* - Recurrent the design idea of seniors and rewrite some details to adapt
*   personal considerations as components of booty.
*
* @Simoncqk - 2018.05.03
*/
#ifndef BOOTY_CONCURRENCY_SATURATINGSEMAPHORE_HPP
#define BOOTY_CONCURRENCY_SATURATINGSEMAPHORE_HPP

#include<atomic>
#include<chrono>

namespace booty {

	namespace concurrency {
		/// SaturatingSemaphore is a flag that allows concurrent posting by
		/// multiple posters and concurrent non-destructive waiting by
		/// multiple waiters.
		///
		/// A SaturatingSemaphore allows one or more waiter threads to check,
		/// spin, or block, indefinitely or with timeout, for a flag to be set
		/// by one or more poster threads. By setting the flag, posters
		/// announce to waiters (that may be already waiting or will check
		/// the flag in the future) that some condition is true. Posts to an
		/// already set flag are idempotent.
		///
		/// SaturatingSemaphore is called so because it behaves like a hybrid
		/// binary/counted _semaphore_ with values zero and infinity, and
		/// post() and wait() functions. It is called _saturating_ because one
		/// post() is enough to set it to infinity and to satisfy any number
		/// of wait()-s. Once set (to infinity) it remains unchanged by
		/// subsequent post()-s and wait()-s, until it is reset() back to
		/// zero.
		///
		/// The implementation of SaturatingSemaphore is based on that of
		/// Baton. It includes no internal padding, and is only 4 bytes in
		/// size. Any alignment or padding to avoid false sharing is up to
		/// the user.
		/// SaturatingSemaphore differs from Baton as follows:
		/// - Baton allows at most one call to post(); this allows any number
		///   and concurrently.
		/// - Baton allows at most one successful call to any wait variant;
		///   this allows any number and concurrently.
		///
		/// Template parameter:
		/// - bool MayBlock: If false, waiting operations spin only. If
		///   true, timed and wait operations may block; adds an atomic
		///   instruction to the critical path of posters.

		template<bool MayBlock, template<typename> class Atom = std::atomic>
		class SaturatingSemaphore {
			enum class State{
				NOT_READY,
				READY,
				BLOCKED
			};
			
		public:
			constexpr SaturatingSemaphore()noexcept
				:state_(State::NOT_READY){}

			inline bool ready() const noexcept {
				return state_.load(std::memory_order_acquire) == State::READY;
			}

			void reset() noexcept {
				state_.store(State::NOT_READY, std::memory_order_release);
			}

			inline void post() noexcept {
				if (!MayBlock) {
					state_.store(State::READY, std::memory_order_release);
				}
				else {
					postFastWaiterMayBlock();
				}
			}

			inline void wait() noexcept {
				// wait as long as it can
				try_wait_until(std::chrono::steady_clock::time_point::max());
			}

			inline bool try_wait() noexcept {
				return ready();
			}

			template<typename Clock,typename Duration>
			inline bool try_wait_until(const std::chrono::time_point<Clock,Duration>& ddl) noexcept {
				if (try_wait())
					return true;
				return tryWaitSlow(ddl);
			}

			template<class Rep,class Period>
			inline bool try_wait_for(const std::chrono::duration<Rep, Period>& duration)noexcept {
				if (try_wait())
					return true;
				auto ddl = std::chrono::steady_clock::now() + duration;
				return tryWaitSlow(ddl);
			}

			~SaturatingSemaphore() {}

		private:
			inline void postFastWaiterMayBlock() noexcept {
				State before = State::NOT_READY;
				if (state_.compare_exchange_strong(before, State::READY, std::memory_order_release, std::memory_order_relaxed))
					return;
				postSlowWaiterMayBlock(before);
			}

			void postSlowWaiterMayBlock(State state_before) noexcept {

			}

			template<typename Clock,typename Duration>
			bool tryWaitSlow(const std::chrono::time_point<Clock, Duration>& ddl) noexcept {

			}

			Atom<State> state_;
		};
	}


}

#endif // !BOOTY_CONCURRENCY_SATURATINGSEMAPHORE_HPP

