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
#ifndef __MENUSCREEN_H__
#define __MENUSCREEN_H__

#pragma once

#include "../../renderer/tr_local.h"

typedef enum mainMenuTransition_e : int8 {
	MENU_TRANSITION_INVALID = -1,
	MENU_TRANSITION_SIMPLE,
	MENU_TRANSITION_ADVANCE,
	MENU_TRANSITION_BACK,
	MENU_TRANSITION_FORCE
} mainMenuTransition_t;

typedef enum cursorState_t : uint8 {
	CURSOR_NONE,
	CURSOR_IN_COMBAT,
	CURSOR_TALK,
	CURSOR_GRABBER,
	CURSOR_ITEM,
} cursorState_t;

/*
================================================
idLBRowBlock 
================================================
*/
class idLBRowBlock { 
public:
	idLBRowBlock() : lastTime( 0 ), startIndex( 0 ) {}

	int										lastTime;
	int										startIndex;
	idList< idLeaderboardCallback::row_t >	rows;
};

enum leaderboardFilterMode_t {
	LEADERBOARD_FILTER_OVERALL	= 0,
	LEADERBOARD_FILTER_MYSCORE	= 1,
	LEADERBOARD_FILTER_FRIENDS	= 2
};

/*
================================================
idLBCache 
================================================
*/
class idLBCache { 
public:
	static constexpr size_t NUM_ROW_BLOCKS		= 5;
	static constexpr leaderboardFilterMode_t DEFAULT_LEADERBOARD_FILTER = LEADERBOARD_FILTER_OVERALL;

	idLBCache() :
		def(nullptr),
		filter( DEFAULT_LEADERBOARD_FILTER ),
		pendingDef(nullptr),
		pendingFilter( DEFAULT_LEADERBOARD_FILTER ),
		requestingRows( false ),
		loadingNewLeaderboard( false ),
		numRowsInLeaderboard( 0 ), 
		entryIndex( 0 ),
		rowOffset( 0 ),
		localIndex( -1 ),
		errorCode( LEADERBOARD_DISPLAY_ERROR_NONE ) {}

	void									Pump();
	void									Reset();
	void									SetLeaderboard( const leaderboardDefinition_t *	def_, leaderboardFilterMode_t filter_ = DEFAULT_LEADERBOARD_FILTER );
	void									CycleFilter();
	leaderboardFilterMode_t					GetFilter() const { return filter; }
	idStr									GetFilterStrType();
	bool									Scroll( int amount );
	bool									ScrollOffset( int amount );
	idLBRowBlock *							FindFreeRowBlock();
	void									Update( const idLeaderboardCallback * callback );
	const idLeaderboardCallback::row_t *	GetLeaderboardRow( int row );

	const leaderboardDefinition_t *			GetLeaderboard() const { return def; }
	size_t									GetNumRowsInLeaderboard() const { return numRowsInLeaderboard; }

	int										GetEntryIndex() const { return entryIndex; }
	int										GetRowOffset() const { return rowOffset; }
	int										GetLocalIndex() const { return localIndex; }
	leaderboardDisplayError_t				GetErrorCode() const { return errorCode; }

	bool									IsRequestingRows() const { return requestingRows; }
	bool									IsLoadingNewLeaderboard() const { return loadingNewLeaderboard; }

	void									SetEntryIndex(const int value ) { entryIndex = value; }
	void									SetRowOffset(const int value ) { rowOffset = value; }

	void									DisplayGamerCardUI( const idLeaderboardCallback::row_t * row );

private:
	leaderboardDisplayError_t				CallbackErrorToDisplayError( leaderboardError_t errorCode );

	idLBRowBlock					rowBlocks[NUM_ROW_BLOCKS];

	const leaderboardDefinition_t *	def;
	leaderboardFilterMode_t			filter;

	// Pending def and filter are simply used to queue up SetLeaderboard calls when the system is currently
	// busy waiting on results from a previous SetLeaderboard/GetLeaderboardRow call.
	// This is so we only have ONE request in-flight at any given time.
	const leaderboardDefinition_t *	pendingDef;
	leaderboardFilterMode_t			pendingFilter;

	bool							requestingRows;				// True while requested rows are in flight
	bool							loadingNewLeaderboard;		// True when changing to a new leaderboard (or filter type)

	int								numRowsInLeaderboard;		// Total rows in this leaderboard (they won't all be cached though)
	int								entryIndex;					// Relative row offset (from top of viewing window)
	int								rowOffset;					// Absolute row offset
	int								localIndex;					// Row for the master local user (-1 means not on leaderboard)

	leaderboardDisplayError_t		errorCode;					// Error state of the leaderboard
};

/*
================================================
idMenuScreen
================================================
*/
class idMenuScreen : public idMenuWidget {
public:

	idMenuScreen();
	~idMenuScreen() override;

	void				Update() override;
	virtual void				UpdateCmds();
	virtual void				HandleMenu( const mainMenuTransition_t type );

	virtual void				ShowScreen( const mainMenuTransition_t transitionType );
	virtual void				HideScreen( const mainMenuTransition_t transitionType );

	void				ObserveEvent( const idMenuWidget & widget, const idWidgetEvent & event ) override;
	virtual void				SetScreenGui( idSWF * gui ) { menuGUI = gui; }
	
protected:

	idSWF *	menuGUI;
	mainMenuTransition_t	transition;

};

/*
================================================
idMenuScreen_PDA_UserData
================================================
*/
class idMenuScreen_PDA_UserData : public idMenuScreen {
public:

	idMenuScreen_PDA_UserData() = default;

	~idMenuScreen_PDA_UserData() override = default;
	void					Initialize( idMenuHandler * data ) override;
	void					Update() override;
	void					ShowScreen( const mainMenuTransition_t transitionType ) override;
	void					HideScreen( const mainMenuTransition_t transitionType ) override;
	bool					HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;
	idMenuWidget_PDA_UserData *		GetUserData() { return &pdaUserData; }
	idMenuWidget_PDA_Objective *	GetObjective() { return &pdaObjectiveSimple; }
	idMenuWidget_PDA_AudioFiles *	GetAudioFiles() { return &pdaAudioFiles; }

private: 
	idMenuWidget_PDA_UserData 			pdaUserData;
	idMenuWidget_PDA_Objective 		pdaObjectiveSimple;
	idMenuWidget_PDA_AudioFiles 		pdaAudioFiles;
};

/*
================================================
idMenuScreen_PDA_UserEmails
================================================
*/
class idMenuScreen_PDA_UserEmails : public idMenuScreen {
public:
	idMenuScreen_PDA_UserEmails() : 
		readingEmails( false ),
		scrollEmailInfo( false ) {
	}

	~idMenuScreen_PDA_UserEmails() override
	= default;

	void					Update() override;
	void					Initialize( idMenuHandler * data ) override;
	void					ShowScreen( const mainMenuTransition_t transitionType ) override;
	void					HideScreen( const mainMenuTransition_t transitionType ) override;
	bool					HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;
	void					ObserveEvent( const idMenuWidget & widget, const idWidgetEvent & event ) override;
	idMenuWidget_PDA_EmailInbox &	GetInbox() { return pdaInbox; }
	
	bool							ScrollCorrectList( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget );
	void							ShowEmail( bool show );
	void							UpdateEmail();
private: 
	idMenuWidget_PDA_EmailInbox 	pdaInbox;
	idMenuWidget_InfoBox 			emailInfo;
	idMenuWidget_ScrollBar 			emailScrollbar;
	bool							readingEmails;
	bool							scrollEmailInfo;
};

/*
================================================
idMenuScreen_PDA_UserData
================================================
*/
class idMenuScreen_PDA_VideoDisks : public idMenuScreen {
public:
	idMenuScreen_PDA_VideoDisks() :
		activeVideo(nullptr) {
	}

	~idMenuScreen_PDA_VideoDisks() override
	= default;

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;

	void						ToggleVideoDiskPlay();
	void						UpdateVideoDetails();
	void						SelectedVideoToPlay( index_t index );
	void						ClearActiveVideo() { activeVideo = nullptr; }
	const idDeclVideo *			GetActiveVideo() { return activeVideo; }
private:
	idMenuWidget_ScrollBar		scrollbar;
	idMenuWidget_DynamicList 		pdaVideoList;
	idMenuWidget_PDA_VideoInfo 	videoDetails;
	idList< idList< idStr, TAG_IDLIB_LIST_MENU>, TAG_IDLIB_LIST_MENU >		videoItems;
	const idDeclVideo *				activeVideo;
};

/*
================================================
idMenuScreen_PDA_UserData
================================================
*/
class idMenuScreen_PDA_Inventory : public idMenuScreen {
public:
	idMenuScreen_PDA_Inventory() = default;

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;

	void						EquipWeapon();
	const char *				GetWeaponName( index_t index );
	bool						IsVisibleWeapon( index_t index );

private:
	idMenuWidget_Carousel 			itemList;
	idMenuWidget_InfoBox 			infoBox;
};

//*
//================================================	
//idMenuScreen_Shell_Root
//================================================
//*/
class idMenuScreen_Shell_Root : public idMenuScreen {
public:
	idMenuScreen_Shell_Root() : 
		options(nullptr),
		helpWidget(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;

	void						HandleExitGameBtn();
	int							GetRootIndex();
	void						SetRootIndex( index_t index );
	idMenuWidget_Help *			GetHelpWidget() { return helpWidget; }

private:
	idMenuWidget_DynamicList *	options;
	idMenuWidget_Help *			helpWidget;
};

//*
//================================================	
//idMenuScreen_Shell_Pause
//================================================
//*/
class idMenuScreen_Shell_Pause : public idMenuScreen {
public:
	idMenuScreen_Shell_Pause() : 
		options(nullptr),
		isMpPause( false ) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;

	void						HandleExitGameBtn();
	void						HandleRestartBtn();

private:
	idMenuWidget_DynamicList *	options;
	bool						isMpPause;
};

//*
//================================================
//idMenuScreen_Shell_PressStart
//================================================
//*/
class idMenuScreen_Shell_PressStart : public idMenuScreen {
public:
	idMenuScreen_Shell_PressStart() : 
		startButton(nullptr),
		options(nullptr),
		itemList(nullptr),
		doomCover(nullptr),
		doom2Cover(nullptr),
		doom3Cover(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;
private: 
	idMenuWidget_Button *		startButton;
	idMenuWidget_DynamicList *	options;
	idMenuWidget_Carousel *		itemList;
	const idMaterial *			doomCover;
	const idMaterial *			doom2Cover;
	const idMaterial *			doom3Cover;
};

//*
//================================================
//idMenuScreen_Shell_PressStart
//================================================
//*/
class idMenuScreen_Shell_GameSelect : public idMenuScreen {
public:
	idMenuScreen_Shell_GameSelect() : 
		startButton(nullptr),
		options(nullptr),
		itemList(nullptr),
		doomCover(nullptr),
		doom2Cover(nullptr),
		doom3Cover(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;
private: 
	idMenuWidget_Button *		startButton;
	idMenuWidget_DynamicList *	options;
	idMenuWidget_Carousel *		itemList;
	const idMaterial *			doomCover;
	const idMaterial *			doom2Cover;
	const idMaterial *			doom3Cover;
};

//*
//================================================	
//idMenuScreen_Shell_Singleplayer
//================================================
//*/
class idMenuScreen_Shell_Singleplayer : public idMenuScreen {
public:
	idMenuScreen_Shell_Singleplayer() : 
		options(nullptr),
		btnBack(nullptr),
		canContinue( false ) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;

	void						SetCanContinue(const bool valid ) { canContinue = valid; }
	void						ContinueGame();
private:
	bool						canContinue;
	idMenuWidget_DynamicList *	options;
	idMenuWidget_Button	*		btnBack;
};

//*
//================================================	
//idMenuScreen_Shell_Settings
//================================================
//*/
class idMenuScreen_Shell_Settings : public idMenuScreen {
public:
	idMenuScreen_Shell_Settings() : 
		options(nullptr),
		btnBack(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;
private:
	idMenuWidget_DynamicList *	options;
	idMenuWidget_Button	*		btnBack;
};

typedef struct creditInfo_s {

	creditInfo_s() {
		type = -1;
		entry = "";
	}

	creditInfo_s(const int t, const char * val ) {
		type = t;
		entry = val;
	}

	int type;
	idStr entry;
} creditInfo_t;

//*
//================================================	
//idMenuScreen_Shell_Credits
//================================================
//*/
class idMenuScreen_Shell_Credits : public idMenuScreen {
public:
	idMenuScreen_Shell_Credits() : 
		btnBack(nullptr),
		creditIndex( 0 ) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;

	void						SetupCreditList();
	void						UpdateCredits();

private:
	idMenuWidget_Button	*		btnBack;
	idList< creditInfo_t >		creditList;
	int							creditIndex;
};

//*
//================================================	
//idMenuScreen_Shell_Resolution
//================================================
//*/
class idMenuScreen_Shell_Resolution : public idMenuScreen {
public:
	idMenuScreen_Shell_Resolution() :
		options(nullptr),
		btnBack(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;

private:
	typedef struct optionData_s {
		optionData_s() {
			fullscreen = -1;
			vidmode = -1;
		}
		optionData_s(const int f, const int v ) {
			fullscreen = f;
			vidmode = v;
		}
		optionData_s( const optionData_s & other ) {
			fullscreen = other.fullscreen;
			vidmode = other.vidmode;
		}
		const optionData_s & operator=( const optionData_s & other ) {
			fullscreen = other.fullscreen;
			vidmode = other.vidmode;
			return *this;
		}
		bool operator==( const optionData_s & other ) const {
			return ( fullscreen == other.fullscreen ) && ( ( vidmode == other.vidmode ) || ( fullscreen == 0 ) );
		}
		int fullscreen;
		int vidmode;
	} optionData_t;
	idList<optionData_t>		optionData;

	optionData_t				originalOption;

	idMenuWidget_DynamicList *	options;
	idMenuWidget_Button	*		btnBack;
};

//*
//================================================	
//idMenuScreen_Shell_Difficulty
//================================================
//*/
class idMenuScreen_Shell_Difficulty : public idMenuScreen {
public:
	idMenuScreen_Shell_Difficulty() : 
		options(nullptr),
		btnBack(nullptr),
		nightmareUnlocked( false ) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;
private:
	bool						nightmareUnlocked;
	idMenuWidget_DynamicList *	options;
	idMenuWidget_Button	*		btnBack;
};

//*
//================================================	
//idMenuScreen_Shell_Playstation
//================================================
//*/
class idMenuScreen_Shell_Playstation : public idMenuScreen {
public:
	idMenuScreen_Shell_Playstation() : 
		options(nullptr),
		btnBack(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;
private:
	idMenuWidget_DynamicList *	options;
	idMenuWidget_Button	*		btnBack;
};

//*
//================================================	
//idMenuScreen_Shell_ModeSelect
//================================================
//*/
class idMenuScreen_Shell_ModeSelect : public idMenuScreen {
public:
	idMenuScreen_Shell_ModeSelect() : 
		options(nullptr),
		btnBack(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;
private:
	idMenuWidget_DynamicList *	options;
	idMenuWidget_Button	*		btnBack;
};

//*
//================================================	
//idMenuScreen_GameBrowser
//================================================
//*/
class idMenuScreen_Shell_GameBrowser : public idMenuScreen {
public:
	idMenuScreen_Shell_GameBrowser() :
		listWidget(nullptr),
		btnBack(nullptr) {
		}

	void				Initialize( idMenuHandler * data ) override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandle = false ) override;
	
	void						UpdateServerList();
	void						OnServerListReady();
	void						DescribeServer( const serverInfo_t & server, const index_t index );

private:
	idMenuWidget_GameBrowserList * listWidget;
	idMenuWidget_Button	*			btnBack;
};

//*
//================================================	
//idMenuScreen_Shell_Leaderboards
//================================================
//*/
class idMenuScreen_Shell_Leaderboards : public idMenuScreen {
public:
	idMenuScreen_Shell_Leaderboards() : 
		options(nullptr),
		btnBack(nullptr),
		refreshLeaderboard( false ),
		refreshWhenMasterIsOnline( false ),
		lbIndex( 0 ),
		btnPrev(nullptr),
		btnNext(nullptr),
		lbHeading(nullptr),
		btnPageDwn(nullptr),
		btnPageUp(nullptr) {
	}

	~idMenuScreen_Shell_Leaderboards() override;

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;

	void						UpdateLeaderboard( const idLeaderboardCallback * callback );
	void						PumpLBCache();
	void						RefreshLeaderboard();
	void						ShowMessage( bool show, idStr message, bool spinner );
	void						ClearLeaderboard();
	void 						SetLeaderboardIndex();

protected:

	struct	doomLeaderboard_t {
		doomLeaderboard_t() : lb(nullptr) { }
		doomLeaderboard_t( const leaderboardDefinition_t * _lb, idStr _name ) { lb=_lb; name=_name; }
		const leaderboardDefinition_t *	lb;
		idStr					name;
	};

	idList< doomLeaderboard_t >	leaderboards;

	idMenuWidget_DynamicList *	options;
	idMenuWidget_Button	*		btnBack;
	idMenuWidget_Button	*		btnPrev;
	idMenuWidget_Button	*		btnNext;
	idMenuWidget_Button	*		btnPageDwn;
	idMenuWidget_Button	*		btnPageUp;	
	idLBCache *					lbCache;
	idSWFTextInstance *			lbHeading;
	int							lbIndex;
	bool						refreshLeaderboard;
	bool						refreshWhenMasterIsOnline;
};

//*
//================================================	
//idMenuScreen_Shell_Bindings
//================================================
//*/
class idMenuScreen_Shell_Bindings : public idMenuScreen {
public:
	idMenuScreen_Shell_Bindings() : 
		options(nullptr),
		btnBack(nullptr),
		blinder(nullptr),
		restoreDefault(nullptr),
		txtBlinder(nullptr),
		bindingsChanged( false ) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;

	void						SetBinding( int keyNum );
	void						UpdateBindingDisplay();
	void						ToggleWait( bool wait );
	void						SetBindingChanged(const bool changed ) { bindingsChanged = changed; }

protected:
	void						HandleRestoreDefaults();

	idMenuWidget_DynamicList *	options;
	idMenuWidget_Button *		restoreDefault;
	idSWFSpriteInstance *		blinder;
	idSWFSpriteInstance *		txtBlinder;
	idMenuWidget_Button	*		btnBack;
	bool						bindingsChanged;
};

//*
//================================================	
//idMenuScreen_Shell_Dev
//================================================
//*/
class idMenuScreen_Shell_Dev : public idMenuScreen {
public:

	struct devOption_t {
		devOption_t() {
			map = "";
			name = "";
		};

		devOption_t( const char * m, const char * n ) {
			map = m;
			name = n;
		}

		const char *	map;
		const char *	name;
	};

	idMenuScreen_Shell_Dev() : 
		options(nullptr),
		btnBack(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;

	void						SetupDevOptions();

private:
	idMenuWidget_DynamicList *	options;
	idMenuWidget_Button	*		btnBack;
	idList< devOption_t, TAG_IDLIB_LIST_MENU >		devOptions;
};

//*
//================================================	
//idMenuScreen_Shell_NewGame
//================================================
//*/
class idMenuScreen_Shell_NewGame : public idMenuScreen {
public:
	idMenuScreen_Shell_NewGame() : 
		options(nullptr),
		btnBack(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;
private:
	idMenuWidget_DynamicList *	options;
	idMenuWidget_Button	*		btnBack;
};

//*
//================================================	
//idMenuScreen_Shell_Load
//================================================
//*/
class idMenuScreen_Shell_Load : public idMenuScreen {
public:
	idMenuScreen_Shell_Load() : 
		options(nullptr),
		btnBack(nullptr),
		btnDelete(nullptr),
		saveInfo(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;
	
	void						UpdateSaveEnumerations();
	void						LoadDamagedGame( index_t index );
	void						LoadGame( index_t index );
	void						DeleteGame( index_t index );
	saveGameDetailsList_t		GetSortedSaves() const { return sortedSaves; }

private:
	idMenuWidget_DynamicList *	options;
	idMenuWidget_Shell_SaveInfo * saveInfo;
	idMenuWidget_Button	*		btnBack;
	idMenuWidget_Button *		btnDelete;
	saveGameDetailsList_t		sortedSaves;
};

//*
//================================================	
//idMenuScreen_Shell_Save
//================================================
//*/
class idMenuScreen_Shell_Save : public idMenuScreen {
public:
	idMenuScreen_Shell_Save() : 
		options(nullptr),
		btnBack(nullptr),
		btnDelete(nullptr),
		saveInfo(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;
	saveGameDetailsList_t		GetSortedSaves() const { return sortedSaves; }
	
	void						UpdateSaveEnumerations();
	void						SaveGame( index_t index );
	void						DeleteGame( index_t index );

private:
	idMenuWidget_Button	*		btnBack;
	idMenuWidget_DynamicList *	options;
	idMenuWidget_Shell_SaveInfo * saveInfo;
	idMenuWidget_Button *		btnDelete;
	saveGameDetailsList_t		sortedSaves;
};

//*
//================================================	
//idMenuScreen_Shell_GameOptions
//================================================
//*/
class idMenuScreen_Shell_GameOptions : public idMenuScreen {
public:

	/*
	================================================
	idMenuDataSource_GameSettings
	================================================
	*/
	class idMenuDataSource_GameSettings : public idMenuDataSource {
	public:
		typedef enum gameSettingFields_e : uint8 {
			GAME_FIELD_FOV,
			GAME_FIELD_CHECKPOINTS,
			GAME_FIELD_AUTO_SWITCH,
			GAME_FIELD_AUTO_RELOAD,
			GAME_FIELD_AIM_ASSIST,
			GAME_FIELD_ALWAYS_SPRINT,
			GAME_FIELD_FLASHLIGHT_SHADOWS,
			MAX_GAME_FIELDS
		} gameSettingFields_t;

		idMenuDataSource_GameSettings();

		// loads data
		void				LoadData() override;

		// submits data
		void				CommitData() override;

		// says whether something changed with the data
		bool				IsDataChanged() const override;

		// retrieves a particular field for reading or updating
		idSWFScriptVar		GetField( const int fieldIndex ) const override { return fields[ fieldIndex ]; }

		void				AdjustField( const int fieldIndex, const int adjustAmount ) override;

	private:
		idStaticList< idSWFScriptVar, MAX_GAME_FIELDS >	fields;
		idStaticList< idSWFScriptVar, MAX_GAME_FIELDS >	originalFields;
	};

	idMenuScreen_Shell_GameOptions() : 
		options(nullptr),
		btnBack(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;
private:
	idMenuWidget_DynamicList *	options;
	idMenuDataSource_GameSettings	systemData;
	idMenuWidget_Button	*		btnBack;
};

//*
//================================================	
//idMenuScreen_Shell_GameOptions
//================================================
//*/
class idMenuScreen_Shell_MatchSettings : public idMenuScreen {
public:

	/*
	================================================
	idMenuDataSource_MatchSettings
	================================================
	*/
	class idMenuDataSource_MatchSettings : public idMenuDataSource {
	public:
		typedef enum matchSettingFields_e : uint8 {
			MATCH_FIELD_MODE,
			MATCH_FIELD_MAP,
			MATCH_FIELD_TIME,
			MATCH_FIELD_SCORE,
			MAX_MATCH_FIELDS
		} matchSettingFields_t;

		idMenuDataSource_MatchSettings();

		// loads data
		void				LoadData() override;

		// submits data
		void				CommitData() override;

		// says whether something changed with the data
		bool				IsDataChanged() const override;
		
		// retrieves a particular field for reading or updating
		idSWFScriptVar		GetField( const int fieldIndex ) const override { return fields[ fieldIndex ]; }

		void				AdjustField( const int fieldIndex, const int adjustAmount ) override;

		bool						MapChanged() { return updateMap; }
		void						ClearMapChanged() { updateMap = false; } 

	private:

		void						GetModeName( index_t index, idStr & name );
		void						GetMapName( index_t index, idStr & name );

		idStaticList< idSWFScriptVar, MAX_MATCH_FIELDS >	fields;
		idStaticList< idSWFScriptVar, MAX_MATCH_FIELDS >	originalFields;
		bool						updateMap;
	};

	idMenuScreen_Shell_MatchSettings() : 
		options(nullptr),
		btnBack(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;
private:
	idMenuWidget_DynamicList *		options;
	idMenuDataSource_MatchSettings	matchData;
	idMenuWidget_Button	*			btnBack;
};

//*
//================================================	
//idMenuScreen_Shell_Controls
//================================================
//*/
class idMenuScreen_Shell_Controls : public idMenuScreen {
public:

	/*
	================================================
	idMenuDataSource_ControlSettings
	================================================
	*/
	class idMenuDataSource_ControlSettings : public idMenuDataSource {
	public:
		typedef enum controlSettingFields_e : uint8 {
			CONTROLS_FIELD_INVERT_MOUSE,
			CONTROLS_FIELD_GAMEPAD_ENABLED,
			CONTROLS_FIELD_MOUSE_SENS,			
			MAX_CONTROL_FIELDS
		} controlSettingFields_t;

		idMenuDataSource_ControlSettings();

		// loads data
		void				LoadData() override;

		// submits data
		void				CommitData() override;

		// says whether something changed with the data
		bool				IsDataChanged() const override;

		// retrieves a particular field for reading or updating
		idSWFScriptVar		GetField( const int fieldIndex ) const override { return fields[ fieldIndex ]; }

		void				AdjustField( const int fieldIndex, const int adjustAmount ) override;

	private:
		idStaticList< idSWFScriptVar, MAX_CONTROL_FIELDS >	fields;
		idStaticList< idSWFScriptVar, MAX_CONTROL_FIELDS >	originalFields;
	};

	idMenuScreen_Shell_Controls() : 
		options(nullptr),
		btnBack(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;
private:
	idMenuWidget_DynamicList *			options;
	idMenuDataSource_ControlSettings	controlData;
	idMenuWidget_Button	*				btnBack;
};

//*
//================================================	
//idMenuScreen_Shell_Controls
//================================================
//*/
class idMenuScreen_Shell_Gamepad : public idMenuScreen {
public:

	/*
	================================================
	idMenuDataSource_ControlSettings
	================================================
	*/
	class idMenuDataSource_GamepadSettings : public idMenuDataSource {
	public:
		typedef enum controlSettingFields_e : uint8 {
			GAMEPAD_FIELD_LEFTY,
			GAMEPAD_FIELD_INVERT,
			GAMEPAD_FIELD_VIBRATE,
			GAMEPAD_FIELD_HOR_SENS,			
			GAMEPAD_FIELD_VERT_SENS,
			GAMEPAD_FIELD_ACCELERATION,
			GAMEPAD_FIELD_THRESHOLD,
			MAX_GAMEPAD_FIELDS
		};

		idMenuDataSource_GamepadSettings();

		// loads data
		void				LoadData() override;

		// submits data
		void				CommitData() override;

		// says whether something changed with the data
		bool				IsDataChanged() const override;

		// retrieves a particular field for reading or updating
		idSWFScriptVar		GetField( const int fieldIndex ) const override { return fields[ fieldIndex ]; }

		void				AdjustField( const int fieldIndex, const int adjustAmount ) override;

	private:
		idStaticList< idSWFScriptVar, MAX_GAMEPAD_FIELDS >	fields;
		idStaticList< idSWFScriptVar, MAX_GAMEPAD_FIELDS >	originalFields;
	};

	idMenuScreen_Shell_Gamepad() : 
		options(nullptr),
		btnBack(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;
private:
	idMenuWidget_DynamicList *			options;
	idMenuDataSource_GamepadSettings	gamepadData;
	idMenuWidget_Button	*				btnBack;
};

//*
//================================================	
//idMenuScreen_Shell_ControllerLayout
//================================================
//*/
class idMenuScreen_Shell_ControllerLayout : public idMenuScreen {
public:

	/*
	================================================
	idMenuDataSource_ControlSettings
	================================================
	*/
	class idMenuDataSource_LayoutSettings : public idMenuDataSource {
	public:
		typedef enum controlSettingFields_e : uint8 {
			LAYOUT_FIELD_LAYOUT,
			MAX_LAYOUT_FIELDS,
		} controlSettingFields_t;

		idMenuDataSource_LayoutSettings();

		// loads data
		void				LoadData() override;

		// submits data
		void				CommitData() override;

		// says whether something changed with the data
		bool				IsDataChanged() const override;

		// retrieves a particular field for reading or updating
		idSWFScriptVar		GetField( const int fieldIndex ) const override { return fields[ fieldIndex ]; }

		void				AdjustField( const int fieldIndex, const int adjustAmount ) override;

	private:
		idStaticList< idSWFScriptVar, MAX_LAYOUT_FIELDS >	fields;
		idStaticList< idSWFScriptVar, MAX_LAYOUT_FIELDS >	originalFields;
	};

	idMenuScreen_Shell_ControllerLayout() :
		btnBack(nullptr),
		options(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;

	void						UpdateBindingInfo();
private:

	idMenuDataSource_LayoutSettings		layoutData;
	idMenuWidget_DynamicList *			options;
	idMenuWidget_Button	*				btnBack;
};

//*
//================================================	
//idMenuScreen_Shell_SystemOptions
//================================================
//*/
class idMenuScreen_Shell_SystemOptions : public idMenuScreen {
public:

	/*
	================================================
	idMenuDataSource_SystemSettings
	================================================
	*/
	class idMenuDataSource_SystemSettings : public idMenuDataSource {
	public:
		typedef enum systemSettingFields_e : uint8 {
			SYSTEM_FIELD_FULLSCREEN,
			SYSTEM_FIELD_FRAMERATE,
			SYSTEM_FIELD_VSYNC,
			SYSTEM_FIELD_ANTIALIASING,
			SYSTEM_FIELD_MOTIONBLUR,
			SYSTEM_FIELD_LODBIAS,
			SYSTEM_FIELD_BRIGHTNESS,
			SYSTEM_FIELD_VOLUME,
			MAX_SYSTEM_FIELDS
		} systemSettingFields_t;

		idMenuDataSource_SystemSettings();

		// loads data
		void				LoadData() override;

		// submits data
		void				CommitData() override;

		// says whether something changed with the data
		bool				IsDataChanged() const override;

		// retrieves a particular field for reading
		idSWFScriptVar		GetField( const int fieldIndex ) const override;

		// updates a particular field value
		void				AdjustField( const int fieldIndex, const int adjustAmount ) override;

		bool						IsRestartRequired() const;

	private:
		int originalFramerate;
		int originalAntialias;
		int originalMotionBlur;
		int originalVsync;
		float originalBrightness;
		float originalVolume;

		idList<vidMode_t>			modeList;
	};

	idMenuScreen_Shell_SystemOptions() : 
		options(nullptr),
		btnBack(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;

private:
	idMenuWidget_DynamicList *	options;
	idMenuDataSource_SystemSettings	systemData;
	idMenuWidget_Button	*		btnBack;
	
};

//*
//================================================	
//idMenuScreen_Shell_Stereoscopics
//================================================
//*/
class idMenuScreen_Shell_Stereoscopics : public idMenuScreen {
public:

	/*
	================================================
	idMenuDataSource_StereoSettings
	================================================
	*/
	class idMenuDataSource_StereoSettings : public idMenuDataSource {
	public:
		typedef enum stereoSettingFields_e : uint8 {
			STEREO_FIELD_ENABLE,
			STEREO_FIELD_SEPERATION,
			STEREO_FIELD_SWAP_EYES,
			MAX_STEREO_FIELDS
		} stereoSettingFields_t;

		idMenuDataSource_StereoSettings();

		// loads data
		void				LoadData() override;

		// submits data
		void				CommitData() override;

		// says whether something changed with the data
		bool				IsDataChanged() const override;

		// retrieves a particular field for reading or updating
		idSWFScriptVar		GetField( const int fieldIndex ) const override;

		void				AdjustField( const int fieldIndex, const int adjustAmount ) override;

		bool						IsRestartRequired() const;

	private:
		idStaticList< idSWFScriptVar, MAX_STEREO_FIELDS >	fields;
		idStaticList< idSWFScriptVar, MAX_STEREO_FIELDS >	originalFields;
	};

	idMenuScreen_Shell_Stereoscopics() : 
		options(nullptr),
		btnBack(nullptr),
		leftEyeMat(nullptr),
		rightEyeMat(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;
private:
	idMenuWidget_DynamicList *	options;
	idMenuDataSource_StereoSettings	stereoData;
	idMenuWidget_Button	*		btnBack;
	const idMaterial *			leftEyeMat;
	const idMaterial *			rightEyeMat;
};

//*
//================================================	
//idMenuScreen_Shell_PartyLobby
//================================================
//*/
class idMenuScreen_Shell_PartyLobby : public idMenuScreen {
public:
	idMenuScreen_Shell_PartyLobby() : 
		options(nullptr),
		lobby(nullptr),
		isHost( false ),
		isPeer( false ),
		btnBack(nullptr),
		inParty( false ) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;

	void						UpdateOptions();
	void						UpdateLobby();
	bool						CanKickSelectedPlayer( lobbyUserID_t & luid );
	void						ShowLeaderboards();

private:

	bool							isHost;
	bool							isPeer;
	bool							inParty;
	idMenuWidget_DynamicList *		options;
	idMenuWidget_LobbyList *		lobby;
	idMenuWidget_Button	*			btnBack;
	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > menuOptions;	
};

//*
//================================================	
//idMenuScreen_Shell_GameLobby
//================================================
//*/
class idMenuScreen_Shell_GameLobby : public idMenuScreen {
public:
	idMenuScreen_Shell_GameLobby() : 
		options(nullptr),
		lobby(nullptr),
		longCountdown( 0 ),
		shortCountdown( 0 ),
		longCountRemaining( 0 ),
		isPeer( false ),
		isHost( false ),
		privateGameLobby( true ),
		btnBack(nullptr) {
	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	void				HideScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;
	void						UpdateLobby();
	bool						CanKickSelectedPlayer( lobbyUserID_t & luid );

private:

	int								longCountdown;
	int								longCountRemaining;
	int								shortCountdown;

	bool							isHost;
	bool							isPeer;
	bool							privateGameLobby;

	idMenuWidget_DynamicList *		options;
	idMenuWidget_LobbyList *		lobby;
	idMenuWidget_Button	*			btnBack;
	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > menuOptions;
};

//*
//================================================
//idMenuScreen_HUD
//================================================
//*/
class idMenuScreen_HUD : public idMenuScreen {
public:

	idMenuScreen_HUD() : 
		weaponInfo(nullptr),
		playerInfo(nullptr),
		audioLog(nullptr),
		communication(nullptr),
		oxygen(nullptr),
		stamina(nullptr),
		weaponName(nullptr),
		weaponImg(nullptr),
		weaponPills(nullptr),
		locationName(nullptr),
		downloadPda(nullptr),
		downloadVideo(nullptr),
		newPDA(nullptr),
		newVideo(nullptr),
		objective(nullptr),
		objectiveComplete(nullptr),
		tipInfo(nullptr),
		healthBorder(nullptr),
		healthPulse(nullptr),
		armorFrame(nullptr),
		security(nullptr),
		securityText(nullptr), 
		newPDADownload(nullptr), 
		newPDAHeading(nullptr), 
		newPDAName(nullptr),
		newVideoDownload(nullptr),
		newVideoHeading(nullptr),
		audioLogPrevTime( 0 ),
		commPrevTime( 0 ),
		oxygenComm( false ),
		inVacuum( false ),
		ammoInfo(nullptr),
		newWeapon(nullptr),
		pickupInfo(nullptr),
		cursorState( CURSOR_NONE ),
		cursorInCombat( 0 ),
		cursorTalking( 0 ),
		cursorItem( 0 ),
		cursorGrabber( 0 ),
		cursorNone( 0 ),
		talkCursor(nullptr),
		combatCursor(nullptr),
		grabberCursor(nullptr),
		bsInfo(nullptr),
		soulcubeInfo(nullptr),
		mpInfo(nullptr),
		mpHitInfo(nullptr),
		mpTime(nullptr),
		mpMessage(nullptr),
		mpChat(nullptr),
		mpWeapons(nullptr),
		newItem(nullptr),
		respawnMessage(nullptr),
		flashlight(nullptr),
		mpChatObject(nullptr),
		showSoulCubeInfoOnLoad( false ),
		mpConnection(nullptr) {
	}

	void			Initialize( idMenuHandler * data ) override;
	void			Update() override;
	void			ShowScreen( const mainMenuTransition_t transitionType ) override;
	void			HideScreen( const mainMenuTransition_t transitionType ) override;

	void					UpdateHealthArmor( idPlayer * player );
	void					UpdateStamina( idPlayer * player );
	void					UpdateLocation( idPlayer * player );
	void					UpdateWeaponInfo( idPlayer * player );
	void					UpdateWeaponStates( idPlayer * player, bool weaponChanged );
	void					ShowTip( const char * title, const char * tip );
	void					HideTip();
	void					DownloadVideo();
	void					DownloadPDA( const idDeclPDA * pda, bool newSecurity );
	void					UpdatedSecurity();
	void					ToggleNewVideo( bool show );
	void					ClearNewPDAInfo();
	void					ToggleNewPDA( bool show );
	void					UpdateAudioLog( bool show );
	void					UpdateCommunication( bool show, idPlayer * player );
	void					UpdateOxygen( bool show, int val = 0 );
	void					SetupObjective( const idStr & title, const idStr & desc, const idMaterial * screenshot );
	void					SetupObjectiveComplete( const idStr & title );
	void					ShowObjective( bool complete );
	void					HideObjective( bool complete );
	void					GiveWeapon( idPlayer * player, int weaponIndex );
	void					UpdatePickupInfo( index_t index, const idStr & name );
	bool					IsPickupListReady();
	void					ShowPickups();
	void					SetCursorState( idPlayer * player, cursorState_t state, int set );
	void					SetCursorText( const idStr & action, const idStr & focus );
	void					UpdateCursorState();
	void					CombatCursorFlash();
	void					UpdateSoulCube( bool ready );
	void					ShowRespawnMessage( bool show );
	void					SetShowSoulCubeOnLoad(const bool show ) { showSoulCubeInfoOnLoad = show; }

	// MULTIPLAYER

	void					ToggleMPInfo( bool show, bool showTeams, bool isCTF = false );
	void					SetFlagState( index_t team, int state );
	void					SetTeamScore( index_t team, int score );
	void					SetTeam( index_t team );
	void					TriggerHitTarget( bool show, const idStr & target, int color = 0 );
	void					ToggleLagged( bool show );
	void					UpdateGameTime( const char * time );
	void					UpdateMessage( bool show, const idStr & message );
	void					ShowNewItem( const char * name, const char * icon );
	void					UpdateFlashlight( idPlayer * player );
	void					UpdateChattingHud( idPlayer * player );
	
private: 

	idSWFScriptObject *		weaponInfo;
	idSWFScriptObject *		playerInfo;
	idSWFScriptObject *		stamina;
	idSWFScriptObject *		weaponName;	
	idSWFScriptObject *		weaponPills;
	idSWFScriptObject *		downloadPda;
	idSWFScriptObject *		downloadVideo;
	idSWFScriptObject *		tipInfo;
	idSWFScriptObject *		mpChat;
	idSWFScriptObject *		mpWeapons;

	idSWFSpriteInstance *	healthBorder;
	idSWFSpriteInstance *	healthPulse;
	idSWFSpriteInstance *	armorFrame;
	idSWFSpriteInstance *	security;
	idSWFSpriteInstance *	newPDADownload;
	idSWFSpriteInstance *	newVideoDownload;
	idSWFSpriteInstance *	newPDA;
	idSWFSpriteInstance *	newVideo;
	idSWFSpriteInstance *	audioLog;
	idSWFSpriteInstance *	communication;
	idSWFSpriteInstance *	oxygen;
	idSWFSpriteInstance *	objective;
	idSWFSpriteInstance *	objectiveComplete;
	idSWFSpriteInstance *	ammoInfo;
	idSWFSpriteInstance *	weaponImg;
	idSWFSpriteInstance *	newWeapon;
	idSWFSpriteInstance *	pickupInfo;
	idSWFSpriteInstance *	talkCursor;
	idSWFSpriteInstance *	combatCursor;
	idSWFSpriteInstance *	grabberCursor;
	idSWFSpriteInstance *	bsInfo;
	idSWFSpriteInstance *	soulcubeInfo;
	idSWFSpriteInstance *	newItem;
	idSWFSpriteInstance	*	respawnMessage;
	idSWFSpriteInstance *	flashlight;
	idSWFSpriteInstance *	mpChatObject;
	idSWFSpriteInstance *	mpConnection;

	idSWFSpriteInstance *	mpInfo;
	idSWFSpriteInstance *	mpHitInfo;

	idSWFTextInstance *		locationName;
	idSWFTextInstance *		securityText;
	idSWFTextInstance *		newPDAName;
	idSWFTextInstance *		newPDAHeading;
	idSWFTextInstance *		newVideoHeading;

	idSWFTextInstance *		mpMessage;
	idSWFTextInstance *		mpTime;
		
	ID_TIME_T				audioLogPrevTime;
	ID_TIME_T				commPrevTime;

	bool					oxygenComm;
	bool					inVacuum;

	idStr					objTitle;
	idStr					objDesc;
	const idMaterial *		objScreenshot;
	idStr					objCompleteTitle;

	cursorState_t			cursorState;
	int						cursorInCombat;
	int						cursorTalking;
	int						cursorItem;
	int						cursorGrabber;
	int						cursorNone;
	idStr					cursorAction;
	idStr					cursorFocus;

	bool					showSoulCubeInfoOnLoad;
};

//*
//================================================	
//idMenuScreen_Scoreboard
//================================================
//*/
class idMenuScreen_Scoreboard : public idMenuScreen {
public:

	idMenuScreen_Scoreboard() : 
		playerList(nullptr) {

	}

	void				Initialize( idMenuHandler * data ) override;
	void				Update() override;
	void				ShowScreen( const mainMenuTransition_t transitionType ) override;
	bool				HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled = false ) override;

	virtual void				SetPlayerData( idList< scoreboardInfo_t, TAG_IDLIB_LIST_MENU > data );
	virtual void				UpdateTeamScores( int r, int b );
	virtual void				UpdateGameInfo( idStr gameInfo );
	virtual void				UpdateSpectating( idStr spectating, idStr follow );
	virtual void				UpdateHighlight();

protected:
	idMenuWidget_ScoreboardList *		playerList;

};

//*
//================================================	
//idMenuScreen_Scoreboard_CTF
//================================================
//*/
class idMenuScreen_Scoreboard_CTF : public idMenuScreen_Scoreboard {
public:
	void				Initialize( idMenuHandler * data ) override;	
};

//*
//================================================	
//idMenuScreen_Scoreboard_Team
//================================================
//*/
class idMenuScreen_Scoreboard_Team : public idMenuScreen_Scoreboard {
public:
	void				Initialize( idMenuHandler * data ) override;
};


/*
========================
InvitePartyOrFriends

Invites the master local user's party, if he's in one and the party isn't in the lobby already.
Otherwise brings up the invite friends system menu.
========================
*/
inline void InvitePartyOrFriends() {
	const idLocalUser * const user = session->GetSignInManager().GetMasterLocalUser();
	if ( user != nullptr && user->IsInParty() && user->GetPartyCount() > 1 && !session->IsPlatformPartyInLobby() ) {
		session->InviteParty();
	} else {
		session->InviteFriends();
	}
}

#endif
