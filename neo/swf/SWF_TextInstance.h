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
#ifndef __SWF_TEXTINSTANCE_H__
#define __SWF_TEXTINSTANCE_H__

struct subTimingWordData_t {
	subTimingWordData_t() noexcept {
		startTime = 0;
		forceBreak = false;
	}

	idStr phrase;
	ID_TIME_T startTime;
	bool forceBreak;
};

class idSWFTextInstance {
public:
	idSWFTextInstance();
	~idSWFTextInstance();

	void Init( idSWFEditText * editText, idSWF * _swf );

	idSWFScriptObject * GetScriptObject() { return &scriptObject; }

	[[nodiscard]] bool	GetHasDropShadow() const { return useDropShadow; }
	[[nodiscard]] bool	HasStroke() const { return useStroke; }
	[[nodiscard]] float	GetStrokeStrength() const { return strokeStrength; }
	[[nodiscard]] float	GetStrokeWeight() const { return strokeWeight; }

	// used for when text has random render mode set 
	[[nodiscard]] bool	IsGeneratingRandomText() const { return generatingText; }
	void	StartRandomText( ID_TIME_T time );	
	idStr	GetRandomText( ID_TIME_T time );
	void	StartParagraphText( ID_TIME_T time );
	idStr	GetParagraphText( ID_TIME_T time );
	[[nodiscard]] bool	NeedsGenerateRandomText() const { return triggerGenerate; }
	[[nodiscard]] bool	NeedsSoundPlayed() const;
	void	ClearPlaySound() { needsSoundUpdate = false; }
	idStr	GetSoundClip() { return soundClip; }
	void	SetIgnoreColor(const bool ignore ) { ignoreColor = ignore; }

	void	SetStrokeInfo( bool use, float strength = 0.75f, float weight = 1.75f );
	int		CalcMaxScroll( size_t numLines = -1 );
	int		CalcNumLines();

	// subtitle functions
	void	SwitchSubtitleText( ID_TIME_T time );
	bool	UpdateSubtitle( ID_TIME_T time );
	[[nodiscard]] bool	IsSubtitle() const { return isSubtitle; }
	[[nodiscard]] bool	IsUpdatingSubtitle() const { return subUpdating; }
	void	SetSubEndIndex( int endChar, ID_TIME_T time );
	[[nodiscard]] int		GetLastWordIndex() const { return subLastWordIndex; }
	[[nodiscard]] int		GetPrevLastWordIndex() const { return subPrevLastWordIndex; }
	void	LastWordChanged( int wordCount, ID_TIME_T time );
	void	SetSubStartIndex(const int value ) { subCharStartIndex = value; }
	[[nodiscard]] int		GetSubEndIndex() const { return subCharEndIndex; }
	[[nodiscard]] int		GetSubStartIndex() const { return subCharStartIndex; }
	void	SetSubNextStartIndex( int value );
	int		GetApporoximateSubtitleBreak( ID_TIME_T time );
	[[nodiscard]] bool	SubNeedsSwitch() const { return subNeedsSwitch; }
	[[nodiscard]] idStr	GetPreviousText() const { return subtitleText.c_str(); }
	void	SubtitleComplete();
	[[nodiscard]] int		GetSubAlignment() const { return subAlign; }
	[[nodiscard]] idStr	GetSpeaker() const { return subSpeaker.c_str(); }
	void	SubtitleCleanup();
	float	GetTextLength();
	[[nodiscard]] int		GetInputStartChar( ) const { return inputTextStartChar; }
	void	SetInputStartCharacter(const int c ) { inputTextStartChar = c; }

	[[nodiscard]] const idSWFEditText * GetEditText() const { return editText; }
	void	SetText( idStr val ) { text = val; lengthCalculated = false; }

	// Removing the private access control statement due to cl 214702
	// Apparently MS's C++ compiler supports the newer C++ standard, and GCC supports C++03
	// In the new C++ standard, nested members of a friend class have access to private/protected members of the class granting friendship
	// In C++03, nested members defined in a friend class do NOT have access to private/protected members of the class granting friendship

	idSWFEditText * editText;
	idSWF *	swf;

	// this text instance's script object
	idSWFScriptObject  scriptObject;

	idStr text;
	idStr randomtext;
	idStr variable;
	swfColorRGBA_t color;

	bool visible;
	bool tooltip;

	int selectionStart;
	int selectionEnd;
	bool ignoreColor;

	int scroll;
	int scrollTime;
	size_t maxscroll;
	size_t maxLines;
	float glyphScale;
	swfRect_t bounds;
	float linespacing;

	bool shiftHeld;
	int lastInputTime;

	bool useDropShadow;
	bool useStroke;

	float strokeStrength;
	float strokeWeight;

	int		textLength;
	bool	lengthCalculated;

	swfTextRenderMode_t renderMode;
	bool		generatingText;
	int			rndSpotsVisible;
	int			rndSpacesVisible;
	int			charMultiplier;
	int			textSpotsVisible;
	int			rndTime;
	int			startRndTime;
	int			prevReplaceIndex;
	bool		triggerGenerate;
	int			renderDelay;
	bool		scrollUpdate;
	idStr		soundClip;
	bool		needsSoundUpdate;
	idList<int, TAG_SWF>	indexArray;
	idRandom2	rnd;

	// used for subtitles
	bool		isSubtitle;
	int			subLength;
	int			subCharDisplayTime;
	int			subAlign;
	bool		subUpdating;
	int			subCharStartIndex;
	int			subNextStartIndex;
	int			subCharEndIndex;
	int			subDisplayTime;
	int			subStartTime;
	int			subSourceID;
	idStr		subtitleText;
	bool		subNeedsSwitch;
	bool		subForceKillQueued;
	bool		subForceKill;
	int			subKillTimeDelay;
	int			subSwitchTime;
	int			subLastWordIndex;
	int			subPrevLastWordIndex;
	idStr		subSpeaker;
	bool		subWaitClear;
	bool		subInitialLine;

	// input text
	int			inputTextStartChar;
	
	idList< subTimingWordData_t, TAG_SWF > subtitleTimingInfo;
};

/*
================================================
This is the prototype object that all the text instance script objects reference
================================================
*/
class idSWFScriptObject_TextInstancePrototype : public idSWFScriptObject {
public:
	idSWFScriptObject_TextInstancePrototype();

	//----------------------------------
	// Native Script Functions
	//----------------------------------
#define SWF_TEXT_FUNCTION_DECLARE( x ) \
	class idSWFScriptFunction_##x : public idSWFScriptFunction_RefCounted { \
	public: \
		void			AddRef() {} \
		void			Release() {} \
		idSWFScriptVar Call( idSWFScriptObject * thisObject, const idSWFParmList & parms ); \
	} scriptFunction_##x;

	SWF_TEXT_FUNCTION_DECLARE( onKey );
	SWF_TEXT_FUNCTION_DECLARE( onChar );
	SWF_TEXT_FUNCTION_DECLARE( generateRnd );
	SWF_TEXT_FUNCTION_DECLARE( calcNumLines );

	SWF_NATIVE_VAR_DECLARE( text );
	SWF_NATIVE_VAR_DECLARE( autoSize );
	SWF_NATIVE_VAR_DECLARE( dropShadow );
	SWF_NATIVE_VAR_DECLARE( _stroke );
	SWF_NATIVE_VAR_DECLARE( _strokeStrength );
	SWF_NATIVE_VAR_DECLARE( _strokeWeight );
	SWF_NATIVE_VAR_DECLARE( variable );
	SWF_NATIVE_VAR_DECLARE( _alpha );
	SWF_NATIVE_VAR_DECLARE( textColor );
	SWF_NATIVE_VAR_DECLARE( _visible );
	SWF_NATIVE_VAR_DECLARE( scroll );
	SWF_NATIVE_VAR_DECLARE( maxscroll );
	SWF_NATIVE_VAR_DECLARE( selectionStart );
	SWF_NATIVE_VAR_DECLARE( selectionEnd );
	SWF_NATIVE_VAR_DECLARE( isTooltip );
	SWF_NATIVE_VAR_DECLARE( mode );
	SWF_NATIVE_VAR_DECLARE( delay );
	SWF_NATIVE_VAR_DECLARE( renderSound );
	SWF_NATIVE_VAR_DECLARE( updateScroll );
	SWF_NATIVE_VAR_DECLARE( subtitle );
	SWF_NATIVE_VAR_DECLARE( subtitleAlign );
	SWF_NATIVE_VAR_DECLARE( subtitleSourceID );
	SWF_NATIVE_VAR_DECLARE( subtitleSpeaker );
	
	SWF_NATIVE_VAR_DECLARE_READONLY( _textLength );

	SWF_TEXT_FUNCTION_DECLARE( subtitleSourceCheck );
	SWF_TEXT_FUNCTION_DECLARE( subtitleStart );
	SWF_TEXT_FUNCTION_DECLARE( subtitleLength );
	SWF_TEXT_FUNCTION_DECLARE( killSubtitle );
	SWF_TEXT_FUNCTION_DECLARE( forceKillSubtitle );
	SWF_TEXT_FUNCTION_DECLARE( subLastLine );
	SWF_TEXT_FUNCTION_DECLARE( addSubtitleInfo );
	SWF_TEXT_FUNCTION_DECLARE( terminateSubtitle );
	SWF_TEXT_FUNCTION_DECLARE( clearTimingInfo );
};

#endif // !__SWF_TEXTINSTANCE_H__
