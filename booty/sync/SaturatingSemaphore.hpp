/*
 * This is a derivative snippet of Facebook::folly, under Apache Lisence.
 * Indention:
 * - Recurrent the design idea of seniors and rewrite some details to adapt
 *   personal considerations as components of booty.
 *
 * @Simoncqk - 2018.05.03
 *
 */
#ifndef BOOTY_SYNC_SATURATINGSEMAPHORE_HPP
#define BOOTY_SYNC_SATURATINGSEMAPHORE_HPP

#include<atomic>
#include<chrono>
#include<cassert>

#include"./Spin.h"

namespace booty {

	namespace sync {
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
		///
		/// Wait options:
		///   WaitOptions contains optional per call setting for spin-max duration:
		///   Calls to wait(), try_wait_until(), and try_wait_for() block only after the
		///   passage of the spin-max period. The default spin-max duration is 10 usec.
		///   The spin-max option is applicable only if MayBlock is true.

		template<bool MayBlock, template<typename> class Atom = std::atomic>
		class SaturatingSemaphore {
			enum class State{
				NOT_READY,
				READY,
				BLOCKED
			};
			
		public:
			inline static WaitOptions wait_options() {
				return {};  // same as WaitOptions(), but reduce one copy.
			}

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
			inline bool try_wait_until(const std::chrono::time_point<Clock,Duration>& ddl,
				const WaitOptions& opt=wait_options()) noexcept {
				if (try_wait())
					return true;
				return tryWaitSlow(ddl,opt);
			}

			template<class Rep,class Period>
			inline bool try_wait_for(const std::chrono::duration<Rep, Period>& duration,
				const WaitOptions& opt=wait_options())noexcept {
				if (try_wait())
					return true;
				auto ddl = std::chrono::steady_clock::now() + duration;
				return tryWaitSlow(ddl,opt);
			}

			~SaturatingSemaphore() {}

		private:
			inline void postFastWaiterMayBlock() noexcept {
				State before = State::NOT_READY;
				if (state_.compare_exchange_strong(before, State::READY, std::memory_order_release, std::memory_order_relaxed))
					return;
				postSlowWaiterMayBlock(before);
			}

			void postSlowWaiterMayBlock(const State& state_before) noexcept {
				while (true) {
					if (state_before == State::NOT_READY) {
						if (state_.compare_exchange_strong(state_before, State::READY,
							std::memory_order_release, std::memory_order_relaxed)) {
							return;
						}
					}
					if (state_before == State::READY) { // Only if multiple posters
						// The reason for not simply returning (without the following
						// steps) is to prevent the following case:
					    //
						//  T1:             T2:             T3:
						//  local1.post();  local2.post();  global.wait();
						//  global.post();  global.post();  global.reset();
						//                                  seq_cst fence
						//                                  local1.try_wait() == true;
						//                                  local2.try_wait() == false;
						//
						// This following steps correspond to T2's global.post(), where
						// global is already posted by T1.
						//
						// The following fence and load guarantee that T3 does not miss
						// T2's prior stores, i.e., local2.post() in this example.
						//
						// The following case is prevented:
						//
						// Starting with local2 == NOTREADY and global == READY
						//
						// T2:                              T3:
						// store READY to local2 // post    store NOTREADY to global // reset
						// seq_cst fenc                     seq_cst fence
						// load READY from global // post   load NOTREADY from local2 // try_wait

						/* sync all threads util reach this fence */
						std::atomic_thread_fence(std::memory_order_seq_cst);
						if (state_.load(std::memory_order_relaxed) == State::READY) {
							return;
						}
						continue;
					}
					assert(state_before == State::BLOCKED);
					if (state_.compare_exchange_strong(state_before, State::READY,
						std::memory_order_release, std::memory_order_relaxed)) {
						// TODO: state.futexWake();
						return;
					}
				}
			}

			template<typename Clock,typename Duration>
			bool tryWaitSlow(const std::chrono::time_point<Clock, Duration>& ddl,
				const WaitOptions& opt) noexcept {

			}

			Atom<State> state_;
		};
	}
}

#endif // !BOOTY_SYNC_SATURATINGSEMAPHORE_HPP

