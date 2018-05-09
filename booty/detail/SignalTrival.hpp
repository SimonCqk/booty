/*
 * Sigle Signal with Multi-Functors relationship class impl.
 * Use it when you're intend to connect more than one functors with sigle signal.
 * It is thread-safe, guaranteed by std::shared_mutex. 
 * @Simoncqk - 2018.05.08
 */
#ifndef BOOTY_DETAIL_SIGNALTRIVAL_HPP
#define BOOTY_DETAIL_SIGNALTRIVAL_HPP

#include <functional>
#include <vector>
#include<shared_mutex>

namespace booty {

	namespace detail {

		template <typename Signature>
		class SignalTrival;

		/// Connect one signal with multi-slots, you may invoke all these connected
		/// functors at one time manually, but ATTENTION: the return value can not be 
		/// get, so use it with a call-back mechanism.
		///
		/// Use it like:
 		/// - SignalTrival<void()> signal;
		///   signal.connect([](){ std::cout << "Hello Booty.\n"; });
		///   ...
		///   signal.call();
		/// - SignalTrival<void(int)> signal;
		///   signal.connect([](int x){ std::cout << "Hello x:" << x; });
		///   ...
		///   signal.call();
		template <typename Return, typename... Args>
		class SignalTrival<Return(Args...)> {

			using Functor = std::function<Return(Args...)>;
			using Slot = std::function<void()>;  // functor after wrapped.

		public:
			void connect(Functor&& func, Args&&... args) {
				std::lock_guard<std::shared_mutex> lock{ smtx_ };
				slots_.push_back(
					std::bind(std::forward<Functor>(func), std::forward<Args>(args)...)
				);
			}

			void call() {
				std::shared_lock<std::shared_mutex> lock{ smtx_ };
				for (auto&& slot : slots_)
					slot();
			}

			void unconnectAll() {
				std::lock_guard<std::shared_mutex> lock{ smtx_ };
				slots_.clear();
			}
		private:
			mutable std::shared_mutex smtx_;
			std::vector<Slot> slots_;
		};

	}  // namespace detail

}  // namespace booty

#endif