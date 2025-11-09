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

#ifndef __TABLE_H__
#define __TABLE_H__

#pragma once

#include <cstdio>
#include <cstring>
#include <tuple>
#include <vector>
#include <array>
#include <type_traits>

template<class... Ts>
constexpr auto Column(Ts&&... ts) {
	return std::tuple{ std::forward<Ts>(ts)... }; // CTAD
}

enum class ColumnAlignment : uint8
{
	Left,
	Right,
	Center
};

// ---------- Table with heterogeneous column types ----------
template<class... ColTs>
class Table {
public:
	static constexpr size_t columnCount = sizeof...(ColTs);
	using Row = std::tuple<ColTs...>;
	using RowFormatter = size_t(*)(const Row&, char*, size_t);

	explicit Table()
	{
		screenAlignedTabs = true;
		cellPaddingChars = 1;
		rows = {};
		columns = {};
		totalWidth = 0;
	}

	// ---------- SetColumns: title required; others optional, order-insensitive ----------
	// Each descriptor is either:
	//   - "Title"
	//   - tuple("Title", [Align], ["%fmt"], [RowFormatter])   // any order after title
	template<typename... Descs>
	void SetColumns(Descs&&... descs) {
		static_assert(sizeof...(Descs) == columnCount, "SetColumns: descriptor count must match number of columns");
		index_t index = 0;
		(ProcessColumn(index++, std::forward<Descs>(descs)), ...);
	}

	// headers
	ID_INLINE void SetHeaders(const idArray<const char*, columnCount>& headerTitles) { for (index_t i = 0; std::cmp_less(i, columnCount); ++i) { SetHeader(i, headerTitles[i]); } }
	ID_INLINE void SetHeader(const index_t columnIndex, const char* h) { VisitColumn(columnIndex, [&](auto& column) { column.header = h ? h : ""; }); }

	// alignment per column
	ID_INLINE void SetAlignment( const index_t columnIndex, const ColumnAlignment alignment ) { VisitColumn(columnIndex, [&](auto& column) { column.align = alignment; }); }
	ID_INLINE void SetRight( const index_t columnIndex ) { SetAlignment(columnIndex, ColumnAlignment::Right); }
	ID_INLINE void SetLeft( const index_t columnIndex ) { SetAlignment(columnIndex, ColumnAlignment::Left); }
	ID_INLINE void SetCenter( const index_t columnIndex ) { SetAlignment(columnIndex, ColumnAlignment::Center); }
	ID_INLINE void SetAlignmentAll( const ColumnAlignment alignment ) { for (index_t i = 0; std::cmp_less(i, columnCount); ++i) { SetAlignment(i, alignment); } }

	// options
	ID_INLINE void SetScreenAlignedTabs( const bool enable ) { screenAlignedTabs = enable; }
	ID_INLINE void SetCellPadding( const size_t paddingChars ) { cellPaddingChars = paddingChars; }

	// format
	ID_INLINE void SetFormat(size_t columnIndex, const char* spec) { VisitColumn(columnIndex, [&](auto& column) { column.fmt = spec; }); }
	ID_INLINE void SetFormatter(size_t columnIndex, RowFormatter formatter) { VisitColumn(columnIndex, [&](auto& column) { column.rowFormatter = formatter; }); }

	// rows
	ID_INLINE index_t AppendRow() { return rows.Append(Row{}); }
	ID_INLINE index_t AppendRow( const Row& row ) { return rows.Append(row); }
	template<class... Args>
	ID_INLINE index_t AppendRow( Args&&... rowArgs ) {
		static_assert(sizeof...(Args) == 0 || sizeof...(Args) == columnCount, "AppendRow: arity mismatch");
		return rows.append(Row(std::forward<Args>(rowArgs)...));
	}
	ID_INLINE index_t InsertRow( const index_t rowIndex ) { return rows.Insert(Row{}, rowIndex); }
	ID_INLINE index_t InsertRow( const index_t rowIndex, const Row& row ) { return rows.Insert(row, rowIndex); }
	template<class... Args>
	ID_INLINE index_t InsertRow(const index_t rowIndex, Args&&... rowArgs) {
		static_assert(sizeof...(Args) == 0 || sizeof...(Args) == columnCount, "InsertRow: arity mismatch");
		return rows.Insert(Row(std::forward<Args>(rowArgs)...), rowIndex);
	}
	ID_INLINE index_t UpdateRow( const index_t rowIndex ) { return rows[rowIndex] = Row{}; }
	ID_INLINE index_t UpdateRow( const index_t rowIndex, const Row& row ) { return rows[rowIndex] = row; }
	template<class... Args>
	ID_INLINE index_t UpdateRow( const index_t rowIndex, Args&&... rowArgs ) {
		static_assert(sizeof...(Args) == 0 || sizeof...(Args) == columnCount, "UpdateRow: arity mismatch");
		return rows[rowIndex] = Row(std::forward<Args>(rowArgs)...);
	}
	ID_INLINE index_t UpsertRow( const index_t rowIndex ) { return UpsertRow(rowIndex, Row{}); }
	ID_INLINE index_t UpsertRow( const index_t rowIndex, const Row& row )
	{
		// Guarantee row count > rowIndex by padding with default rows
		while (std::cmp_less(rows.Num(), rowIndex)) {
			AppendRow();
		}

		if (rowIndex < rows.Num()) {
			return UpdateRow(rowIndex, row);
		}
		else if (rowIndex == rows.Num()) {
			return AppendRow(row);
		}

		return -1;  // Something went wrong
	}
	template<class... Args>
	ID_INLINE index_t UpsertRow(const index_t rowIndex, Args&&... rowArgs) {
		static_assert(sizeof...(Args) == 0 || sizeof...(Args) == columnCount, "UpsertRow: arity mismatch");
		return rows.UpsertRow(rowIndex, Row(std::forward<Args>(rowArgs)...));
	}
	ID_INLINE index_t RemoveRow(const index_t rowIndex) { return rows.RemoveIndex(rowIndex); }
	ID_INLINE void ClearRows() { rows.Clear(); }

	// container-like
	[[nodiscard]] size_t Num() const noexcept { return rows.Num(); }
	[[nodiscard]] bool   Empty() const noexcept { return rows.Empty(); }
	[[nodiscard]] Row& operator[]( const index_t i ) { return rows[i]; }
	[[nodiscard]] const Row& operator[]( const index_t i ) const { return rows[i]; }
	[[nodiscard]] auto begin() noexcept { return rows.begin(); }
	[[nodiscard]] auto end() noexcept { return rows.end(); }
	[[nodiscard]] auto begin() const noexcept { return rows.begin(); }
	[[nodiscard]] auto end() const noexcept { return rows.end(); }
	[[nodiscard]] auto cbegin() const noexcept { return begin(); }
	[[nodiscard]] auto cend()   const noexcept { return end(); }

	// render
	ID_INLINE void Printf( const bool showSummaryFooter = true ) const;

private:
	template<class T>
	struct Column {
		idStr header = "";
		ColumnAlignment align = ColumnAlignment::Left;
		const char* fmt = nullptr;          // optional printf spec for this type
		RowFormatter rowFormatter = nullptr;          // optional row-aware formatter
		size_t width = 0;
	};

	template<size_t I = 0, class F>
	ID_INLINE void VisitColumn(std::size_t idx, F&& f);
	template<size_t I = 0, class F>
	ID_INLINE void VisitColumns(F&& f);
	template<size_t I = 0, class F>
	ID_INLINE void VisitColumns(F&& f) const;

	ID_INLINE void ProcessColumn(size_t i, const char* title);
	template<typename Tup>
	ID_INLINE void ProcessColumn(size_t i, Tup&& t);
	template<typename... Ts>
	ID_INLINE void ApplyColumnTuple(size_t i, const std::tuple<Ts...>& t);
	template<typename... Ts>
	static ID_INLINE const char* GetColumnHeaderFromTuple(const std::tuple<Ts...>& t);
	template<typename... Ts, size_t... Is>
	static ID_INLINE void SetColumnParametersFromTuple(ColumnAlignment& a, const char*& fmt, RowFormatter& rf, const std::tuple<Ts...>& t, std::index_sequence<Is...>);
	template<size_t I, typename... Ts>
	static ID_INLINE void ConsumeColumnTuple(ColumnAlignment& a, const char*& fmt, RowFormatter& rf, std::integral_constant<size_t, I>, const std::tuple<Ts...>& t);

	template<size_t I>
	void FormatCell( const Row& row, char* dest, size_t capacity ) const;

	// compute widths (tab-aware)
	void ComputeWidths();

	void PrintBorder( const bool useInnerDividers = true ) const;
	void PrintHeader() const;
	void PrintRow( const Row& row ) const;
	void PrintSummary() const;

private:
	bool   screenAlignedTabs;
	size_t cellPaddingChars;

	std::tuple<Column<ColTs>...> columns;
	idList<Row> rows;
	size_t totalWidth;
};

namespace tbl::detail {

	// ---------------- tab helpers ----------------
	static size_t visual_width_tabs(const char* s, size_t tabW, size_t startCol) {
		if (!s)
		{
			return 0;
		}
		if (!tabW)
		{
			tabW = 1;
		}
		size_t col = startCol;
		for (const unsigned char* p = reinterpret_cast<const unsigned char*>(s); *p; ++p) {
			unsigned char c = *p;
			if (c == '\t') {
				size_t adv = tabW - (col % tabW);
				if (!adv)
				{
					adv = tabW;
				}
				col += adv;
			}
			else if (c == '\r' || c == '\n' || c == '\v' || c == '\f') {
				// ignore in single-line cell width
			}
			else {
				++col;
			}
		}
		return col - startCol;
	}

	static void expand_tabs(char* dest, size_t destSize, const char* source, index_t startColumn) {
		if (!dest || !destSize)
		{
			return;
		}

		if (!source)
		{
			dest[0] = '\0';
			return;
		}

		index_t column = startColumn, out = 0;
		for (const unsigned char* p = reinterpret_cast<const unsigned char*>(source); *p; ++p)
		{
			unsigned char c = *p;

			if (c == '\t') {
				size_t advance = TAB_STOP_CHARS - (column % TAB_STOP_CHARS);
				if (!advance)
				{
					advance = TAB_STOP_CHARS;
				}
				while (advance--) {
					if (std::cmp_less(out + 1, destSize))
					{
						dest[out] = ' ';
					}
					++out; ++column;
				}
			}
			else if (c == '\r' || c == '\n' || c == '\v' || c == '\f') {
				// drop control newlines for single-line cells
			}
			else {
				if (std::cmp_less(out + 1, destSize))
				{
					dest[out] = static_cast<char>(c);
				}
				++out; ++column;
			}
		}

		dest[(std::cmp_less(out, destSize)) ? out : (destSize - 1)] = '\0';
	}

} // namespace tbl::detail



template<class... ColTs>
void Table<ColTs ...>::Printf(const bool showSummaryFooter) const {
	ComputeWidths();

	// Top Border
	PrintBorder();

	// Headers
	PrintHeader();

	// Mid Border
	PrintBorder();

	// Data
	for (const auto& row : rows) {
		PrintRow(row);
	}

	// Footer Border
	PrintBorder();

	if (showSummaryFooter)
	{
		// Summary
		PrintSummary();

		// Bottom Border
		PrintBorder(false);
	}
}

// ---------- private visiting ----------
template<class... ColTs>
template<size_t I, class F>
void Table<ColTs ...>::VisitColumn(std::size_t idx, F&& f) {
	if constexpr (I < columnCount) {
		if (idx == I) { f(std::get<I>(columns)); return; }
		VisitColumn<I + 1>(idx, std::forward<F>(f));
	}
}

template<class... ColTs>
template<size_t I, class F>
void Table<ColTs ...>::VisitColumns(F&& f) {
	if constexpr (I < columnCount) {
		f(std::get<I>(columns));
		VisitColumns<I + 1>(std::forward<F>(f));
	}
}

template<class... ColTs>
template<size_t I, class F>
void Table<ColTs ...>::VisitColumns(F&& f) const {
	if constexpr (I < columnCount) {
		f(std::get<I>(columns));
		VisitColumns<I + 1>(std::forward<F>(f));
	}
}

// compute widths (tab-aware)
template<class... ColTs>
void Table<ColTs ...>::ComputeWidths() {
	// headers
	VisitColumns([&](auto& column) {
		column.width = column.header.TabExpandedLengthWithoutColors();
		});

	// rows
	char char_buffer[MAX_STRING_CHARS] = { '\0' };
	for (const auto& row : rows) {
		VisitColumns([&]<std::size_t I>(auto& column) {
			FormatCell<I>(row, char_buffer, sizeof(char_buffer));
			const size_t w = idStr::TabExpandedLengthWithoutColors(char_buffer);
			if (w > column.width)
			{
				column.width = w;
			}
		});
	}

	size_t totalInner = 0;
	VisitColumns([&](auto& C) {
		totalInner += C.width;
		});
	totalWidth = totalInner + columns.Num() + 1; // + for borders
}

template<class... ColTs>
void Table<ColTs ...>::PrintBorder(const bool useInnerDividers) const {
	idStr outString('+');
	VisitColumns([&](const auto& column) {
		const size_t count = column.width + 2 * cellPaddingChars;
		outString.Appendn('-', count);
		if (useInnerDividers)
		{
			outString.Append("+");
		}
		else
		{
			outString.Append("-");
		}
		});
	outString.Append("\n");

	idLib::Printf(outString);
}

// header line
template<class... ColTs>
void Table<ColTs ...>::PrintHeader() const {
	idStr outString('|');
	size_t startCol = 1;
	char  expanded_buffer[MAX_STRING_CHARS * 2] = { '\0' };

	VisitColumns([&]<size_t I>(const auto& column) {
		for (size_t p = 0; p < cellPaddingChars; ++p)
		{
			outString.Append(' ');
		}
		startCol += cellPaddingChars;

		tbl::detail::expand_tabs(expanded_buffer, sizeof(expanded_buffer), column.header.c_str(), screenAlignedTabs ? startCol : 0);

		outString.Appendf("%-*s", column.width, expanded_buffer);

		for (size_t p = 0; p < cellPaddingChars; ++p)
		{
			outString.Append(' ');
		}

		outString.Append('|');

		startCol += column.width + cellPaddingChars + 1;
	});

	outString.Append('\n');

	idLib::Printf(outString);
}


// row line
template<class... ColTs>
void Table<ColTs ...>::PrintRow(const Row& row) const {
	idStr outString('|');
	size_t startCol = 1;
	char raw_buffer[MAX_STRING_CHARS] = { '\0' }, expanded_buffer[MAX_STRING_CHARS * 2] = { '\0' };

	VisitColumns([&]<size_t I>(const auto& column) {
		for (index_t p = 0; std::cmp_less(p, cellPaddingChars); ++p)
		{
			outString.Append(' ');
		}
		startCol += cellPaddingChars;

		FormatCell<I>(row, raw_buffer, sizeof(raw_buffer));
		tbl::detail::expand_tabs(expanded_buffer, sizeof(expanded_buffer), raw_buffer, screenAlignedTabs ? startCol : 0);

		switch (column.alignment) {
		default:
		case ColumnAlignment::Left:
			outString.Appendf("%-*s", column.width, expanded_buffer);
			break;
		case ColumnAlignment::Right:
			outString.Appendf("%*s", column.width, expanded_buffer);
			break;
		case ColumnAlignment::Center: {
			const size_t len = strlen(expanded_buffer);
			const size_t pad = (column.width > len) ? (column.width - len) : 0;
			const size_t L = pad / 2, R = pad - L;
			outString.Appendf("%*s%-*s", (L + len), expanded_buffer, R, "");
		} break;
		}

		for (index_t p = 0; std::cmp_less(p, cellPaddingChars); ++p)
		{
			outString.Append(' ');
		}

		outString.Append('|');

		startCol += column.width + cellPaddingChars + 1;
	});

	outString.Append('\n');

	idLib::Printf(outString);
}

template<class... ColTs>
void Table<ColTs ...>::PrintSummary() const {
	char msg[MAX_STRING_CHARS] = {};
	idStr::snPrintf(msg, "Total Rows: %zu", rows.Num());

	idStr outString('|');

	// pad left/right so it's centered across full width minus borders
	size_t len = strlen(msg);
	if (totalWidth <= len + 2)
	{
		outString.Appendf(" %s |\n", msg);
		return;
	}
	size_t usable = totalWidth - 2; // remove outer borders
	size_t pad = (usable > len) ? (usable - len) : 0;
	size_t left = pad / 2, right = pad - left;
	outString.Appendn(' ', left);
	outString.Append(msg);
	outString.Appendn(' ', right);
	outString.Append("|\n");

	idLib::Printf(outString);
}

// ---- per-cell formatting (printf spec > formatter > default) ----
template<class... ColTs>
template<size_t I>
void Table<ColTs...>::FormatCell(const Row& row, char* dest, size_t capacity) const {
	const auto& column = std::get<I>(columns);
	const auto& value = std::get<I>(row);
	using T = std::decay_t<decltype(value)>;

	if (column.rowFormatter)
	{
		column.rowFormatter(row, dest, capacity);
		return;
	}

	if (column.fmt && *column.fmt) {
		idStr::snPrintf(dest, capacity, column.fmt, value);
		return;
	}

	// default formatter
	idStr::ToCString(value, dest, capacity);
}

// helpers to detect tuple
template<typename T>
static constexpr bool is_cstr_v =
std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, char*>;

template<typename T>
static constexpr bool is_tuple_v = requires { typename std::tuple_size<std::decay_t<T>>::type; };

// overload 1: plain title (const char*)
template<class... ColTs>
void Table<ColTs...>::ProcessColumn(size_t i, const char* title) {
	SetHeader(i, title ? title : "");
	SetAlignment(i, ColumnAlignment::Left);
}

// overload 2: generic tuple
template<class... ColTs>
template<typename Tup>
void Table<ColTs...>::ProcessColumn(size_t i, Tup&& t) {
	static_assert(is_tuple_v<Tup>, "Each descriptor must be a const char* or a tuple");
	ApplyColumnTuple(i, std::forward<Tup>(t));
}

// apply tuple contents by arity & types
// title required: always the first element
template<class... ColTs>
template<typename... Ts>
void Table<ColTs...>::ApplyColumnTuple(size_t i, const std::tuple<Ts...>& t) {
	static_assert(sizeof...(Ts) >= 1, "Tuple column descriptor must have at least a title");
	const char* title = GetColumnHeaderFromTuple(t);
	SetHeader(i, title ? title : "");

	// defaults
	ColumnAlignment a = ColumnAlignment::Left;
	const char* fmt = nullptr;
	RowFormatter rf = nullptr;

	// walk remaining elements (if any)
	SetColumnParametersFromTuple(a, fmt, rf, t, std::make_index_sequence<sizeof...(Ts)>{});

	SetAlignment(i, a);
	if (fmt)
	{
		SetFormat(i, fmt);
	}
	if (rf)
	{
		SetFormatter(i, rf);
	}
}

// first element must be c-string title
template<class... ColTs>
template<typename... Ts>
const char* Table<ColTs...>::GetColumnHeaderFromTuple(const std::tuple<Ts...>& t) {
	using First = std::tuple_element_t<0, std::tuple<Ts...>>;
	static_assert(is_cstr_v<First>, "First item of tuple must be const char* (title)");
	return std::get<0>(t);
}

// scan rest of tuple positions 1..M-1 and fill a/fmt/rf if types match
template<class... ColTs>
template<typename... Ts, size_t... Is>
void Table<ColTs...>::SetColumnParametersFromTuple(ColumnAlignment& a, const char*& fmt, RowFormatter& rf, const std::tuple<Ts...>& t, std::index_sequence<Is...>) {
	(ConsumeColumnTuple(a, fmt, rf, std::integral_constant<size_t, Is>{}, t), ...);
}

template<class... ColTs>
template<size_t I, typename... Ts>
void Table<ColTs...>::ConsumeColumnTuple(ColumnAlignment& a, const char*& fmt, RowFormatter& rf, std::integral_constant<size_t, I>, const std::tuple<Ts...>& t) {
	if constexpr (I == 0) { return; }  // skip title
	else {
		using E = std::tuple_element_t<I, std::tuple<Ts...>>;
		if constexpr (std::is_same_v<E, ColumnAlignment>) {
			a = std::get<I>(t);
		}
		else if constexpr (is_cstr_v<E>) {
			fmt = std::get<I>(t);
		}
		else if constexpr (std::is_same_v<E, RowFormatter>) {
			rf = std::get<I>(t);
		}
		else {
			static_assert(!sizeof(E), "Unsupported item in column tuple: must be ColumnAlignment, const char*, or RowFormatter");
		}
	}
}

#endif // __TABLE_H__