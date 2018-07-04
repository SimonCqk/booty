#ifndef BOOTY_BASE_TIMER_HPP
#define BOOTY_BASE_TIMER_HPP

#include<atomic>
#include<functional>
#include"Timestamp.hpp"

namespace booty {

	using Callback = std::function<void()>;

	class Timer {
	public:

		Timer(const Callback& cb, const Timestamp& when, double interval)
			:callback_(cb), expiration_(when), interval_(interval), repeat_(interval > 0.0), sequence_(numCreated_.fetch_add(1)) {}

		Timer(Callback&& cb, const Timestamp& when, double interval)
			:callback_(std::move(cb)), expiration_(when), interval_(interval), repeat_(interval > 0.0), sequence_(numCreated_.fetch_add(1)) {}

		void run() const {
			callback_();
		}

		Timestamp expiration() const {
			return expiration_;
		}

		bool repeat() const {
			return repeat_;
		}

		long sequence() const {
			return sequence_;
		}

		void restart(Timestamp now) {
			// TODO.
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

