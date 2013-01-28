/*
===========================================================================
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of Spearmint Source Code.

Spearmint Source Code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Spearmint Source Code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Spearmint Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, Spearmint Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following
the terms and conditions of the GNU General Public License.  If not, please
request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional
terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/
//
/*
=============================================================================

SINGLE PLAYER POSTGAME MENU

=============================================================================
*/

#include "ui_local.h"

#ifndef TA_SP
#ifdef TURTLEARENA // AWARDS
#define MAX_UI_AWARDS		3
#else
#define MAX_UI_AWARDS		6
#endif

#define MAX_SCOREBOARD_CLIENTS		8

#define AWARD_PRESENTATION_TIME		2000
#endif

#ifdef TA_SP
#define ART_FRAME		"menu/art/cut_frame"
#endif
#define ART_MENU0		"menu/art/menu_0"
#define ART_MENU1		"menu/art/menu_1"
#define ART_REPLAY0		"menu/art/replay_0"
#define ART_REPLAY1		"menu/art/replay_1"
#define ART_NEXT0		"menu/art/next_0"
#define ART_NEXT1		"menu/art/next_1"

#define ID_AGAIN		10
#define ID_NEXT			11
#define ID_MENU			12

typedef struct {
	menuframework_s	menu;

#ifdef TA_SP
	// enter name
	menubitmap_s	art_frame;
	menufield_s		item_name;

	// after the game
#endif
	menubitmap_s	item_again;
	menubitmap_s	item_next;
	menubitmap_s	item_menu;

	int				phase;
#ifndef TA_SP
	int				ignoreKeysTime;
	int				starttime;
	int				scoreboardtime;
#endif
	int				serverId;

#ifdef TA_SP
	int				gametype;

	arcadeGameData_t gamedata;
	arcadeScore_t	newScore;
	int				scoreIndex;
#else
	int				clientNums[MAX_SCOREBOARD_CLIENTS];
	int				ranks[MAX_SCOREBOARD_CLIENTS];
	int				scores[MAX_SCOREBOARD_CLIENTS];

	char			placeNames[3][64];

	int				level;
	int				numClients;
	int				won;
	int				numAwards;
	int				awardsEarned[MAX_UI_AWARDS];
	int				awardsLevels[MAX_UI_AWARDS];
	qboolean		playedSound[MAX_UI_AWARDS];
	int				lastTier;
	sfxHandle_t		winnerSound;
#endif
} postgameMenuInfo_t;

static postgameMenuInfo_t	postgameMenuInfo;
#ifndef TA_SP
static char					arenainfo[MAX_INFO_VALUE];

char	*ui_medalNames[] = {"Accuracy",
#ifndef TURTLEARENA // AWARDS
"Impressive", "Excellent", "Gauntlet",
#endif
"Frags", "Perfect"};
char	*ui_medalPicNames[] = {
	"menu/medals/medal_accuracy",
#ifndef TURTLEARENA // AWARDS
	"menu/medals/medal_impressive",
	"menu/medals/medal_excellent",
	"menu/medals/medal_gauntlet",
#endif
	"menu/medals/medal_frags",
	"menu/medals/medal_victory"
};
char	*ui_medalSounds[] = {
	"sound/feedback/accuracy.wav",
#ifndef TURTLEARENA // AWARDS
	"sound/feedback/impressive_a.wav",
	"sound/feedback/excellent_a.wav",
	"sound/feedback/gauntlet.wav",
#endif
	"sound/feedback/frags.wav",
	"sound/feedback/perfect.wav"
};
#endif

/*
=================
UI_SPPostgameMenu_AgainEvent
=================
*/
static void UI_SPPostgameMenu_AgainEvent( void* ptr, int event )
{
	if (event != QM_ACTIVATED) {
		return;
	}
	UI_PopMenu();
	trap_Cmd_ExecuteText( EXEC_APPEND, "map_restart 0\n" );
}


#ifdef TA_SP
void UI_SavePostGameStats(const char *name);

/*
=================
UI_SPPostgameMenu_NextEvent
=================
*/
static void UI_SPPostgameMenu_NextEvent( void* ptr, int event ) {
	if (event != QM_ACTIVATED) {
		return;
	}

	trap_Cvar_Set("ui_arcadeName", postgameMenuInfo.item_name.field.buffer);
	UI_SavePostGameStats(postgameMenuInfo.item_name.field.buffer);

	// Move to the next phase
	postgameMenuInfo.phase = 2;

	postgameMenuInfo.art_frame.generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);
	postgameMenuInfo.item_name.generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);
	postgameMenuInfo.item_next.generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);

	postgameMenuInfo.item_menu.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	postgameMenuInfo.item_again.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
}
#else
/*
=================
UI_SPPostgameMenu_NextEvent
=================
*/
static void UI_SPPostgameMenu_NextEvent( void* ptr, int event ) {
	int			currentSet;
	int			levelSet;
	int			level;
	int			currentLevel;
	const char	*arenaInfo;

	if (event != QM_ACTIVATED) {
		return;
	}
	UI_PopMenu();

	// handle specially if we just won the training map
	if( postgameMenuInfo.won == 0 ) {
		level = 0;
	}
	else {
		level = postgameMenuInfo.level + 1;
	}
	levelSet = level / ARENAS_PER_TIER;

	currentLevel = UI_GetCurrentGame();
	if( currentLevel == -1 ) {
		currentLevel = postgameMenuInfo.level;
	}
	currentSet = currentLevel / ARENAS_PER_TIER;

	if( levelSet > currentSet || levelSet == UI_GetNumSPTiers() ) {
		level = currentLevel;
	}

	arenaInfo = UI_GetArenaInfoByNumber( level );
	if ( !arenaInfo ) {
		return;
	}

	UI_SPArena_Start( arenaInfo );
}
#endif


/*
=================
UI_SPPostgameMenu_MenuEvent
=================
*/
static void UI_SPPostgameMenu_MenuEvent( void* ptr, int event )
{
	if (event != QM_ACTIVATED) {
		return;
	}
	UI_PopMenu();
#ifdef TA_SP
	trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect; arcade\n" );
#else
	trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect; levelselect\n" );
#endif
}


/*
=================
UI_SPPostgameMenu_MenuKey
=================
*/
static sfxHandle_t UI_SPPostgameMenu_MenuKey( int key ) {
#ifndef TA_SP
	if ( uis.realtime < postgameMenuInfo.ignoreKeysTime ) {
		return 0;
	}

	if( postgameMenuInfo.phase == 1 ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "abort_podium\n" );
		postgameMenuInfo.phase = 2;
		postgameMenuInfo.starttime = uis.realtime;
		postgameMenuInfo.ignoreKeysTime	= uis.realtime + 250;
		return 0;
	}

	if( postgameMenuInfo.phase == 2 ) {
		postgameMenuInfo.phase = 3;
		postgameMenuInfo.starttime = uis.realtime;
		postgameMenuInfo.ignoreKeysTime	= uis.realtime + 250;
		return 0;
	}
#endif

	if( key == K_ESCAPE
#ifndef TA_MISC // MENU: Right Mouse button = left arrow
		|| key == K_MOUSE2
#endif
		)
	{
		return 0;
	}

	return Menu_DefaultKey( &postgameMenuInfo.menu, key );
}


#ifndef TA_SP
#ifdef TURTLEARENA // AWARDS
static int medalLocations[MAX_UI_AWARDS] = {144, 32, 560};
#else
static int medalLocations[6] = {144, 448, 88, 504, 32, 560};
#endif

static void UI_SPPostgameMenu_DrawAwardsMedals( int max ) {
	int		n;
	int		medal;
	int		amount;
	int		x, y;
	char	buf[16];

	for( n = 0; n < max; n++ ) {
		x = medalLocations[n];
		y = 64;
		medal = postgameMenuInfo.awardsEarned[n];
		amount = postgameMenuInfo.awardsLevels[n];

		UI_DrawNamedPic( x, y, 48, 48, ui_medalPicNames[medal] );

		if( medal == AWARD_ACCURACY ) {
			Com_sprintf( buf, sizeof(buf), "%i%%", amount );
		}
		else {
			if( amount == 1 ) {
				continue;
			}
			Com_sprintf( buf, sizeof(buf), "%i", amount );
		}

		UI_DrawString( x + 24, y + 52, buf, UI_CENTER, color_yellow );
	}
}


static void UI_SPPostgameMenu_DrawAwardsPresentation( int timer ) {
	int		awardNum;
	int		atimer;
	vec4_t	color;

	awardNum = timer / AWARD_PRESENTATION_TIME;
	atimer = timer % AWARD_PRESENTATION_TIME;

	color[0] = color[1] = color[2] = 1.0f;
	color[3] = (float)( AWARD_PRESENTATION_TIME - atimer ) / (float)AWARD_PRESENTATION_TIME;
	UI_DrawProportionalString( 320, 64, ui_medalNames[postgameMenuInfo.awardsEarned[awardNum]], UI_CENTER, color );

	UI_SPPostgameMenu_DrawAwardsMedals( awardNum + 1 );

	if( !postgameMenuInfo.playedSound[awardNum] ) {
		postgameMenuInfo.playedSound[awardNum] = qtrue;
		trap_S_StartLocalSound( trap_S_RegisterSound( ui_medalSounds[postgameMenuInfo.awardsEarned[awardNum]], qfalse ), CHAN_ANNOUNCER );
	}
}
#endif


#ifdef IOQ3ZTM
/*
=================
UI_SPPostgameMenu_MenuDrawScoreLine

#1  1500 [pic]  1:25 Name
=================
*/
static void UI_SPPostgameMenu_MenuDrawScoreLine( int startx, int y, vec4_t color, int rank, arcadeScore_t *score ) {
	int x = startx;
	int minutes;
	int seconds;

	// Draw score rank
	if (rank >= 0) {
		UI_DrawString( x, y, va( "#%i:", rank ), UI_SMALLFONT, color );
	}
	x += SMALLCHAR_WIDTH * 5;

	// Draw score (right justified)
	x += SMALLCHAR_WIDTH * 6;
	UI_DrawString( x, y, va( "%i", score->score ), UI_RIGHT|UI_SMALLFONT, color );

	// Draw player model icon
	// ZTM: FIXME: Don't call trap_R_RegisterShaderNoMip each time!
	x += 13;
	if (score->character[0])
		UI_DrawHandlePic( x, y - 4, 26, 26, trap_R_RegisterShaderNoMip(va("models/players/%s/icon_default", score->character)) );
	else
		UI_DrawHandlePic( x, y - 4, 26, 26, trap_R_RegisterShaderNoMip("menu/art/randombot_icon") );
	x += 26;

	// Draw time (right justified)
	minutes = score->time / 60;
	seconds = score->time % 60;
	x += SMALLCHAR_WIDTH * 7;
	UI_DrawString( x, y, va( "%d:%.2d", minutes, seconds ), UI_RIGHT|UI_SMALLFONT, color );

	// Draw user's name
	x += SMALLCHAR_WIDTH;
	UI_DrawString( x, y, va( "%s", score->name ), UI_SMALLFONT, color );

	if (postgameMenuInfo.gametype >= GT_CTF) {
		// ZTM: TODO: Draw captures, redScore, and blueScore
	}
}

/*
=================
UI_SPPostgameMenu_MenuDrawHighScores

#1  1500  1:25 ZTM
=================
*/
#define CENTER_X -640
static void UI_SPPostgameMenu_MenuDrawHighScores( int startx, int y ) {
	int x, centerx;
	int n;

	if (startx == CENTER_X) {
		x = centerx = (640 - SMALLCHAR_WIDTH * 27 - 39) / 2;
	} else {
		x = startx;
	}

	// Draw high score list
	if (startx == CENTER_X) {
		x = centerx;
	} else {
		x = startx;
	}
	y += SMALLCHAR_HEIGHT;

	for (n = 0; n < NUM_ARCADE_SCORES; ++n) {
		if (n == postgameMenuInfo.scoreIndex)
			UI_SPPostgameMenu_MenuDrawScoreLine(x, 325+n*(SMALLCHAR_HEIGHT+10), color_yellow, n+1, &postgameMenuInfo.gamedata.scores[n]);
		else
			UI_SPPostgameMenu_MenuDrawScoreLine(x, 325+n*(SMALLCHAR_HEIGHT+10), color_white, n+1, &postgameMenuInfo.gamedata.scores[n]);
	}

	if (postgameMenuInfo.scoreIndex == -1) {
		// Show new socre (which wasn't a high score)
		UI_SPPostgameMenu_MenuDrawScoreLine(x, 325+NUM_ARCADE_SCORES*(SMALLCHAR_HEIGHT+10), color_yellow, -1, &postgameMenuInfo.newScore);
	}
}
#else
/*
=================
UI_SPPostgameMenu_MenuDrawScoreLine
=================
*/
static void UI_SPPostgameMenu_MenuDrawScoreLine( int n, int y ) {
	int		rank;
	char	name[64];
	char	info[MAX_INFO_STRING];

	if( n > (postgameMenuInfo.numClients + 1) ) {
		n -= (postgameMenuInfo.numClients + 2);
	}

	if( n >= postgameMenuInfo.numClients ) {
		return;
	}

	rank = postgameMenuInfo.ranks[n];
	if( rank & RANK_TIED_FLAG ) {
		UI_DrawString( 640 - 31 * SMALLCHAR_WIDTH, y, "(tie)", UI_LEFT|UI_SMALLFONT, color_white );
		rank &= ~RANK_TIED_FLAG;
	}
	trap_GetConfigString( CS_PLAYERS + postgameMenuInfo.clientNums[n], info, MAX_INFO_STRING );
	Q_strncpyz( name, Info_ValueForKey( info, "n" ), sizeof(name) );
	Q_CleanStr( name );

	UI_DrawString( 640 - 25 * SMALLCHAR_WIDTH, y, va( "#%i: %-16s %2i", rank + 1, name, postgameMenuInfo.scores[n] ), UI_LEFT|UI_SMALLFONT, color_white );
}
#endif


/*
=================
UI_SPPostgameMenu_MenuDraw
=================
*/
static void UI_SPPostgameMenu_MenuDraw( void ) {
#ifndef TA_SP
	int		timer;
#endif
	int		serverId;
#ifndef TA_SP
	int		n;
#endif
	char	info[MAX_INFO_STRING];

	trap_GetConfigString( CS_SYSTEMINFO, info, sizeof(info) );
	serverId = atoi( Info_ValueForKey( info, "sv_serverid" ) );
	if( serverId != postgameMenuInfo.serverId ) {
		UI_PopMenu();
		return;
	}

#ifdef TA_SP
	if (postgameMenuInfo.phase == 2) {
		UI_SPPostgameMenu_MenuDrawHighScores(CENTER_X, 310);
	}

	Menu_Draw( &postgameMenuInfo.menu );
#else
	// phase 1
	if ( postgameMenuInfo.numClients > 2 ) {
		UI_DrawProportionalString( 510, 480 - 64 - PROP_HEIGHT, postgameMenuInfo.placeNames[2], UI_CENTER, color_white );
	}
	UI_DrawProportionalString( 130, 480 - 64 - PROP_HEIGHT, postgameMenuInfo.placeNames[1], UI_CENTER, color_white );
	UI_DrawProportionalString( 320, 480 - 64 - 2 * PROP_HEIGHT, postgameMenuInfo.placeNames[0], UI_CENTER, color_white );

	if( postgameMenuInfo.phase == 1 ) {
		timer = uis.realtime - postgameMenuInfo.starttime;

		if( timer >= 1000 && postgameMenuInfo.winnerSound ) {
			trap_S_StartLocalSound( postgameMenuInfo.winnerSound, CHAN_ANNOUNCER );
			postgameMenuInfo.winnerSound = 0;
		}

		if( timer < 5000 ) {
			return;
		}
		postgameMenuInfo.phase = 2;
		postgameMenuInfo.starttime = uis.realtime;
	}

	// phase 2
	if( postgameMenuInfo.phase == 2 ) {
		timer = uis.realtime - postgameMenuInfo.starttime;
		if( timer >= ( postgameMenuInfo.numAwards * AWARD_PRESENTATION_TIME ) ) {

			if( timer < 5000 ) {
				return;
			}

			postgameMenuInfo.phase = 3;
			postgameMenuInfo.starttime = uis.realtime;
		}
		else {
			UI_SPPostgameMenu_DrawAwardsPresentation( timer );
		}
	}

	// phase 3
	if( postgameMenuInfo.phase == 3 ) {
		if( uis.demoversion ) {
			if( postgameMenuInfo.won == 1 && UI_ShowTierVideo( 8 )) {
				trap_Cvar_Set( "nextmap", "" );
				trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect; cinematic demoEnd.RoQ\n" );
				return;
			}
		}
		else if( postgameMenuInfo.won > -1 && UI_ShowTierVideo( postgameMenuInfo.won + 1 )) {
			if( postgameMenuInfo.won == postgameMenuInfo.lastTier ) {
				trap_Cvar_Set( "nextmap", "" );
				trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect; cinematic end.RoQ\n" );
				return;
			}

			trap_Cvar_SetValue( "ui_spSelection", postgameMenuInfo.won * ARENAS_PER_TIER );
			trap_Cvar_Set( "nextmap", "levelselect" );
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "disconnect; cinematic tier%i.RoQ\n", postgameMenuInfo.won + 1 ) );
			return;
		}

		postgameMenuInfo.item_again.generic.flags &= ~QMF_INACTIVE;
		postgameMenuInfo.item_next.generic.flags &= ~QMF_INACTIVE;
		postgameMenuInfo.item_menu.generic.flags &= ~QMF_INACTIVE;

		UI_SPPostgameMenu_DrawAwardsMedals( postgameMenuInfo.numAwards );

		Menu_Draw( &postgameMenuInfo.menu );
	}

	// draw the scoreboard
	if( !trap_Cvar_VariableValue( "ui_spScoreboard" ) ) {
		return;
	}

	timer = uis.realtime - postgameMenuInfo.scoreboardtime;
	if( postgameMenuInfo.numClients <= 3 ) {
		n = 0;
	}
	else {
		n = timer / 1500 % (postgameMenuInfo.numClients + 2);
	}
	UI_SPPostgameMenu_MenuDrawScoreLine( n, 0 );
	UI_SPPostgameMenu_MenuDrawScoreLine( n + 1, 0 + SMALLCHAR_HEIGHT );
	UI_SPPostgameMenu_MenuDrawScoreLine( n + 2, 0 + 2 * SMALLCHAR_HEIGHT );
#endif
}


/*
=================
UI_SPPostgameMenu_Cache
=================
*/
void UI_SPPostgameMenu_Cache( void ) {
#ifndef TA_SP
	int			n;
	qboolean	buildscript;

	buildscript = trap_Cvar_VariableValue("com_buildscript");
#endif

#ifdef TA_SP
	trap_R_RegisterShaderNoMip( ART_FRAME );
#endif
	trap_R_RegisterShaderNoMip( ART_MENU0 );
	trap_R_RegisterShaderNoMip( ART_MENU1 );
	trap_R_RegisterShaderNoMip( ART_REPLAY0 );
	trap_R_RegisterShaderNoMip( ART_REPLAY1 );
#ifndef TA_SP
	trap_R_RegisterShaderNoMip( ART_NEXT0 );
	trap_R_RegisterShaderNoMip( ART_NEXT1 );
	for( n = 0; n < MAX_UI_AWARDS; n++ )
	{
		trap_R_RegisterShaderNoMip( ui_medalPicNames[n] );
		trap_S_RegisterSound( ui_medalSounds[n], qfalse );
	}

	if( buildscript ) {
		trap_S_RegisterSound( "music/loss.wav", qfalse );
		trap_S_RegisterSound( "music/win.wav", qfalse );
		trap_S_RegisterSound( "sound/player/announce/youwin.wav", qfalse );
	}
#endif
}


/*
=================
UI_SPPostgameMenu_Init
=================
*/
static void UI_SPPostgameMenu_Init( void ) {
	postgameMenuInfo.menu.wrapAround	= qtrue;
	postgameMenuInfo.menu.key			= UI_SPPostgameMenu_MenuKey;
	postgameMenuInfo.menu.draw			= UI_SPPostgameMenu_MenuDraw;
#ifndef TA_SP
	postgameMenuInfo.ignoreKeysTime		= uis.realtime + 1500;
#endif

	UI_SPPostgameMenu_Cache();

#ifdef TA_SP
	postgameMenuInfo.art_frame.generic.type			= MTYPE_BITMAP;
	postgameMenuInfo.art_frame.generic.name			= ART_FRAME;
	postgameMenuInfo.art_frame.generic.flags		= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	postgameMenuInfo.art_frame.generic.x			= 142;
	postgameMenuInfo.art_frame.generic.y			= 118;
	postgameMenuInfo.art_frame.width				= 359;
	postgameMenuInfo.art_frame.height				= 256;

	postgameMenuInfo.item_name.generic.type       = MTYPE_FIELD;
	postgameMenuInfo.item_name.generic.name       = "Name:";
	postgameMenuInfo.item_name.generic.flags      = QMF_PULSEIFFOCUS;
	postgameMenuInfo.item_name.generic.x	      = 270;
	postgameMenuInfo.item_name.generic.y	      = 220;
	postgameMenuInfo.item_name.field.widthInChars = 8;
	postgameMenuInfo.item_name.field.maxchars     = 8;
#endif

	postgameMenuInfo.item_menu.generic.type			= MTYPE_BITMAP;
	postgameMenuInfo.item_menu.generic.name			= ART_MENU0;
#ifdef TA_SP
	postgameMenuInfo.item_menu.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_HIDDEN|QMF_INACTIVE;
#else
	postgameMenuInfo.item_menu.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_INACTIVE;
#endif
	postgameMenuInfo.item_menu.generic.x			= 0;
	postgameMenuInfo.item_menu.generic.y			= 480-64;
	postgameMenuInfo.item_menu.generic.callback		= UI_SPPostgameMenu_MenuEvent;
	postgameMenuInfo.item_menu.generic.id			= ID_MENU;
	postgameMenuInfo.item_menu.width				= 128;
	postgameMenuInfo.item_menu.height				= 64;
	postgameMenuInfo.item_menu.focuspic				= ART_MENU1;

	postgameMenuInfo.item_again.generic.type		= MTYPE_BITMAP;
	postgameMenuInfo.item_again.generic.name		= ART_REPLAY0;
#ifdef TA_SP
	postgameMenuInfo.item_again.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_HIDDEN|QMF_INACTIVE;
	postgameMenuInfo.item_again.generic.x			= 640;
#else
	postgameMenuInfo.item_again.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS|QMF_INACTIVE;
	postgameMenuInfo.item_again.generic.x			= 320;
#endif
	postgameMenuInfo.item_again.generic.y			= 480-64;
	postgameMenuInfo.item_again.generic.callback	= UI_SPPostgameMenu_AgainEvent;
	postgameMenuInfo.item_again.generic.id			= ID_AGAIN;
	postgameMenuInfo.item_again.width				= 128;
	postgameMenuInfo.item_again.height				= 64;
	postgameMenuInfo.item_again.focuspic			= ART_REPLAY1;

	postgameMenuInfo.item_next.generic.type			= MTYPE_BITMAP;
	postgameMenuInfo.item_next.generic.name			= ART_NEXT0;
#ifdef TA_SP
	postgameMenuInfo.item_next.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
#else
	postgameMenuInfo.item_next.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_INACTIVE;
#endif
	postgameMenuInfo.item_next.generic.x			= 640;
	postgameMenuInfo.item_next.generic.y			= 480-64;
	postgameMenuInfo.item_next.generic.callback		= UI_SPPostgameMenu_NextEvent;
	postgameMenuInfo.item_next.generic.id			= ID_NEXT;
	postgameMenuInfo.item_next.width				= 128;
	postgameMenuInfo.item_next.height				= 64;
	postgameMenuInfo.item_next.focuspic				= ART_NEXT1;

#ifdef TA_SP
	Menu_AddItem( &postgameMenuInfo.menu, ( void * )&postgameMenuInfo.art_frame );
	Menu_AddItem( &postgameMenuInfo.menu, ( void * )&postgameMenuInfo.item_name );
#endif
	Menu_AddItem( &postgameMenuInfo.menu, ( void * )&postgameMenuInfo.item_menu );
	Menu_AddItem( &postgameMenuInfo.menu, ( void * )&postgameMenuInfo.item_again );
	Menu_AddItem( &postgameMenuInfo.menu, ( void * )&postgameMenuInfo.item_next );

#ifdef TA_SP
	trap_Cvar_VariableStringBuffer("ui_arcadeName", postgameMenuInfo.item_name.field.buffer, postgameMenuInfo.item_name.field.maxchars);
#endif
}


#ifdef TA_SP
// Returns score pos, or -1 if didn't get high enough score to save it.
int UI_AddArcadeScore(arcadeGameData_t *gamedata, arcadeScore_t *score)
{
	int index, i;

	// Find where to put score
	for (index = 0; index < NUM_ARCADE_SCORES; ++index) {
		if (!gamedata->scores[index].time || score->score > gamedata->scores[index].score
			|| (score->score == gamedata->scores[index].score && score->time < gamedata->scores[index].time)) {
			// Empty score, higher score or same score and faster
			break;
		}
	}

	// Not a new high score
	if (index == NUM_ARCADE_SCORES) {
		return -1;
	}

	// Move slots to make room for new score
	for (i = NUM_ARCADE_SCORES-2; i >= index; --i) {
		memcpy(&gamedata->scores[i+1], &gamedata->scores[i], sizeof (arcadeScore_t));
	}

	// Add new score
	memcpy(&gamedata->scores[index], score, sizeof (arcadeScore_t));

	return index;
}

/*
=================
UI_GamedataFilename
=================
*/
static void UI_GamedataFilename(char *fileName, int length) {
	static char *gametypeNames[] = {"ffa", "tourney", "single", "team", "ctf", "oneflag", "overload", "harvester"};
	char		map[MAX_QPATH];
	char		info[MAX_INFO_STRING];

	trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) );
	Q_strncpyz( map, Info_ValueForKey( info, "mapname" ), sizeof(map) );
	postgameMenuInfo.gametype = Com_Clamp(0, ARRAY_LEN(gametypeNames), atoi(Info_ValueForKey(info, "g_gametype")));

	// compose file name
	Com_sprintf(fileName, length, "scores/%s_%s.score", map, gametypeNames[postgameMenuInfo.gametype]);
}

/*
=================
UI_SavePostGameStats
=================
*/
void UI_SavePostGameStats(const char *name) {
	char		fileName[MAX_QPATH];
	fileHandle_t f;

	if (postgameMenuInfo.scoreIndex == -1) {
		return;
	}

	// Set name in score data.
	Q_strncpyz(postgameMenuInfo.gamedata.scores[postgameMenuInfo.scoreIndex].name, name, STRARRAY_LEN(postgameMenuInfo.gamedata.scores[postgameMenuInfo.scoreIndex].name));

	// Write updated gamedata
	UI_GamedataFilename(fileName, sizeof(fileName));
	if (trap_FS_FOpenFile(fileName, &f, FS_WRITE) >= 0) {
		trap_FS_Write(&postgameMenuInfo.gamedata, sizeof(arcadeGameData_t), f);
		trap_FS_FCloseFile(f);
	}
}

/*
=================
UI_CalcPostGameStats

Setup new score data.
=================
*/
static void UI_CalcPostGameStats(void) {
	int					time, redScore, blueScore;
	char				fileName[MAX_QPATH];
	fileHandle_t		f;
	qboolean			validData;
	arcadeGameData_t	*gamedata;
	arcadeScore_t		*newScore;

	gamedata = &postgameMenuInfo.gamedata;
	newScore = &postgameMenuInfo.newScore;

	// Load game data if it exists
	validData = qfalse;
	UI_GamedataFilename(fileName, sizeof(fileName));
	if (trap_FS_FOpenFile(fileName, &f, FS_READ) >= 0) {
		trap_FS_Read(gamedata, sizeof(arcadeGameData_t), f);
		trap_FS_FCloseFile(f);

		if (!Q_strncmp(gamedata->magic, ARCADE_GAMEDATA_MAGIC, ARRAY_LEN(gamedata->magic))) {
			if (gamedata->version == ARCADE_GAMEDATA_VERSION) {
				validData = qtrue;
			}
		}
	}

	if (!validData) {
		memset(gamedata, 0, sizeof(arcadeGameData_t));
	}

	// Setup gamedata
	memcpy(gamedata->magic, ARCADE_GAMEDATA_MAGIC, ARRAY_LEN(gamedata->magic));
	gamedata->version = ARCADE_GAMEDATA_VERSION;
	
	// Setup new score line
	time = (atoi(UI_Argv(1)) - trap_Cvar_VariableValue("ui_matchStartTime")) / 1000;
	redScore = atoi(UI_Argv(2));
	blueScore = atoi(UI_Argv(3));

	// Setup newScore, name is set in UI_SavePostGameStats.
	Q_strncpyz(newScore->name, "", STRARRAY_LEN(newScore->name));
	Q_strncpyz(newScore->character, UI_Argv(4), STRARRAY_LEN(newScore->character));
	newScore->score = atoi(UI_Argv(5));
	newScore->time = time;

	// CTF
	newScore->captures = atoi(UI_Argv(6));
	newScore->redScore = redScore;
	newScore->blueScore = blueScore;

	// Add the score
	postgameMenuInfo.scoreIndex = UI_AddArcadeScore(gamedata, newScore);
	//trap_Cvar_Set("ui_scoreIndex", va("%d", postgameMenuInfo.scoreIndex));

	// If user didn't place high enough, don't need to ask for their name.
	if (postgameMenuInfo.scoreIndex == -1) {
		UI_SPPostgameMenu_NextEvent(NULL, QM_ACTIVATED);
	}
}
#else
static void Prepname( int index ) {
	int		len;
	char	name[64];
	char	info[MAX_INFO_STRING];

	trap_GetConfigString( CS_PLAYERS + postgameMenuInfo.clientNums[index], info, MAX_INFO_STRING );
	Q_strncpyz( name, Info_ValueForKey( info, "n" ), sizeof(name) );
	Q_CleanStr( name );
	len = strlen( name );

	while( len && UI_ProportionalStringWidth( name, 0 ) > 256 ) {
		len--;
		name[len] = 0;
	}

	Q_strncpyz( postgameMenuInfo.placeNames[index], name, sizeof(postgameMenuInfo.placeNames[index]) );
}
#endif


/*
=================
UI_SPPostgameMenu_f
=================
*/
void UI_SPPostgameMenu_f( void ) {
#ifndef TA_SP
	int			playerGameRank;
	int			playerClientNum;
	int			n;
	int			oldFrags, newFrags;
	const char	*arena;
	int			awardValues[MAX_UI_AWARDS];
	char		map[MAX_QPATH];
#endif
	char		info[MAX_INFO_STRING];

	memset( &postgameMenuInfo, 0, sizeof(postgameMenuInfo) );

	trap_GetConfigString( CS_SYSTEMINFO, info, sizeof(info) );
	postgameMenuInfo.serverId = atoi( Info_ValueForKey( info, "sv_serverid" ) );

#ifndef TA_SP // ARCADE_SCORE
	trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) );
	Q_strncpyz( map, Info_ValueForKey( info, "mapname" ), sizeof(map) );
	arena = UI_GetArenaInfoByMap( map );
	if ( !arena ) {
		return;
	}
	Q_strncpyz( arenainfo, arena, sizeof(arenainfo) );

	postgameMenuInfo.level = atoi( Info_ValueForKey( arenainfo, "num" ) );

	postgameMenuInfo.numClients = atoi( UI_Argv( 1 ) );
	playerClientNum = atoi( UI_Argv( 2 ) );
	playerGameRank = 8;		// in case they ended game as a spectator

	if( postgameMenuInfo.numClients > MAX_SCOREBOARD_CLIENTS ) {
		postgameMenuInfo.numClients = MAX_SCOREBOARD_CLIENTS;
	}

	for( n = 0; n < postgameMenuInfo.numClients; n++ ) {
		postgameMenuInfo.clientNums[n] = atoi( UI_Argv( 8 + n * 3 + 1 ) );
		postgameMenuInfo.ranks[n] = atoi( UI_Argv( 8 + n * 3 + 2 ) );
		postgameMenuInfo.scores[n] = atoi( UI_Argv( 8 + n * 3 + 3 ) );

		if( postgameMenuInfo.clientNums[n] == playerClientNum ) {
			playerGameRank = (postgameMenuInfo.ranks[n] & ~RANK_TIED_FLAG) + 1;
		}
	}

	UI_SetBestScore( postgameMenuInfo.level, playerGameRank );

	// process award stats and prepare presentation data
	awardValues[AWARD_ACCURACY] = atoi( UI_Argv( 3 ) );
#ifdef TURTLEARENA // AWARDS
	awardValues[AWARD_FRAGS] = atoi( UI_Argv( 4 ) );
	awardValues[AWARD_PERFECT] = atoi( UI_Argv( 5 ) );
#else
	awardValues[AWARD_IMPRESSIVE] = atoi( UI_Argv( 4 ) );
	awardValues[AWARD_EXCELLENT] = atoi( UI_Argv( 5 ) );
	awardValues[AWARD_GAUNTLET] = atoi( UI_Argv( 6 ) );
	awardValues[AWARD_FRAGS] = atoi( UI_Argv( 7 ) );
	awardValues[AWARD_PERFECT] = atoi( UI_Argv( 8 ) );
#endif

	postgameMenuInfo.numAwards = 0;

	if( awardValues[AWARD_ACCURACY] >= 50 ) {
		UI_LogAwardData( AWARD_ACCURACY, 1 );
		postgameMenuInfo.awardsEarned[postgameMenuInfo.numAwards] = AWARD_ACCURACY;
		postgameMenuInfo.awardsLevels[postgameMenuInfo.numAwards] = awardValues[AWARD_ACCURACY];
		postgameMenuInfo.numAwards++;
	}

#ifndef TURTLEARENA // AWARDS
	if( awardValues[AWARD_IMPRESSIVE] ) {
		UI_LogAwardData( AWARD_IMPRESSIVE, awardValues[AWARD_IMPRESSIVE] );
		postgameMenuInfo.awardsEarned[postgameMenuInfo.numAwards] = AWARD_IMPRESSIVE;
		postgameMenuInfo.awardsLevels[postgameMenuInfo.numAwards] = awardValues[AWARD_IMPRESSIVE];
		postgameMenuInfo.numAwards++;
	}

	if( awardValues[AWARD_EXCELLENT] ) {
		UI_LogAwardData( AWARD_EXCELLENT, awardValues[AWARD_EXCELLENT] );
		postgameMenuInfo.awardsEarned[postgameMenuInfo.numAwards] = AWARD_EXCELLENT;
		postgameMenuInfo.awardsLevels[postgameMenuInfo.numAwards] = awardValues[AWARD_EXCELLENT];
		postgameMenuInfo.numAwards++;
	}

	if( awardValues[AWARD_GAUNTLET] ) {
		UI_LogAwardData( AWARD_GAUNTLET, awardValues[AWARD_GAUNTLET] );
		postgameMenuInfo.awardsEarned[postgameMenuInfo.numAwards] = AWARD_GAUNTLET;
		postgameMenuInfo.awardsLevels[postgameMenuInfo.numAwards] = awardValues[AWARD_GAUNTLET];
		postgameMenuInfo.numAwards++;
	}
#endif

	oldFrags = UI_GetAwardLevel( AWARD_FRAGS ) / 100;
	UI_LogAwardData( AWARD_FRAGS, awardValues[AWARD_FRAGS] );
	newFrags = UI_GetAwardLevel( AWARD_FRAGS ) / 100;
	if( newFrags > oldFrags ) {
		postgameMenuInfo.awardsEarned[postgameMenuInfo.numAwards] = AWARD_FRAGS;
		postgameMenuInfo.awardsLevels[postgameMenuInfo.numAwards] = newFrags * 100;
		postgameMenuInfo.numAwards++;
	}

	if( awardValues[AWARD_PERFECT] ) {
		UI_LogAwardData( AWARD_PERFECT, 1 );
		postgameMenuInfo.awardsEarned[postgameMenuInfo.numAwards] = AWARD_PERFECT;
		postgameMenuInfo.awardsLevels[postgameMenuInfo.numAwards] = 1;
		postgameMenuInfo.numAwards++;
	}

	if ( playerGameRank == 1 ) {
		postgameMenuInfo.won = UI_TierCompleted( postgameMenuInfo.level );
	}
	else {
		postgameMenuInfo.won = -1;
	}

	postgameMenuInfo.starttime = uis.realtime;
	postgameMenuInfo.scoreboardtime = uis.realtime;
#endif

	trap_Key_SetCatcher( KEYCATCH_UI );
	uis.menusp = 0;

	UI_SPPostgameMenu_Init();
	UI_PushMenu( &postgameMenuInfo.menu );

#ifdef TA_SP
	Menu_SetCursorToItem( &postgameMenuInfo.menu, &postgameMenuInfo.item_name );

	trap_Cmd_ExecuteText( EXEC_APPEND, "music music/postgame\n" );

	postgameMenuInfo.phase = 1;

	UI_CalcPostGameStats();
#else
	if ( playerGameRank == 1 ) {
		Menu_SetCursorToItem( &postgameMenuInfo.menu, &postgameMenuInfo.item_next );
	}
	else {
		Menu_SetCursorToItem( &postgameMenuInfo.menu, &postgameMenuInfo.item_again );
	}

	Prepname( 0 );
	Prepname( 1 );
	Prepname( 2 );

	if ( playerGameRank != 1 ) {
		postgameMenuInfo.winnerSound = trap_S_RegisterSound( va( "sound/player/announce/%s_wins.wav", postgameMenuInfo.placeNames[0] ), qfalse );
		trap_Cmd_ExecuteText( EXEC_APPEND, "music music/loss\n" );
	}
	else {
		postgameMenuInfo.winnerSound = trap_S_RegisterSound( "sound/player/announce/youwin.wav", qfalse );
		trap_Cmd_ExecuteText( EXEC_APPEND, "music music/win\n" );
	}

	postgameMenuInfo.phase = 1;

	postgameMenuInfo.lastTier = UI_GetNumSPTiers();
	if ( UI_GetSpecialArenaInfo( "final" ) ) {
		postgameMenuInfo.lastTier++;
	}
#endif
}
