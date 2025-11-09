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

#ifndef __DICT_H__
#define __DICT_H__

#pragma once

class idSerializer;

/*
===============================================================================

Key/value dictionary

This is a dictionary class that tracks an arbitrary number of key / value
pair combinations. It is used for map entity spawning, GUI state management,
and other things.

Keys are compared case-insensitive.

Does not allocate memory until the first key/value pair is added.

===============================================================================
*/

template < Formattable T >
class idDict;

template < Formattable T = idStr >
class idKeyValue {
	friend class idDict<T>;

public:
	[[nodiscard]] const idStr &		GetKey() const { return *key; }
	[[nodiscard]] const T&          GetValue() const { return ParseValueToT(*value_string); }
	[[nodiscard]] const idStr &		GetValueString() const { return *value_string; }

	[[nodiscard]] size_t			Allocated() const { return key->Allocated() + value_string->Allocated(); }
	[[nodiscard]] size_t			Size() const { return sizeof( *this ) + key->Size() + value_string->Size(); }

	bool			            	operator==( const idKeyValue &kv ) const { return ( key == kv.key && value_string == kv.value_string); }

private:
	[[nodiscard]] static T &        ParseValueToT( const idStr& value );

	const idPoolStr *	key;
	const idPoolStr *	value_string;
};

/*
================================================
idSort_KeyValue 
================================================
*/
template < Formattable T = idStr >
class idSort_KeyValue : public idSort_Quick< idKeyValue<T>, idSort_KeyValue<T> > {
public:
	[[nodiscard]] int Compare( const idKeyValue<T> & a, const idKeyValue<T> & b ) const { return a.GetKey().Icmp( b.GetKey() ); }
};

template < Formattable T = idStr >
class idDict {
public:
						idDict() noexcept;
	           explicit idDict( const idDict<T> &other );	// allow declaration with assignment
						~idDict();

						// set the granularity for the index
	void				SetGranularity( size_t granularity );
						// set hash size
	void				SetHashSize( size_t hashSize );
						// clear existing key/value pairs and copy all key/value pairs from other
	idDict<T> &		    operator=( const idDict<T>& other );

	[[nodiscard]] const T& operator[]( const FormattableNoStrings auto& key ) const noexcept;
	[[nodiscard]]       T& operator[]( const FormattableNoStrings auto& key ) noexcept;
	[[nodiscard]] const T& operator[]( const StringLike auto& key ) const noexcept;
	[[nodiscard]]       T& operator[]( const StringLike auto& key ) noexcept;

						// copy from other while leaving existing key/value pairs in place
	void				Copy( const idDict &other );
						// clear existing key/value pairs and transfer key/value pairs from other
	void				TransferKeyValues( idDict &other );
						// parse dict from parser
	bool				Parse( idParser &parser );
						// copy key/value pairs from other dict not present in this dict
	void				SetDefaults( const idDict *dict );
						// clear dict freeing up memory
	void				Clear();
						// print the dict
	void				Print() const;

	[[nodiscard]] size_t				Allocated() const;
	[[nodiscard]] size_t				Size() const { return sizeof( *this ) + Allocated(); }

	void				Set( const StringLikeOrEnum auto &key, const StringLikeOrEnum auto &value );
	void				SetFloat( const StringLikeOrEnum auto &key, float val );
	void				SetDouble( const StringLikeOrEnum auto & key, double val );
	void				SetInt( const StringLikeOrEnum auto &key, int32 val );
	void				SetInt64( const StringLikeOrEnum auto & key, int64 val );
	void				SetBool( const StringLikeOrEnum auto &key, bool val );
	void				SetVector( const StringLikeOrEnum auto &key, const idVec3 &val );
	void				SetVec2( const StringLikeOrEnum auto &key, const idVec2 &val );
	void				SetVec4( const StringLikeOrEnum auto &key, const idVec4 &val );
	void				SetAngles( const StringLikeOrEnum auto &key, const idAngles &val );
	void				SetMatrix( const StringLikeOrEnum auto &key, const idMat3 &val );

						// these return default values of 0.0, 0 and false
	[[nodiscard]] const char *	GetString( const StringLikeOrEnum auto &key, const char* defaultString = "" ) const;
	[[nodiscard]] float			GetFloat( const StringLikeOrEnum auto &key, const char* defaultString ) const;
	[[nodiscard]] double		GetDouble( const StringLikeOrEnum auto & key, const char* defaultString ) const;
	[[nodiscard]] int32			GetInt( const StringLikeOrEnum auto &key, const char* defaultString ) const;
	[[nodiscard]] int64			GetInt64( const StringLikeOrEnum auto & key, const char* defaultString ) const;
	[[nodiscard]] bool			GetBool( const StringLikeOrEnum auto &key, const char* defaultString ) const;
	[[nodiscard]] float			GetFloat( const StringLikeOrEnum auto &key, const float defaultFloat = 0.0f ) const;
	[[nodiscard]] double		GetDouble( const StringLikeOrEnum auto & key, const double defaultDouble = 0.0 ) const;
	[[nodiscard]] int32			GetInt( const StringLikeOrEnum auto &key, const int32 defaultInt = 0 ) const;
	[[nodiscard]] int64			GetInt64(const StringLikeOrEnum auto& key, const int64 defaultInt = 0 ) const;
	[[nodiscard]] bool			GetBool( const StringLikeOrEnum auto &key, const bool defaultBool = false ) const;
	[[nodiscard]] idVec3		GetVector( const StringLikeOrEnum auto &key, const char* defaultString = nullptr ) const;
	[[nodiscard]] idVec2		GetVec2( const StringLikeOrEnum auto &key, const char* defaultString = nullptr ) const;
	[[nodiscard]] idVec4		GetVec4( const StringLikeOrEnum auto &key, const char* defaultString = nullptr ) const;
	[[nodiscard]] idAngles		GetAngles( const StringLikeOrEnum auto &key, const char* defaultString = nullptr ) const;
	[[nodiscard]] idMat3		GetMatrix( const StringLikeOrEnum auto &key, const char* defaultString = nullptr ) const;

	bool				GetString( const StringLikeOrEnum auto &key, const char* defaultString, const char ** out ) const;
	bool				GetString( const StringLikeOrEnum auto &key, const char* defaultString, idStr &out ) const;
	bool				GetFloat( const StringLikeOrEnum auto &key, const char* defaultString, float &out ) const;
	bool				GetDouble(const StringLikeOrEnum auto& key, const char* defaultString, double &out ) const;
	bool				GetInt( const StringLikeOrEnum auto &key, const char* defaultString, int32 &out ) const;
	bool				GetInt64(const StringLikeOrEnum auto& key, const char* defaultString, int64& out ) const;
	bool				GetBool( const StringLikeOrEnum auto &key, const char* defaultString, bool &out ) const;
	bool				GetFloat( const StringLikeOrEnum auto &key, const float defaultFloat, float &out ) const;
	bool				GetDouble(const StringLikeOrEnum auto& key, const double defaultDouble, double &out ) const;
	bool				GetInt( const StringLikeOrEnum auto &key, const int32 defaultInt, int32 &out ) const;
	bool				GetInt64(const StringLikeOrEnum auto& key, const int64 defaultInt, int64& out ) const;
	bool				GetBool( const StringLikeOrEnum auto &key, const bool defaultBool, bool &out ) const;
	bool				GetVector( const StringLikeOrEnum auto &key, const char* defaultString, idVec3 &out ) const;
	bool				GetVec2( const StringLikeOrEnum auto &key, const char* defaultString, idVec2 &out ) const;
	bool				GetVec4( const StringLikeOrEnum auto &key, const char* defaultString, idVec4 &out ) const;
	bool				GetAngles( const StringLikeOrEnum auto &key, const char* defaultString, idAngles &out ) const;
	bool				GetMatrix( const StringLikeOrEnum auto &key, const char* defaultString, idMat3 &out ) const;

	[[nodiscard]] size_t				GetNumKeyVals() const;
	
	const idKeyValue<T> *	GetKeyVal( const Ordinal auto index ) const;
						// returns the key/value pair with the given key
						// returns NULL if the key/value pair does not exist
	const idKeyValue<T> *	FindKey( const Formattable auto &key ) const;
						// returns the index to the key/value pair with the given key
						// returns -1 if the key/value pair does not exist
	index_t				FindKeyIndex( const Formattable auto &key ) const;
						// delete the key/value pair with the given key
	void				Delete( const Formattable auto &key );
						// finds the next key/value pair with the given key prefix.
						// lastMatch can be used to do additional searches past the first match.
	const idKeyValue<T> *	MatchPrefix( const Formattable auto &prefix, const idKeyValue<T> *lastMatch = nullptr) const;
						// randomly chooses one of the key/value pairs with the given key prefix and returns it's value
	const char *		RandomPrefix(const Formattable auto &prefix, idRandom &random ) const;

	void				WriteToFileHandle( idFile *f ) const;
	void				ReadFromFileHandle( idFile *f );

	void				WriteToIniFile( idFile * f ) const;
	bool				ReadFromIniFile( idFile * f );

	void				Serialize( idSerializer & ser );

						// returns a unique checksum for this dictionary's content
	[[nodiscard]] int					Checksum() const;

	static void			Init();
	static void			Shutdown();

	static void			ShowMemoryUsage_f( const idCmdArgs &args );
	static void			ListKeys_f( const idCmdArgs &args );
	static void			ListValues_f( const idCmdArgs &args );
	static void			ListKeyValuePairs_f( const idCmdArgs& args );

	// Modern Range-Based for-loop Iteration
	idKeyValue<T>* begin() noexcept { return args.begin(); }
	idKeyValue<T>* end() noexcept { return args.end(); }

	const idKeyValue<T>* begin() const noexcept { return args.begin(); }
	const idKeyValue<T>* end() const noexcept { return args.end(); }

	const idKeyValue<T>* cbegin() const noexcept { return args.begin(); }
	const idKeyValue<T>* cend()   const noexcept { return args.end(); }

private:
	idList<idKeyValue<T>>	args;
	idHashIndex			argHash;
};

template< Formattable T>
ID_INLINE idDict<T>::idDict() noexcept {
	args.SetGranularity( 16 );
	argHash.SetGranularity( 16 );
	argHash.Clear( 128, 16 );
}

template< Formattable T>
ID_INLINE idDict<T>::idDict( const idDict<T> &other ) {
	*this = other;
}

template< Formattable T>
ID_INLINE idDict<T>::~idDict() {
	Clear();
}

template<>
ID_INLINE void idDict<>::SetGranularity(const size_t granularity ) {
	args.SetGranularity( granularity );
	argHash.SetGranularity( granularity );
}

template<Formattable T>
ID_INLINE void idDict<T>::SetHashSize(const size_t hashSize ) {
	if ( args.Num() == 0 ) {
		argHash.Clear( hashSize, 16 );
	}
}

template<Formattable T>
ID_INLINE void idDict<T>::SetFloat( const StringLikeOrEnum auto &key, const float val ) {
	Set( key, va( "%f", val ) );
}

template<Formattable T>
ID_INLINE void idDict<T>::SetDouble(const StringLikeOrEnum auto & key, const double val) {
	Set(key, va("%lf", val));
}

template<Formattable T>
ID_INLINE void idDict<T>::SetInt( const StringLikeOrEnum auto &key, const int32 val ) {
	Set( key, va( "%i", val ) );
}

template<Formattable T>
ID_INLINE void idDict<T>::SetInt64(const StringLikeOrEnum auto & key, const int64 val) {
	Set(key, va("%lli", val));
}

template<Formattable T>
ID_INLINE void idDict<T>::SetBool( const StringLikeOrEnum auto &key, const bool val ) {
	Set( key, va( "%i", val ) );
}

template<Formattable T>
ID_INLINE void idDict<T>::SetVector( const StringLikeOrEnum auto &key, const idVec3 &val ) {
	Set( key, val.ToString() );
}

template<Formattable T>
ID_INLINE void idDict<T>::SetVec4( const StringLikeOrEnum auto &key, const idVec4 &val ) {
	Set( key, val.ToString() );
}

template<Formattable T>
ID_INLINE void idDict<T>::SetVec2( const StringLikeOrEnum auto &key, const idVec2 &val ) {
	Set( key, val.ToString() );
}

template<Formattable T>
ID_INLINE void idDict<T>::SetAngles( const StringLikeOrEnum auto &key, const idAngles &val ) {
	Set( key, val.ToString() );
}

template<Formattable T>
ID_INLINE void idDict<T>::SetMatrix( const StringLikeOrEnum auto &key, const idMat3 &val ) {
	Set( key, val.ToString() );
}

template< Formattable T>
ID_INLINE bool idDict<T>::GetString( const StringLikeOrEnum auto &key, const char* defaultString, const char **out ) const {
	const idKeyValue<T> *kv = FindKey( key );

	if ( kv ) {
		*out = kv->GetValue();

		return true;
	}

	*out = defaultString;

	return false;
}

template< Formattable T>
ID_INLINE bool idDict<T>::GetString( const StringLikeOrEnum auto& key, const char* defaultString, idStr &out ) const {
	const idKeyValue<T> *kv = FindKey( key );

	if ( kv ) {
		out = kv->GetValue();

		return true;
	}

	out = defaultString;

	return false;
}

template< Formattable T>
ID_INLINE const char *idDict<T>::GetString( const StringLikeOrEnum auto& key, const char* defaultString ) const {
	const idKeyValue<T> *kv = FindKey( key );

	if ( kv ) {
		return kv->GetValueString();
	}

	return defaultString;
}

template< Formattable T>
ID_INLINE float idDict<T>::GetFloat( const StringLikeOrEnum auto &key, const char* defaultString ) const {
	using U = std::remove_cvref_t<T>;
	using V = float;

	const idKeyValue<T>* kv = FindKey(key);

	if (kv)
	{
		if constexpr (std::is_same_v<U, V>) {
			return kv->GetValue();
		}

		return idStr::AtoF<V>(kv->GetValueString());
	}

	return idStr::AtoF<V>(defaultString);
}


template< Formattable T>
ID_INLINE double idDict<T>::GetDouble(const StringLikeOrEnum auto& key, const char* defaultString) const {
	using U = std::remove_cvref_t<T>;
	using V = double;

	const idKeyValue<T>* kv = FindKey(key);

	if (kv)
	{
		if constexpr (std::is_same_v<U, V>) {
			return kv->GetValue();
		}

		return idStr::AtoF<V>(kv->GetValueString());
	}

	return idStr::AtoF<V>(defaultString);
}

template< Formattable T>
ID_INLINE int32 idDict<T>::GetInt( const StringLikeOrEnum auto &key, const char* defaultString ) const {
	using U = std::remove_cvref_t<T>;
	using V = int32;

	const idKeyValue<T>* kv = FindKey(key);

	if (kv)
	{
		if constexpr (std::is_same_v<U, V>) {
			return kv->GetValue();
		}

		return idStr::AtoI<V>(kv->GetValueString());
	}

	return idStr::AtoI<V>(defaultString);
}

template< Formattable T>
ID_INLINE int64 idDict<T>::GetInt64(const StringLikeOrEnum auto& key, const char* defaultString) const {
	using U = std::remove_cvref_t<T>;
	using V = int64;

	const idKeyValue<T>* kv = FindKey(key);

	if (kv)
	{
		if constexpr (std::is_same_v<U, V>) {
			return kv->GetValue();
		}

		return idStr::AtoI<V>(kv->GetValueString());
	}

	return idStr::AtoI<V>(defaultString);
}

template< Formattable T>
ID_INLINE bool idDict<T>::GetBool( const StringLikeOrEnum auto &key, const char* defaultString ) const {
	using U = std::remove_cvref_t<T>;
	using V = bool;

	const idKeyValue<T>* kv = FindKey(key);

	if (kv)
	{
		if constexpr (std::is_same_v<U, V>) {
			return kv->GetValue();
		}

		return idStr::AtoI<int64>(kv->GetValueString()) ? true : false;
	}

	return idStr::AtoI<int64>(defaultString) ? true : false;
}

template< Formattable T>
ID_INLINE float idDict<T>::GetFloat( const StringLikeOrEnum auto &key, const float defaultFloat ) const {
	using U = std::remove_cvref_t<T>;
	using V = float;

	const idKeyValue<T>* kv = FindKey(key);

	if (kv)
	{
		if constexpr (std::is_same_v<U, V>) {
			return kv->GetValue();
		}

		return idStr::AtoF<V>(kv->GetValueString());
	}

	return defaultFloat;
}

template< Formattable T>
ID_INLINE double idDict<T>::GetDouble( const StringLikeOrEnum auto& key, const double defaultDouble ) const {
	using U = std::remove_cvref_t<T>;
	using V = double;

	const idKeyValue<T>* kv = FindKey(key);

	if (kv)
	{
		if constexpr (std::is_same_v<U, V>) {
			return kv->GetValue();
		}

		return idStr::AtoF<V>(kv->GetValueString());
	}

	return defaultDouble;
}

template< Formattable T>
ID_INLINE int32 idDict<T>::GetInt( const StringLikeOrEnum auto &key, const int32 defaultInt ) const {
	using U = std::remove_cvref_t<T>;
	using V = int32;

	const idKeyValue<T>* kv = FindKey(key);

	if (kv)
	{
		if constexpr (std::is_same_v<U, V>) {
			return kv->GetValue();
		}

		return idStr::AtoI<V>(kv->GetValueString());
	}

	return defaultInt;
}

template< Formattable T>
ID_INLINE int64 idDict<T>::GetInt64(const StringLikeOrEnum auto& key, const int64 defaultInt) const {
	using U = std::remove_cvref_t<T>;
	using V = int64;

	const idKeyValue<T>* kv = FindKey(key);

	if (kv)
	{
		if constexpr (std::is_same_v<U, V>) {
			return kv->GetValue();
		}

		return idStr::AtoI<V>(kv->GetValueString());
	}

	return defaultInt;
}

template< Formattable T>
ID_INLINE bool idDict<T>::GetBool( const StringLikeOrEnum auto &key, const bool defaultBool ) const {
	using U = std::remove_cvref_t<T>;
	using V = bool;

	const idKeyValue<T>* kv = FindKey(key);

	if (kv)
	{
		if constexpr (std::is_same_v<U, V>) {
			return kv->GetValue();
		}

		return idStr::AtoI<int64>(kv->GetValueString()) ? true : false;
	}

	return defaultBool;
}

template< Formattable T>
ID_INLINE idVec3 idDict<T>::GetVector( const StringLikeOrEnum auto &key, const char* defaultString ) const {
	idVec3 out = {};
	idDict<T>::GetVector( key, defaultString, out );
	return out;
}

template< Formattable T>
ID_INLINE idVec2 idDict<T>::GetVec2( const StringLikeOrEnum auto &key, const char* defaultString ) const {
	idVec2 out = {};
	idDict<T>::GetVec2( key, defaultString, out );
	return out;
}

template< Formattable T>
ID_INLINE idVec4 idDict<T>::GetVec4( const StringLikeOrEnum auto &key, const char* defaultString ) const {
	idVec4 out = {};
	idDict<T>::GetVec4( key, defaultString, out );
	return out;
}

template< Formattable T>
ID_INLINE idAngles idDict<T>::GetAngles( const StringLikeOrEnum auto &key, const char* defaultString ) const {
	idAngles out = {};
	idDict<T>::GetAngles( key, defaultString, out );
	return out;
}

template< Formattable T>
ID_INLINE idMat3 idDict<T>::GetMatrix( const StringLikeOrEnum auto &key, const char* defaultString ) const {
	idMat3 out = {};
	idDict<T>::GetMatrix( key, defaultString, out );
	return out;
}

template< Formattable T>
ID_INLINE size_t idDict<T>::GetNumKeyVals() const {
	return args.Num();
}

template< Formattable T>
ID_INLINE const idKeyValue<T> *idDict<T>::GetKeyVal( const Ordinal auto index ) const {
	if ( index >= 0 && std::cmp_less(index, args.Num()) ) {
		return &args[ index ];
	}
	return nullptr;
}

#endif /* !__DICT_H__ */
