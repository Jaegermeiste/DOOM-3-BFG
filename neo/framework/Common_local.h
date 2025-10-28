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
#ifndef __COMMON_LOCAL_H__
#define __COMMON_LOCAL_H__

#pragma once

static constexpr size_t    MAX_USERCMD_BACKUP = 256;
static constexpr size_t    NUM_USERCMD_RELAY = 10;
static constexpr size_t    NUM_USERCMD_SEND = 8;

static constexpr double    initialHz = 60.0;
static constexpr double    initialBaseTicks = 1000.0 / initialHz;
static constexpr double    initialBaseTicksPerSec = initialHz * initialBaseTicks;

static constexpr ID_TIME_T LOAD_TIP_CHANGE_INTERVAL = 12000;
static constexpr size_t    LOAD_TIP_COUNT = 26;

class idGameThread : public idSysThread {
public:
	idGameThread() :
		gameTime(),
		drawTime(),
		threadTime(),
		threadGameTime(),
		threadRenderTime(),
		userCmdMgr(nullptr),
		ret(),
		numGameFrames(),
		isClient()
	{}

	// the gameReturn_t is from the previous frame, the
	// new frame will be running in parallel on exit
	gameReturn_t	RunGameAndDraw( size_t numGameFrames, idUserCmdMgr & userCmdMgr_, bool isClient_, int startGameFrame );

	// Accessors to the stored frame/thread time information
	void			SetThreadTotalTime( const ID_MICROSEC_T inTime ) { threadTime = inTime; }
	ID_MICROSEC_T	    	GetThreadTotalTime() const { return threadTime; }

	void			SetThreadGameTime( const ID_MICROSEC_T time ) { threadGameTime = time; }
	ID_MICROSEC_T	    	GetThreadGameTime() const { return threadGameTime; }

	void			SetThreadRenderTime( const ID_MICROSEC_T time ) { threadRenderTime = time; }
	ID_MICROSEC_T  		GetThreadRenderTime() const { return threadRenderTime; }

private:
	int	Run() override;

	ID_TIME_T		gameTime;
	ID_TIME_T		drawTime;
	ID_TIME_T		threadTime;					// total time : game time + foreground render time
	ID_TIME_T		threadGameTime;				// game time only
	ID_TIME_T		threadRenderTime;			// render fg time only
	idUserCmdMgr *	userCmdMgr;
	gameReturn_t	ret;
	size_t			numGameFrames;
	bool			isClient;
};

enum errorParm_t : uint8 {
	ERP_NONE,
	ERP_FATAL,						// exit the entire game with a popup window
	ERP_DROP,						// print to console and disconnect from game
	ERP_DISCONNECT					// don't kill server
};

enum gameLaunch_t : uint8 {
	LAUNCH_TITLE_DOOM = 0,
	LAUNCH_TITLE_DOOM2,
};

struct netTimes_t {
	ID_TIME_T localTime;
	ID_TIME_T serverTime;
};

struct frameTiming_t {
	ID_MICROSEC_T	startSyncTime;
	ID_MICROSEC_T	finishSyncTime;
	ID_MICROSEC_T	startGameTime;
	ID_MICROSEC_T	finishGameTime;
	ID_MICROSEC_T	finishDrawTime;
	ID_MICROSEC_T	startRenderTime;
	ID_MICROSEC_T	finishRenderTime;
};

constexpr size_t MAX_PRINT_MSG_SIZE = 4096;
constexpr size_t MAX_WARNING_LIST   = 256;

constexpr auto   SAVEGAME_CHECKPOINT_FILENAME  = "gamedata.save";
constexpr auto   SAVEGAME_DESCRIPTION_FILENAME = "gamedata.txt";
constexpr auto   SAVEGAME_STRINGS_FILENAME     = "gamedata.strings";

class idCommonLocal : public idCommon {
public:
								idCommonLocal();

								void				Init( int argc, const char * const * argv, const char *cmdline ) override;
								void				Shutdown() override;
								void				CreateMainMenu() override;
								void				Quit() override;
								bool				IsInitialized() const override;
								void				Frame() override;
								void				UpdateScreen( bool captureToImage ) override;
								void				UpdateLevelLoadPacifier() override;
								void				StartupVariable( const char * match ) override;
	virtual void				WriteConfigToFile( const char *filename );
								void				BeginRedirect( char *buffer, size_t buffersize, void (*flush)( const char * ) ) override;
								void				EndRedirect() override;
								void				SetRefreshOnPrint( bool set ) override;
								void				Printf( VERIFY_FORMAT_STRING const char *fmt, ... ) override;
								void				VPrintf( const char *fmt, va_list arg ) override;
								void				DPrintf( VERIFY_FORMAT_STRING const char *fmt, ... ) override;
								void				Warning( VERIFY_FORMAT_STRING const char *fmt, ... ) override;
								void				DWarning( VERIFY_FORMAT_STRING const char *fmt, ...) override;
								void				PrintWarnings() override;
								void				ClearWarnings( const char *reason ) override;
								void				Error( VERIFY_FORMAT_STRING const char *fmt, ... ) override;
								void				FatalError( VERIFY_FORMAT_STRING const char *fmt, ... ) override;
								bool				IsShuttingDown() const override { return com_shuttingDown; }

								const char *		KeysFromBinding( const char *bind ) override;
								const char *		BindingFromKey( const char *key ) override;

								bool				IsMultiplayer() override;
								bool				IsServer() override;
								bool				IsClient() override;

								bool				GetConsoleUsed() override { return consoleUsed; }

								int					GetSnapRate() override;

								void				NetReceiveReliable( index_t peer, int type, idBitMsg & msg ) override;
								void				NetReceiveSnapshot( class idSnapShot & ss ) override;
								void				NetReceiveUsercmds( index_t peer, idBitMsg & msg ) override;
	void						NetReadUsercmds( index_t clientNum, idBitMsg & msg );

								bool				ProcessEvent( const sysEvent_t *event ) override;

								bool				LoadGame( const char * saveName ) override;
								bool				SaveGame( const char * saveName ) override;

								int					ButtonState( usercmdButton_t key ) override;
								int					KeyState( keyNum_t key ) override;

								idDemoFile *		ReadDemo() override { return readDemo; }
								idDemoFile *		WriteDemo() override { return writeDemo; }

								idGame *			Game() override { return game; }
								idRenderWorld *		RW() override { return renderWorld; }
								idSoundWorld *		SW() override { return soundWorld; }
								idSoundWorld *		MenuSW() override { return menuSoundWorld; }
								idSession *			Session() override { return session; }
								idCommonDialog &	Dialog() override { return commonDialog; }

								void				OnSaveCompleted( idSaveLoadParms & parms ) override;
								void				OnLoadCompleted( idSaveLoadParms & parms ) override;
								void				OnLoadFilesCompleted( idSaveLoadParms & parms ) override;
								void				OnEnumerationCompleted( idSaveLoadParms & parms ) override;
								void				OnDeleteCompleted( idSaveLoadParms & parms ) override;
								void				TriggerScreenWipe( const char * _wipeMaterial, bool hold ) override;

								void				OnStartHosting( idMatchParameters & parms ) override;

								size_t				GetGameFrame() override { return gameFrame; }

								void				LaunchExternalTitle( index_t titleIndex,
												                         index_t device,
												                         const lobbyConnectInfo_t * const connectInfo ) override; // For handling invitations. NULL if no invitation used.

								void				InitializeMPMapsModes() override;
								const idStrList &			GetModeList() const override { return mpGameModes; }
								const idStrList &			GetModeDisplayList() const override { return mpDisplayGameModes; }
								const idList<mpMap_t> &		GetMapList() const override { return mpGameMaps; }

								void				ResetPlayerInput( index_t playerIndex ) override;

								bool				JapaneseCensorship() const override;

								void				QueueShowShell() override { showShellRequested = true; }

								currentGame_t		GetCurrentGame() const override { return currentGame; }
								void				SwitchToGame( currentGame_t newGame ) override;		

public:
	void	Draw();			// called by gameThread

	ID_MICROSEC_T GetGameThreadTotalTime() const { return gameThread.GetThreadTotalTime(); }
	ID_MICROSEC_T	GetGameThreadGameTime() const { return gameThread.GetThreadGameTime(); }
	ID_MICROSEC_T	GetGameThreadRenderTime() const { return gameThread.GetThreadRenderTime(); }
	ID_MICROSEC_T	GetRendererBackEndMicroseconds() const { return time_backend; }
	ID_MICROSEC_T	GetRendererShadowsMicroseconds() const { return time_shadows; }
	ID_MICROSEC_T	GetRendererIdleMicroseconds() const { return mainFrameTiming.startRenderTime - mainFrameTiming.finishSyncTime; }
	ID_MICROSEC_T	GetRendererGPUMicroseconds() const { return time_gpu; }

	frameTiming_t		frameTiming;
	frameTiming_t		mainFrameTiming;

public:	// These are public because they are called directly by static functions in this file

	const char * GetCurrentMapName() const { return currentMapName.c_str(); }

	// loads a map and starts a new game on it
	void	StartNewGame( const char * mapName, bool devmap, int8 gameMode );
	void	LeaveGame();

	void	DemoShot( const char *name );
	void	StartRecordingRenderDemo( const char *name );
	void	StopRecordingRenderDemo();
	void	StartPlayingRenderDemo( idStr name );
	void	StopPlayingRenderDemo();
	void	CompressDemoFile( const char *scheme, const char *name );
	void	TimeRenderDemo( const char *name, bool twice = false, bool quit = false );
	void	AVIRenderDemo( const char *name );
	void	AVIGame( const char *name );

	// localization
	void	InitLanguageDict();
	void	LocalizeGui( const char *fileName, idLangDict &langDict );
	void	LocalizeMapData( const char *fileName, idLangDict &langDict );
	void	LocalizeSpecificMapData( const char *fileName, idLangDict &langDict, const idLangDict &replaceArgs );

	idUserCmdMgr & GetUCmdMgr() { return userCmdMgr; }

private:
	bool						com_fullyInitialized;
	bool						com_refreshOnPrint;		// update the screen every print for dmap
	errorParm_t					com_errorEntered;
	bool						com_shuttingDown;
	bool						com_isJapaneseSKU;

	idFile *					logFile;

	char						errorMessage[MAX_PRINT_MSG_SIZE];

	char *						rd_buffer;
	size_t						rd_buffersize;
	void						(*rd_flush)( const char *buffer );

	idStr						warningCaption;
	idStrList					warningList;
	idStrList					errorList;

	int							gameDLL;

	idCommonDialog				commonDialog;

	idFile_SaveGame 			saveFile;
	idFile_SaveGame 			stringsFile;
	idFile_SaveGamePipelined 	*pipelineFile;

	// The main render world and sound world
	idRenderWorld *		renderWorld;
	idSoundWorld *		soundWorld;

	// The renderer and sound system will write changes to writeDemo.
	// Demos can be recorded and played at the same time when splicing.
	idDemoFile *		readDemo;
	idDemoFile *		writeDemo;
	
	bool				menuActive;
	idSoundWorld *		menuSoundWorld;			// so the game soundWorld can be muted

	bool				insideExecuteMapChange;	// Enable Pacifier Updates

	// This is set if the player enables the console, which disables achievements
	bool				consoleUsed;

	// This additional information is required for ExecuteMapChange for SP games ONLY
	// This data is cleared after ExecuteMapChange
	struct mapSpawnData_t {
		idFile_SaveGame *	savegameFile;				// Used for loading a save game
		idFile_SaveGame *	stringTableFile;			// String table read from save game loaded
		idFile_SaveGamePipelined *pipelineFile;			
		int					savegameVersion;			// Version of the save game we're loading
		idDict				persistentPlayerInfo;		// Used for transitioning from map to map
	};
	mapSpawnData_t		mapSpawnData;
	idStr				currentMapName;			// for checking reload on same level
	bool				mapSpawned;				// cleared on Stop()

	bool				insideUpdateScreen;		// true while inside ::UpdateScreen()

	idUserCmdMgr		userCmdMgr;
	
	ID_TIME_T			nextUsercmdSendTime;	// Next time to send usercmds
	ID_TIME_T			nextSnapshotSendTime;	// Next time to send a snapshot

	idSnapShot			lastSnapShot;		// last snapshot we received from the server
	struct reliableMsg_t {
		int	client;
		int type;
		size_t dataSize;
		byte * data;
	};
	idList<reliableMsg_t> reliableQueue;


	// Snapshot interpolation
	idSnapShot		oldss;				// last local snapshot
										// (ie on server this is the last "master" snapshot  we created)
										// (on clients this is the last received snapshot)
										// used for comparisons with the new snapshot for com_drawSnapshot

	// This is ultimately controlled by net_maxBufferedSnapshots by running double speed, but this is the hard max before seeing visual popping
	static constexpr size_t RECEIVE_SNAPSHOT_BUFFER_SIZE = 16;			

	index_t			readSnapshotIndex;
	index_t			writeSnapshotIndex;
	idArray<idSnapShot,RECEIVE_SNAPSHOT_BUFFER_SIZE>	receivedSnaps;

	float			optimalPCTBuffer;
	double  		optimalTimeBuffered;
	double			optimalTimeBufferedWindow;

	double  		snapRate;
	double  		actualRate;

	ID_TIME_T		snapTime;			// time we got the most recent snapshot
	ID_TIME_T		snapTimeDelta;		// time interval that current ss was sent in

	ID_TIME_T		snapTimeWrite;
	ID_TIME_T		snapCurrentTime;	// realtime playback time
	netTimes_t		snapCurrent;		// current snapshot
	netTimes_t		snapPrevious;		// previous snapshot
	double			snapCurrentResidual;

	double  		snapTimeBuffered;
	double			effectiveSnapRate;
	ID_TIME_T		totalBufferedTime;
	ID_TIME_T		totalRecvTime;



	int					clientPrediction;

	size_t				gameFrame;			// Frame number of the local game
	double   			gameTimeResidual;	// left over msec from the last game frame
	bool				syncNextGameFrame;

	bool				aviCaptureMode;		// if true, screenshots will be taken and sound captured
	idStr				aviDemoShortName;	// 
	size_t				aviDemoFrameCount;

	enum timeDemo_t {
		TD_NO,
		TD_YES,
		TD_YES_THEN_QUIT
	};
	timeDemo_t			timeDemo;
	ID_TIME_T			timeDemoStartTime;
	size_t				numDemoFrames;		// for timeDemo and demoShot
	ID_TIME_T			demoTimeOffset;
	renderView_t		currentDemoRenderView;

	idStrList			mpGameModes;
	idStrList			mpDisplayGameModes;
	idList<mpMap_t>		mpGameMaps;

	idSWF *				loadGUI;
	ID_TIME_T			nextLoadTip;
	bool				isHellMap;
	bool				defaultLoadscreen;
	idStaticList<int, LOAD_TIP_COUNT>	loadTipList;

	const idMaterial *	splashScreen;

	const idMaterial *	whiteMaterial;

	const idMaterial *	wipeMaterial;
	ID_TIME_T			wipeStartTime;
	ID_TIME_T			wipeStopTime;
	bool				wipeHold;
	bool				wipeForced;		// used for the PS3 to start an early wipe while we are accessing saved game data

	idGameThread		gameThread;				// the game and draw code can be run in parallel

	// com_speeds times
	size_t				count_numGameFrames;	// total number of game frames that were run
	ID_TIME_T			time_gameFrame;			// game logic time
	ID_TIME_T			time_maxGameFrame;		// maximum single frame game logic time
	ID_TIME_T			time_gameDraw;			// game present time
	ID_MICROSEC_T		    	time_frontend;			// renderer frontend time microseconds
	ID_MICROSEC_T			    time_backend;			// renderer backend time microseconds
	ID_MICROSEC_T			    time_shadows;			// renderer backend waiting for shadow volumes to be created (microseconds)
	ID_MICROSEC_T		    	time_gpu;				// total gpu time, at least for PC (microseconds)

	// Used during loading screens
	ID_TIME_T			lastPacifierSessionTime;
	ID_TIME_T			lastPacifierGuiTime;
	bool				lastPacifierDialogState;

	bool				showShellRequested;

	currentGame_t		currentGame;
	currentGame_t		idealCurrentGame;		// Defer game switching so that bad things don't happen in the middle of the frame.
	const idMaterial *	doomClassicMaterial;

	static constexpr size_t			DOOMCLASSIC_RENDERWIDTH = 320 * 3;
	static constexpr size_t			DOOMCLASSIC_RENDERHEIGHT = 200 * 3;
	static constexpr size_t			DOOMCLASSIC_BYTES_PER_PIXEL = 4;
	static constexpr size_t			DOOMCLASSIC_IMAGE_SIZE_IN_BYTES = DOOMCLASSIC_RENDERWIDTH * DOOMCLASSIC_RENDERHEIGHT * DOOMCLASSIC_BYTES_PER_PIXEL;
	
	idArray< byte, DOOMCLASSIC_IMAGE_SIZE_IN_BYTES >	doomClassicImageData;

private:
	void	InitCommands();
	void	InitSIMD();
	void	AddStartupCommands();
	void	ParseCommandLine( int argc, const char * const * argv );
	bool	SafeMode();
	void	CloseLogFile();
	void	WriteConfiguration();
	void	DumpWarnings();
	void	LoadGameDLL();
	void	UnloadGameDLL();
	void	CleanupShell();
	void	RenderBink( const char * path );
	void	RenderSplash();
	void	FilterLangList( idStrList* list, idStr lang );
	void	CheckStartupStorageRequirements();

	void	ExitMenu();
	bool	MenuEvent( const sysEvent_t * event );
	
	void	StartMenu( bool playIntro = false );
	void	GuiFrameEvents();

	void	BeginAVICapture( const char *name );
	void	EndAVICapture();

	void	AdvanceRenderDemo( bool singleFrameOnly );

	void	ProcessGameReturn( const gameReturn_t & ret );

	void	RunNetworkSnapshotFrame();
	void	ExecuteReliableMessages();


	// Snapshot interpolation
	void	ProcessSnapshot( idSnapShot & ss );
	ID_TIME_T	CalcSnapTimeBuffered( ID_TIME_T & totalBufferedTime, ID_TIME_T & totalRecvTime  );
	void	ProcessNextSnapshot();
	void	InterpolateSnapshot( netTimes_t & prev, netTimes_t & next, double fraction, bool predict );
	void	ResetNetworkingState();

	int		NetworkFrame();
	void	SendSnapshots();
	void	SendUsercmds( index_t localClientNum );
	
	void	LoadLoadingGui(const char *mapName, bool & hellMap );

	// Meant to be used like:
	// while ( waiting ) { BusyWait(); }
	void	BusyWait();
	bool	WaitForSessionState( idSession::sessionState_t desiredState );

	void	ExecuteMapChange();
	void	UnloadMap();

	void	Stop( bool resetSession = true );

	// called by Draw when the scene to scene wipe is still running
	void	DrawWipeModel() const;
	void	StartWipe( const char *materialName, bool hold = false);
	void	CompleteWipe();
	void	ClearWipe();

	void	MoveToNewMap( const char * mapName, bool devmap );

	void	PlayIntroGui();
	
	void	ScrubSaveGameFileName( idStr &saveFileName ) const;

	// Doom classic support
	void	RunDoomClassicFrame();
	void	RenderDoomClassic();
	bool	IsPlayingDoomClassic() const { return GetCurrentGame() != DOOM3_BFG; }
	void	PerformGameSwitch();
};

extern idCommonLocal commonLocal;
#endif // __COMMON_LOCAL_H__
