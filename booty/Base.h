/*
 * This file includes some base classes with specific features.
 * They are used to inherited and expand.
 * @Simoncqk - 2018.05.10
 */

#ifndef BOOTY_BASE_H
#define BOOTY_BASE_H

namespace booty {

	class NonCopyable {
		// forbid copy ctor and copy assignment operator.
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;
	};

	class 

} // booty

#endif // !BOOTY_BASE_H
