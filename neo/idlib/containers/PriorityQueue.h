/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#ifndef __PRIORITY_QUEUE_H__
#define __PRIORITY_QUEUE_H__

#pragma once


// Convenience key extractors (optional helpers)
template <class T, class P>
struct MemberPriorityKey {
	P T::* member;
	constexpr MemberPriorityKey(P T::* m) noexcept : member(m) {}
	constexpr P operator()(T const& t) const noexcept { return t.*member; }
};

template <class T, class P>
struct MethodPriorityKey {
	P(T::* method)() const;
	constexpr MethodPriorityKey(P(T::* m)() const) noexcept : method(m) {}
	constexpr P operator()(T const& t) const noexcept { return (t.*method)(); }
};

/*
===============================================================================

	idPriorityQueue
	- intrusive, derived from idQueue
	- LOWEST key at the front by default
	- stable for equal keys (new items go AFTER existing equal-key items)

Template params:
 - type     : payload type (must contain idQueueNode<type> at nodePtr)
 - nodePtr  : pointer-to-member specifying the intrusive node
 - KeyOf    : callable Priority(const type&) extracting the sort key
 - Compare  : strict weak ordering on Priority; default std::less<> (lower first)

===============================================================================
*/
template<
	class type,
	idQueueNode<type> type::* nodePtr,
	class KeyOf,
	class Compare = std::less<>
>
class idPriorityQueue : public idQueue<type, nodePtr> {
public:
	using base_t = idQueue<type, nodePtr>;

	constexpr idPriorityQueue(KeyOf key_of, Compare comp = Compare{})
		: key_of_(std::move(key_of)), comp_(std::move(comp)) {
	}

	using base_t::Add;
	using base_t::RemoveFirst;
	using base_t::Peek;
	using base_t::IsEmpty;


	// Insert keeping ascending order; stable for ties.
	void Add(type* element) {
		// Ensure element starts with a clean next pointer
		set_next(element, nullptr);

		// Fast path: empty queue
		if (this->first == nullptr) {
			this->first = this->last = element;
			return;
		}

		const auto newKey = key_of_(*element);

		// Insert at head if newKey < headKey
		if (comp_(newKey, key_of_(*this->first))) {
			set_next(element, this->first);
			this->first = element;
			return;
		}

		// Walk until we find first node with key > newKey (to be stable on equals)
		type* prev = this->first;
		type* cur = get_next(prev);

		while (cur) {
			const auto curKey = key_of_(*cur);

			// We insert BEFORE cur if newKey < curKey
			if (comp_(newKey, curKey)) break;

			// Otherwise (newKey >= curKey), advance to keep stability for ties
			prev = cur;
			cur = get_next(cur);
		}

		// Splice element after prev, before cur
		set_next(element, cur);
		set_next(prev, element);

		// If appended at tail, update last
		if (cur == nullptr) {
			this->last = element;
		}
	}

private:
	// Small helpers to access the intrusive link via nodePtr
	static ID_INLINE type* get_next(type* n) noexcept {
		return (n->*nodePtr).GetNext();
	}
	static ID_INLINE void set_next(type* n, type* nxt) noexcept {
		(n->*nodePtr).SetNext(nxt);
	}

	KeyOf   key_of_;
	Compare comp_;
};

// ---------- Convenience builders (optional) ----------

template<
	class type,
	idQueueNode<type> type::* nodePtr,
	class Priority,
	class Compare = std::less<>
>
constexpr auto MakeMemberPriorityQueue(Priority type::* member, Compare comp = Compare{}) {
	using KeyOf = MemberPriorityKey<type, Priority>;
	return idPriorityQueue<type, nodePtr, KeyOf, Compare>(KeyOf{ member }, std::move(comp));
}

template<
	class type,
	idQueueNode<type> type::* nodePtr,
	class Priority,
	class Compare = std::less<>
>
constexpr auto MakeMethodPriorityQueue(Priority(type::* method)() const, Compare comp = Compare{}) {
	using KeyOf = MethodPriorityKey<type, Priority>;
	return idPriorityQueue<type, nodePtr, KeyOf, Compare>(KeyOf{ method }, std::move(comp));
}

#endif // __PRIORITY_QUEUE_H__