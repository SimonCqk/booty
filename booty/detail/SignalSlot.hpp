/*
 * Supports for single-sigle-multi-slots connection.
 * You can easily achieve callback, signal-passing...etc by this simple
 * but applied implementation. It's thread-safe.
 * @Simoncqk - 2018.05.07
 */
#ifndef BOOTY_DETAIL_SIGNALSLOT_HPP
#define BOOTY_DETAIL_SIGNALSLOT_HPP

#include<functional>
#include<memory>
#include<vector>
#include<mutex>
#include<cassert>

namespace booty {

	namespace detail {

		template<typename CallBack>
		class SlotImpl;

		template<typename CallBack>
		struct SignalImpl {

			using Slots = std::vector<std::weak_ptr<SlotImpl<CallBack>>>;

		public:
			SignalImpl()
				:slots_(new Slots) {}

			/* when !slot_.unique(), more than one readers are visiting slots,
			 * and every reader has its own stack-obj to reach slots, so what we
			 * have to do is to make a hole copy and reset reference-count, then
			 * it is to be safe to write on new slots_.
			 */
			void copyOnWrite() {

				if (!slots_.unique()) {
					slots_.reset(new Slots(*slots_));
				}
				assert(slots_.unique());
			}

			// clean does writing.
			void clean() {
				std::lock_guard<std::mutex> lock{ mtx_ };
				copyOnWrite();
				Slots& copy(*slots_);
				for (auto beg = copy.begin(), last = copy.end();
					beg != last;) {
					if (beg->expired()) {
						beg = copy.erase(beg);
					}
					else
						++beg;
				}
			}

			SignalImpl(const SignalImpl&) = delete;
			SignalImpl& operator=(const SignalImpl&) = delete;
		private:
			std::mutex mtx_;
			std::shared_ptr<Slots> slots_;
		};

		template<typename CallBack>
		class SlotImpl {

			using CoreData = SignalImpl<CallBack>;

		public:
			explicit SlotImpl(const std::shared_ptr<CoreData>& data, CallBack&& cb)
				:data_(data), cb_(cb), is_tied_(false) {}

			explicit SlotImpl(const std::shared_ptr<CoreData>& data, CallBack&& cb,
				const std::shared_ptr<void>& tie)
				:data_(data), cb_(cb), tie_(tie), is_tied_(true) {}

			~SlotImpl() noexcept {
				std::shared_ptr<Data> data(data_.lock());
				if (data) {
					data->clean();
				}
			}

			SlotImpl(const SlotImpl&) = delete;
			SlotImpl& operator=(const SlotImpl&) = delete;
		private:
			CallBack cb_;
			std::weak_ptr<CoreData> data_;
			std::weak_ptr<void> tie_;
			bool is_tied_;
		};

	}  // namespace detail


	/// Handler of slot.
	/// This slot will remain connected to the signal fot the lifetime of the 
	/// returned Slot object as well as its copies.
	using Slot = std::shared_ptr<void>;

	template<typename Signature>
	class Signal;

	template<typename Return, typename... Args>
	class Signal<Return(Args...)> {

		using CallBack = std::function<void(Args...)>;
		using SignalImpl = detail::SignalImpl<CallBack>;
		using SlotImpl = detail::SlotImpl<CallBack>;
		using Tie = std::shared_ptr<void>;

	public:
		Signal()
			:impl_(new SignalImpl) {}

		~Signal() = default;

		Slot connect(CallBack&& func, Args&&... args) {
			std::shared_ptr<SlotImpl> slot_impl(new SlotImpl(
				impl_,
				std::bind(std::forward<CallBack>(func), std::forward<Args>(args)...)
			));
			add(slot_impl);
			return slot_impl;
		}

		Slot connect(CallBack&& func, const Tie& tie, Args&&... args) {
			std::shared_ptr<SlotImpl> slot_impl(new SlotImpl(
				impl_,
				std::bind(std::forward<CallBack>(func), std::forward<Args>(args)...),
				tie
			));
			add(slot_impl);
			return slot_impl;
		}

		/// Call all connected slots(callback functors at once.
		void call() {
			/* Both impl & slots are accessors to the member of Signal Impl.
			 * Use stack-obj as accessors to make it strong thread-safe and
			 * minimize the critical area.
			 */
			SignalImpl& impl{ *impl_ };
			std::shared_ptr<typename SignalImpl::Slots> slots;
			{
				std::lock_guard<std::mutex> lock{ impl.mtx_ };
				slots = impl.slots_;
			}
			typename SignalImpl::Slots& in_slots(*slots);
			for (auto beg = in_slots.cbegin(), last = in_slots.cend();
				beg != last; ++beg) {
				auto slot_impl = beg->lock();
				if (slot_impl) {
					if (slot_impl->is_tied_&&slot_impl.tie_.lock())
						slot_impl->cb_();
					else
						slot_impl->cb_();
				}
			}
		}

		Signal(const Signal&) = delete;
		Signal& operator=(const Signal&) = delete;
	private:
		void add(const std::shared_ptr<SlotImpl>& slot) {
			SignalImpl& impl(*impl_);
			{
				std::lock_guard<std::mutex> lock{ impl.mtx_ };
				impl.copyOnWrite();
				impl.slots_->push_back(slot);
			}
		}

		const std::shared_ptr<SignalImpl> impl_;
	};




}  // namespace booty

#endif // !BOOTY_DETAIL_SIGNALSLOT_HPP
