/*
 * This is a derivative snippet of Facebook::folly, under Apache Lisence.
 * Indention:
 * - Recurrent the design idea of seniors and rewrite some details to adapt
 *   personal considerations as components of booty.
 *
 * @Simoncqk - 2018.05.02
 *
 */
#pragma once

namespace booty {
#ifdef NDEBUG
	constexpr bool kIsDebug = false;
#else
	constexpr bool kIsDebug = true;
#endif // NDEBUG

} // namespace booty
