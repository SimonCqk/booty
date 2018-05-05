/*
 * This is a derivative snippet of Facebook::folly, under Apache Lisence.
 * Indention:
 * - Recurrent the design idea of seniors and rewrite some details to adapt
 *   personal considerations as components of booty.
 *
 * @Simoncqk - 2018.05.03
 *
 */
#ifndef BOOTY_UNIT_H
#define BOOTY_UNIT_H

#include<type_traits>

namespace booty {
	/// In functional programming, the degenerate case is often called "unit". In
	/// C++, "void" is often the best analogue. However, because of the syntactic
	/// special-casing required for void, it is frequently a liability for template
	/// metaprogramming. So, instead of writing specializations to handle cases like
	/// SomeContainer<void>, a library author may instead rule that out and simply
	/// have library users use SomeContainer<Unit>. Contained values may be ignored.
	/// Much easier.
	///
	/// "void" is the type that admits of no values at all. It is not possible to
	/// construct a value of this type.
	/// "unit" is the type that admits of precisely one unique value. It is
	/// possible to construct a value of this type, but it is always the same value
	/// every time, so it is uninteresting.

	/* 
	---------------------- folly is sooooo awesome ----------------------------
	   Here is my personal understanding: 
	   - "void" can be represented as `Any Type You Like`, except instantiation.
	   - "unit" can also be represented as above, HOWEVER, once instantiated, the
	            type will settled and being unchangable.
	 */
	struct Unit {
		// These are structs rather than type aliases because MSVC 2017 RC has
		// trouble correctly resolving dependent expressions in type aliases
		// in certain very specific contexts, including a couple where this is
		// used. See the known issues section here for more info:
		// https://blogs.msdn.microsoft.com/vcblog/2016/06/07/expression-sfinae-improvements-in-vs-2015-update-3/

		template <typename T>
		struct Lift : std::conditional<std::is_same<T, void>::value, Unit, T> {};
		template <typename T>
		using LiftT = typename Lift<T>::type;
		template <typename T>
		struct Drop : std::conditional<std::is_same<T, Unit>::value, void, T> {};
		template <typename T>
		using DropT = typename Drop<T>::type;

		constexpr bool operator==(const Unit& /*other*/) const {
			return true;
		}
		constexpr bool operator!=(const Unit& /*other*/) const {
			return false;
		}
	};

}

#endif // !BOOTY_UNIT_H
