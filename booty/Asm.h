#pragma once

extern void _mm_pause(void);

inline void asm_volatile_pause() {
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
	::_mm_pause();
#elif defined(__i386__) || FOLLY_X64
	asm volatile("pause");
#elif FOLLY_AARCH64 || defined(__arm__)
	asm volatile("yield");
#elif FOLLY_PPC64
	asm volatile("or 27,27,27");
#endif
}