/*
 * String.hpp include some util/help functions to make string-handling easier.
 * @Simoncqk - 2018.08
 */
#ifndef BOOTY_UTILS_STRINGS_HPP
#define BOOTY_UTILS_STRINGS_HPP

#include <string>
#include <vector>

namespace booty {

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
}  // namespace booty

#endif