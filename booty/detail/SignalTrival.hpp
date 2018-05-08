/*
 * Sigle Signal with Multi-Functors relationship class impl.
 * Use it when you're intend to connect more than one functors with sigle signal.
 * @Simoncqk - 2018.05.08
 */
#ifndef BOOTY_DETAIL_SIGNALTRIVAL_HPP
#define BOOTY_DETAIL_SIGNALTRIVAL_HPP
#include <functional>
#include <vector>

namespace booty {

	namespace detail {

		template <typename Signature>
		class SignalTrival;

		/// Connect one signal with multi-slots, you may invoke all these connected
		/// functors at one time manually, but ATTENTION: the return value can not be 
		/// get, so use it with a call-back mechanism.
		template <typename Return, typename... Args>
		class SignalTrival<Return(Args...)> {

			using Functor = std::function<Return(Args...)>;
			using Slot = std::function<void()>;  // functor after wrapped.

		public:
			void connect(Functor&& func, Args&&... args) {
				slots_.push_back(
					std::bind(std::forward<Functor>(func), std::forward<Args>(args)...)
				);
			}

			void call() {
				for (auto&& slot : slots_)
					slot();
			}

			void unconnectAll() {
				slots_.clear();
			}
		private:
			std::vector<Slot> slots_;
		};

	}  // namespace detail

}  // namespace booty

#endif