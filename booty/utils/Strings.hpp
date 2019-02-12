/*
 * String.hpp include some util/help functions to make string-handling easier.
 * @Simoncqk - 2018.08
 */
#ifndef BOOTY_UTILS_STRINGS_HPP
#define BOOTY_UTILS_STRINGS_HPP

#include <string>
#include <vector>

namespace booty {

	namespace strings {

		/// Split string into words by given separator.
		std::vector<std::string> Split(const std::string& str, const char& sep = '\n') {
			if (str.size() <= 1) return { str };
			std::vector<std::string> res;
			auto beg = str.cbegin(), end = str.cend(), mover = str.cbegin();
			while (mover != end) {
				while (mover != end && (*mover) != sep) ++mover;
				res.emplace_back(beg, mover);
				while ((*mover) == sep) ++mover;  // skip redundent seperators.
				beg = mover;
			}
			return res;
		}

		/// Judge whether string starts with the given substring.
		bool StartWith(const std::string& str, const std::string& sub) {
			if (str.size() < sub.size()) return false;
			for (auto beg = str.cbegin(), end = str.cend(), sbeg = sub.cbegin(), send = sub.cend();
				beg != end; ++beg, ++sbeg) {
				if (sbeg == send)break;
				if ((*beg) != (*sbeg)) return false;
			}
			return true;
		}

		/// Judge whether string ends with the given substring.
		bool EndWith(const std::string& str, const std::string& sub) {
			if (str.size() < sub.size()) return false;
			for (auto beg = str.crbegin(), end = str.crend(), sbeg = sub.crbegin(), send = sub.crend();
				beg != end; ++beg, ++sbeg) {
				if (sbeg == send)break;
				if ((*beg) != (*sbeg)) return false;
			}
			return true;
		}

	} // namespace strings

}  // namespace booty

#endif