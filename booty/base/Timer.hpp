#ifndef BOOTY_BASE_TIMER_HPP
#define BOOTY_BASE_TIMER_HPP

#include<atomic>
#include<functional>
#include"Timestamp.hpp"

namespace booty {

	using Callback = std::function<void()>;

	/// A clean and clear timer impl, this is not a timer call interruption automatically,
	/// but users control the initiative to judge whether it's expired or not, by invoking
	/// interfaces, and events(callbacks) should be registered.
	class Timer {
	public:

		Timer(const Callback& cb, const Timestamp& when, double interval)
			:callback_(cb),
			expiration_(when),
			interval_(interval),
			repeat_(interval > 0.0),
			sequence_(numCreated_.fetch_add(1)) {}

		Timer(Callback&& cb, const Timestamp& when, double interval)
			:callback_(std::move(cb)),
			expiration_(when),
			interval_(interval),
			repeat_(interval > 0.0),
			sequence_(numCreated_.fetch_add(1)) {}

		// run the registered event, driven by this timer.
		void run() const {
			callback_();
		}

		Timestamp expiration() const {
			return expiration_;
		}

		bool expired() const {
			return expiration_ > Timestamp::now();
		}

		bool repeat() const {
			return repeat_;
		}

		// sequence number of this timer
		long sequence() const {
			return sequence_;
		}

		// reset the expiration timestamp of this timer
		void restart(Timestamp now) {
			if (repeat_) {
				expiration_ = AddTime(now, interval_);
			}
			else {
				expiration_ = Timestamp();
			}
		}

		static long numCreated() {
			return numCreated_.load();
		}

	private:
		const Callback callback_;
		Timestamp expiration_;
		const double interval_;
		const bool repeat_;
		const long sequence_;

		static std::atomic<long> numCreated_;
	};

	std::atomic<long> Timer::numCreated_{ 0 };
}

#endif // !BOOTY_BASE_TIMER_HPP

