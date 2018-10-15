#ifndef BOOTY_BASE_TIMESTAMP_H
#define BOOTY_BASE_TIMESTAMP_H

#include<chrono>
#include<string>
#include<ctime>

namespace booty {

	/// Timestamp impl.
	class Timestamp {

		// `long long int` is not always 64 bit which differs in different platforms.
		// For uniting, define Time Type a 64 bit integer.
		using TimeType = int64_t;
		using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

	public:

		constexpr static int kMicrosecondsPerSecond = 1000 * 1000;

		Timestamp()
			:epochTimePoint_(std::chrono::system_clock::now()) {}

		explicit Timestamp(TimeType timesince)
			:epochTimePoint_(std::chrono::system_clock::now() + std::chrono::microseconds(timesince)) {}

		// default duration unit is `microseconds` so we don't have to do timepoint cast.
		static Timestamp now() {
			return Timestamp(std::chrono::system_clock::now().time_since_epoch().count());
		}

		static Timestamp fromUnixTime(time_t t) {
			return Timestamp(static_cast<TimeType>(t)*kMicrosecondsPerSecond);
		}

		void swap(Timestamp& ts) {
			std::swap(epochTimePoint_, ts.epochTimePoint_);
		}

		TimeType microsecondsSinceEpoch() const {
			return epochTimePoint_.time_since_epoch().count();
		}

		time_t secondsSinceEpoch() const {
			return static_cast<time_t>(epochTimePoint_.time_since_epoch().count() / kMicrosecondsPerSecond);
		}

		// toString() provides simple formatted time presentation:
		// e.g. 12.345678s
		std::string toString() const {
			char buff[32] = { 0 };
			TimeType seconds = epochTimePoint_.time_since_epoch().count() / kMicrosecondsPerSecond;
			TimeType microseconds = epochTimePoint_.time_since_epoch().count() % kMicrosecondsPerSecond;
			std::snprintf(buff, sizeof(buff) - 1, "%lld.%06lld", seconds, microseconds);
			return buff;
		}

		// toFormattedString() provides detailed formatted time presentation, and contains
		// more information like year/month...arg `showMicroseconds` specify to show microseconds
		// in decimal point or not:
		// e.g. showMicroseconds = true: 2018-06-25 22:25:30.123456
		//      showMicroseconds = false: 2018-06-25 22:25:30
		std::string toFormattedString(bool showMicroseconds = true) const {
			using namespace std::chrono;
			char buff[64] = { 0 };
			std::time_t now = system_clock::to_time_t(system_clock::now());
			std::tm tm_time = (*std::localtime(&now));
			if (showMicroseconds) {
				int microseconds = static_cast<int>(epochTimePoint_.time_since_epoch().count() %kMicrosecondsPerSecond);
				snprintf(buff, sizeof(buff), "%4d-%02d-%02d %02d:%02d:%02d.%06d",
					tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_hour,
					tm_time.tm_min, tm_time.tm_sec, microseconds);
			}
			else {
				snprintf(buff, sizeof(buff), "%4d-%02d-%02d %02d:%02d:%02d",
					tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
					tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
			}
			return buff;
		}

		// Define some comparation-operators.
		bool operator < (const Timestamp& ts) {
			return epochTimePoint_ < ts.epochTimePoint_;
		}

		bool operator > (const Timestamp& ts) {
			return epochTimePoint_ > ts.epochTimePoint_;
		}

		bool operator <= (const Timestamp& ts) {
			return epochTimePoint_ <= ts.epochTimePoint_;
		}

		bool operator >= (const Timestamp& ts) {
			return epochTimePoint_ >= ts.epochTimePoint_;
		}

		bool operator == (const Timestamp& ts) {
			return epochTimePoint_ == ts.epochTimePoint_;
		}

		bool operator != (const Timestamp& ts) {
			return epochTimePoint_ != ts.epochTimePoint_;
		}

	private:
		TimePoint epochTimePoint_;
	};

	// Some utility functions defined.
	// Time difference between two timestamps, return by double.
	inline double TimeDifference(const Timestamp& former, const Timestamp& later) {
		auto diff = former.microsecondsSinceEpoch() - later.microsecondsSinceEpoch();
		return static_cast<double>(diff) / Timestamp::kMicrosecondsPerSecond;
	}
	// Add more time to original timestamp, return a new TimeStamp and do not
	// modify original one.
	inline Timestamp AddTime(const Timestamp& ts, double seconds) {
		int64_t delta = static_cast<int64_t>(seconds*Timestamp::kMicrosecondsPerSecond);
		return Timestamp(ts.microsecondsSinceEpoch() + delta);
	}

}

#endif // !BOOTY_BASE_TIMESTAMP_H
