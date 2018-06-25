#ifndef BOOTY_BASE_TIMESTAMP_H
#define BOOTY_BASE_TIMESTAMP_H

#include<chrono>
#include<string>
#include<ctime>

using namespace std::chrono;

namespace booty {

	class TimeStamp {

		using TimeType = int64_t;

	public:

		constexpr static int kMicrosecondsPerSecond = 1000 * 1000;

		TimeStamp()
			:microsecondsSinceEpoch_(0) {}

		explicit TimeStamp(TimeType timesince)
			:microsecondsSinceEpoch_(timesince) {}

		// default duration unit is `microseconds` so we don't have to do timepoint cast.
		static TimeStamp now() {
			return TimeStamp(std::chrono::system_clock::now().time_since_epoch().count());
		}

		static TimeStamp fromUnixTime(time_t t) {
			return TimeStamp(static_cast<TimeType>(t)*kMicrosecondsPerSecond);
		}

		void swap(TimeStamp& ts) {
			std::swap(microsecondsSinceEpoch_, ts.microsecondsSinceEpoch_);
		}

		TimeType microsecondsSinceEpoch() const {
			return microsecondsSinceEpoch_;
		}

		time_t secondsSinceEpoch() const {
			return static_cast<time_t>(microsecondsSinceEpoch_ / kMicrosecondsPerSecond);
		}

		std::string toString() const {
			char buff[32] = { 0 };
			TimeType seconds = microsecondsSinceEpoch_ / kMicrosecondsPerSecond;
			TimeType microseconds = microsecondsSinceEpoch_ % kMicrosecondsPerSecond;
			std::snprintf(buff, sizeof(buff) - 1, "%lld.%06lld", seconds, microseconds);
			return buff;
		}

		std::string toFormattedString(bool showMicroseconds = true) const {
			using namespace std::chrono;
			char buff[64] = { 0 };
			std::time_t now = system_clock::to_time_t(system_clock::now());
			std::tm tm_time = (*std::localtime(&now));
			if (showMicroseconds) {
				int microseconds = static_cast<int>(microsecondsSinceEpoch_%kMicrosecondsPerSecond);
				snprintf(buff, sizeof(buff), "%4d%02d%02d %02d:%02d:%02d.%06d",
					tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_hour,
					tm_time.tm_min, tm_time.tm_sec, microseconds);
			}
			else {
				snprintf(buff, sizeof(buff), "%4d%02d%02d %02d:%02d:%02d",
					tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
					tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
			}
			return buff;
		}

	private:
		TimeType microsecondsSinceEpoch_;
	};

	inline bool operator < (const TimeStamp& lhs, const TimeStamp& rhs) {
		return lhs.microsecondsSinceEpoch() < rhs.microsecondsSinceEpoch();
	}

	inline bool operator > (const TimeStamp& lhs, const TimeStamp& rhs) {
		return lhs.microsecondsSinceEpoch() > rhs.microsecondsSinceEpoch();
	}

	inline bool operator <= (const TimeStamp& lhs, const TimeStamp& rhs) {
		return lhs.microsecondsSinceEpoch() <= rhs.microsecondsSinceEpoch();
	}

	inline bool operator >= (const TimeStamp& lhs, const TimeStamp& rhs) {
		return lhs.microsecondsSinceEpoch() >= rhs.microsecondsSinceEpoch();
	}

	inline bool operator == (const TimeStamp& lhs, const TimeStamp& rhs) {
		return lhs.microsecondsSinceEpoch() == rhs.microsecondsSinceEpoch();
	}

	inline bool operator != (const TimeStamp& lhs, const TimeStamp& rhs) {
		return lhs.microsecondsSinceEpoch() != rhs.microsecondsSinceEpoch();
	}

	inline double TimeDifference(const TimeStamp& former, const TimeStamp& later) {
		auto diff = former.microsecondsSinceEpoch() - later.microsecondsSinceEpoch();
		return static_cast<double>(diff) / TimeStamp::kMicrosecondsPerSecond;
	}

	inline TimeStamp AddTime(const TimeStamp& ts, double seconds) {
		int64_t delta = static_cast<int64_t>(seconds*TimeStamp::kMicrosecondsPerSecond);
		return TimeStamp(ts.microsecondsSinceEpoch() + delta);
	}

}

#endif // !BOOTY_BASE_TIMESTAMP_H
