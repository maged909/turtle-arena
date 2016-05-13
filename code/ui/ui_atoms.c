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
/**********************************************************************
	UI_ATOMS.C

	User interface building blocks and support functions.
**********************************************************************/
#include "ui_local.h"

#ifdef TA_SP
void UI_SetBestScores(arcadeScore_t *newInfo, qboolean postGame) {
	trap_Cvar_Set("ui_scoreName", 			newInfo->name);
	trap_Cvar_Set("ui_scoreCharacter", 		newInfo->character);
	trap_Cvar_Set("ui_scoreScore", 			va("%i", newInfo->score));
	trap_Cvar_Set("ui_scoreTeam",			va("%i to %i", newInfo->redScore, newInfo->blueScore));
	trap_Cvar_Set("ui_scoreTime",			va("%02i:%02i", newInfo->time / 60, newInfo->time % 60));
	trap_Cvar_Set("ui_scoreCaptures",		va("%i", newInfo->captures));
	if (postGame) {
		trap_Cvar_Set("ui_scoreName2", 			newInfo->name);
		trap_Cvar_Set("ui_scoreCharacter2", 	newInfo->character);
		trap_Cvar_Set("ui_scoreScore2", 		va("%i", newInfo->score));
		trap_Cvar_Set("ui_scoreTeam2",			va("%i to %i", newInfo->redScore, newInfo->blueScore));
		trap_Cvar_Set("ui_scoreTime2",			va("%02i:%02i", newInfo->time / 60, newInfo->time % 60));
		trap_Cvar_Set("ui_scoreCaptures2",		va("%i", newInfo->captures));
	}
}
#else
void UI_SetBestScores(postGameInfo_t *newInfo, qboolean postGame) {
	trap_Cvar_Set("ui_scoreAccuracy",				va("%i%%", newInfo->accuracy));
#ifndef TURTLEARENA // AWARDS
	trap_Cvar_SetValue("ui_scoreImpressives",	newInfo->impressives);
	trap_Cvar_SetValue("ui_scoreExcellents", 	newInfo->excellents);
#endif
	trap_Cvar_SetValue("ui_scoreDefends", 		newInfo->defends);
	trap_Cvar_SetValue("ui_scoreAssists", 		newInfo->assists);
#ifndef TURTLEARENA // AWARDS
	trap_Cvar_SetValue("ui_scoreGauntlets", 	newInfo->gauntlets);
#endif
	trap_Cvar_SetValue("ui_scoreScore", 		newInfo->score);
	trap_Cvar_SetValue("ui_scorePerfect",	 	newInfo->perfects);
	trap_Cvar_Set("ui_scoreTeam",					va("%i to %i", newInfo->redScore, newInfo->blueScore));
	trap_Cvar_SetValue("ui_scoreBase",			newInfo->baseScore);
	trap_Cvar_SetValue("ui_scoreTimeBonus",		newInfo->timeBonus);
	trap_Cvar_SetValue("ui_scoreSkillBonus",	newInfo->skillBonus);
	trap_Cvar_SetValue("ui_scoreShutoutBonus",	newInfo->shutoutBonus);
	trap_Cvar_Set("ui_scoreTime",					va("%02i:%02i", newInfo->time / 60, newInfo->time % 60));
	trap_Cvar_SetValue("ui_scoreCaptures",		newInfo->captures);
  if (postGame) {
		trap_Cvar_Set("ui_scoreAccuracy2",				va("%i%%", newInfo->accuracy));
#ifndef TURTLEARENA // AWARDS
		trap_Cvar_SetValue("ui_scoreImpressives2",	newInfo->impressives);
		trap_Cvar_SetValue("ui_scoreExcellents2",	newInfo->excellents);
#endif
		trap_Cvar_SetValue("ui_scoreDefends2",		newInfo->defends);
		trap_Cvar_SetValue("ui_scoreAssists2",		newInfo->assists);
#ifndef TURTLEARENA // AWARDS
		trap_Cvar_SetValue("ui_scoreGauntlets2",	newInfo->gauntlets);
#endif
		trap_Cvar_SetValue("ui_scoreScore2",		newInfo->score);
		trap_Cvar_SetValue("ui_scorePerfect2",		newInfo->perfects);
		trap_Cvar_Set("ui_scoreTeam2",					va("%i to %i", newInfo->redScore, newInfo->blueScore));
		trap_Cvar_SetValue("ui_scoreBase2",			newInfo->baseScore);
		trap_Cvar_SetValue("ui_scoreTimeBonus2",	newInfo->timeBonus);
		trap_Cvar_SetValue("ui_scoreSkillBonus2",	newInfo->skillBonus);
		trap_Cvar_SetValue("ui_scoreShutoutBonus2",	newInfo->shutoutBonus);
		trap_Cvar_Set("ui_scoreTime2",					va("%02i:%02i", newInfo->time / 60, newInfo->time % 60));
		trap_Cvar_SetValue("ui_scoreCaptures2",		newInfo->captures);
	}
}
#endif

void UI_LoadBestScores(const char *map, int game)
{
	char		fileName[MAX_QPATH];
	fileHandle_t f;
#ifdef TA_SP
	static char *gametypeNames[] = {"ffa", "tourney", "single", "team", "ctf", "oneflag", "overload", "harvester"};
	arcadeGameData_t gamedata;
	qboolean	validData;
#else
	postGameInfo_t newInfo;
#endif
	
#ifdef TA_SP
	// compose file name
	Com_sprintf(fileName, MAX_QPATH, "scores/%s_%s.score", map, gametypeNames[game]);
	// see if we have one already
	validData = qfalse;
	if (trap_FS_FOpenFile(fileName, &f, FS_READ) >= 0) {
		trap_FS_Read(&gamedata, sizeof(arcadeGameData_t), f);
		trap_FS_FCloseFile(f);

		if (!Q_strncmp(gamedata.magic, ARCADE_GAMEDATA_MAGIC, ARRAY_LEN(gamedata.magic)))
		{
			if (gamedata.version == ARCADE_GAMEDATA_VERSION) {
				validData = qtrue;
			}
		}
	}

	if (!validData) {
		memset(&gamedata, 0, sizeof(arcadeGameData_t));
	}

	UI_SetBestScores(&gamedata.scores[0], qfalse);
#else
	memset(&newInfo, 0, sizeof(postGameInfo_t));
	Com_sprintf(fileName, MAX_QPATH, "games/%s_%i.game", map, game);
	if (trap_FS_FOpenFile(fileName, &f, FS_READ) >= 0) {
		int size = 0;
		trap_FS_Read(&size, sizeof(int), f);
		if (size == sizeof(postGameInfo_t)) {
			trap_FS_Read(&newInfo, sizeof(postGameInfo_t), f);
		}
		trap_FS_FCloseFile(f);
	}
	UI_SetBestScores(&newInfo, qfalse);
#endif

	uiInfo.demoAvailable = qfalse;

	Com_sprintf(fileName, MAX_QPATH, "%s_%i", map, game);
	if(trap_GetDemoFileInfo(fileName, NULL, NULL, NULL, NULL, NULL) != 0)
	{
		uiInfo.demoAvailable = qtrue;
		trap_FS_FCloseFile(f);
	}
}

/*
===============
UI_ClearScores
===============
*/
void UI_ClearScores(void) {
	char	gameList[4096];
	char *gameFile;
	int		i, len, count, size;
	fileHandle_t f;
#ifdef TA_SP
	arcadeGameData_t newInfo;

	count = trap_FS_GetFileList( "scores", "score", gameList, sizeof(gameList) );

	size = sizeof(arcadeGameData_t);
#else
	postGameInfo_t newInfo;

	count = trap_FS_GetFileList( "games", "game", gameList, sizeof(gameList) );

	size = sizeof(postGameInfo_t);
#endif
	memset(&newInfo, 0, size);

	if (count > 0) {
		gameFile = gameList;
		for ( i = 0; i < count; i++ ) {
			len = strlen(gameFile);
#ifdef TA_SP
			if (trap_FS_FOpenFile(va("scores/%s",gameFile), &f, FS_WRITE) >= 0)
#else
			if (trap_FS_FOpenFile(va("games/%s",gameFile), &f, FS_WRITE) >= 0)
#endif
			{
				trap_FS_Write(&size, sizeof(int), f);
				trap_FS_Write(&newInfo, size, f);
				trap_FS_FCloseFile(f);
			}
			gameFile += len + 1;
		}
	}
	
#ifdef TA_SP
	UI_SetBestScores(&newInfo.scores[0], qfalse);
#else
	UI_SetBestScores(&newInfo, qfalse);
#endif

}



static void	UI_Cache_f( void ) {
	Display_CacheAll();
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
#endif

/*
=======================
UI_CalcPostGameStats
=======================
*/
static void UI_CalcPostGameStats( void ) {
#ifdef TA_SP
	static char *gametypeNames[] = {"ffa", "tourney", "single", "team", "ctf", "oneflag", "overload", "harvester"};
	char		map[MAX_QPATH];
	char		fileName[MAX_QPATH];
	char		info[MAX_INFO_STRING];
	char		name[9];
	fileHandle_t f;
	qboolean	validData;
	int			time, redScore, blueScore;
	arcadeGameData_t *gamedata;
	arcadeScore_t *newScore;
	arcadeGameData_t uiGamedata;
	arcadeScore_t uiNewScore;
	int scoreIndex;
	int gametype;

	gamedata = &uiGamedata;
	newScore = &uiNewScore;

	trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) );
	Q_strncpyz( map, Info_ValueForKey( info, "mapname" ), sizeof(map) );
	gametype = Com_Clamp(0, ARRAY_LEN(gametypeNames), atoi(Info_ValueForKey(info, "g_gametype")));

	// compose file name
	Com_sprintf(fileName, MAX_QPATH, "scores/%s_%s.score", map, gametypeNames[gametype]);
	// see if we have one already
	validData = qfalse;
	if (trap_FS_FOpenFile(fileName, &f, FS_READ) >= 0) {
		trap_FS_Read(gamedata, sizeof(arcadeGameData_t), f);
		trap_FS_FCloseFile(f);

		if (!Q_strncmp(gamedata->magic, ARCADE_GAMEDATA_MAGIC, ARRAY_LEN(gamedata->magic)))
		{
			if (gamedata->version == ARCADE_GAMEDATA_VERSION) {
				validData = qtrue;
			}
		}
	}

	if (!validData) {
		memset(gamedata, 0, sizeof(arcadeGameData_t));
	}

	// Get name
	// TODO: Ask user to enter name
	trap_Cvar_VariableStringBuffer("name", name, sizeof (name));

	// Setup gamedata
	memcpy(gamedata->magic, ARCADE_GAMEDATA_MAGIC, ARRAY_LEN(gamedata->magic));
	gamedata->version = ARCADE_GAMEDATA_VERSION;

	time = (atoi(CG_Argv(1)) - trap_Cvar_VariableValue("ui_matchStartTime")) / 1000;
	redScore = atoi(CG_Argv(2));
	blueScore = atoi(CG_Argv(3));

	// Setup newScore
	Q_strncpyz(newScore->name, name, STRARRAY_LEN(newScore->name));
	Q_strncpyz(newScore->character, CG_Argv(4), STRARRAY_LEN(newScore->character));
	newScore->score = atoi(CG_Argv(5));
	newScore->time = time;

	// CTF
	newScore->captures = atoi(CG_Argv(6));
	newScore->redScore = redScore;
	newScore->blueScore = blueScore;

	// Add the score
	scoreIndex = UI_AddArcadeScore(gamedata, newScore);
	trap_Cvar_Set("ui_scoreIndex", va("%d", scoreIndex));

	// Write updated gamedata
	if (trap_FS_FOpenFile(fileName, &f, FS_WRITE) >= 0) {
		trap_FS_Write(gamedata, sizeof(arcadeGameData_t), f);
		trap_FS_FCloseFile(f);
	}
#else
	char		map[MAX_QPATH];
	char		fileName[MAX_QPATH];
	char		info[MAX_INFO_STRING];
	fileHandle_t f;
	int size, game, time, adjustedTime;
	postGameInfo_t oldInfo;
	postGameInfo_t newInfo;
	qboolean newHigh = qfalse;

	trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) );
	Q_strncpyz( map, Info_ValueForKey( info, "mapname" ), sizeof(map) );
	game = atoi(Info_ValueForKey(info, "g_gametype"));

	// compose file name
	Com_sprintf(fileName, MAX_QPATH, "games/%s_%i.game", map, game);
	// see if we have one already
	memset(&oldInfo, 0, sizeof(postGameInfo_t));
	if (trap_FS_FOpenFile(fileName, &f, FS_READ) >= 0) {
	// if so load it
		size = 0;
		trap_FS_Read(&size, sizeof(int), f);
		if (size == sizeof(postGameInfo_t)) {
			trap_FS_Read(&oldInfo, sizeof(postGameInfo_t), f);
		}
		trap_FS_FCloseFile(f);
	}					 

	newInfo.accuracy = atoi(CG_Argv(3));
#ifdef TURTLEARENA // AWARDS
	newInfo.baseScore = atoi(CG_Argv(4));
	newInfo.perfects = atoi(CG_Argv(5));

	// Extra data for missionpack
	newInfo.defends = atoi(CG_Argv(6));
	newInfo.assists = atoi(CG_Argv(7));
	newInfo.redScore = atoi(CG_Argv(8));
	newInfo.blueScore = atoi(CG_Argv(9));
	time = atoi(CG_Argv(10));
	newInfo.captures = atoi(CG_Argv(11));
#else
	newInfo.accuracy = atoi(CG_Argv(3));
	newInfo.impressives = atoi(CG_Argv(4));
	newInfo.excellents = atoi(CG_Argv(5));
	newInfo.defends = atoi(CG_Argv(6));
	newInfo.assists = atoi(CG_Argv(7));
	newInfo.gauntlets = atoi(CG_Argv(8));
	newInfo.baseScore = atoi(CG_Argv(9));
	newInfo.perfects = atoi(CG_Argv(10));
	newInfo.redScore = atoi(CG_Argv(11));
	newInfo.blueScore = atoi(CG_Argv(12));
	time = atoi(CG_Argv(13));
	newInfo.captures = atoi(CG_Argv(14));
#endif

	newInfo.time = (time - trap_Cvar_VariableValue("ui_matchStartTime")) / 1000;
	adjustedTime = uiInfo.mapList[ui_currentMap.integer].timeToBeat[game];
	if (newInfo.time < adjustedTime) { 
		newInfo.timeBonus = (adjustedTime - newInfo.time) * 10;
	} else {
		newInfo.timeBonus = 0;
	}

	if (newInfo.redScore > newInfo.blueScore && newInfo.blueScore <= 0) {
		newInfo.shutoutBonus = 100;
	} else {
		newInfo.shutoutBonus = 0;
	}

	newInfo.skillBonus = trap_Cvar_VariableValue("g_spSkill");
	if (newInfo.skillBonus <= 0) {
		newInfo.skillBonus = 1;
	}
	newInfo.score = newInfo.baseScore + newInfo.shutoutBonus + newInfo.timeBonus;
	newInfo.score *= newInfo.skillBonus;

	// see if the score is higher for this one
	newHigh = (newInfo.redScore > newInfo.blueScore && newInfo.score > oldInfo.score);

	if  (newHigh) {
		// if so write out the new one
		uiInfo.newHighScoreTime = uiInfo.uiDC.realTime + 20000;
		if (trap_FS_FOpenFile(fileName, &f, FS_WRITE) >= 0) {
			size = sizeof(postGameInfo_t);
			trap_FS_Write(&size, sizeof(int), f);
			trap_FS_Write(&newInfo, sizeof(postGameInfo_t), f);
			trap_FS_FCloseFile(f);
		}
	}

	if (newInfo.time < oldInfo.time) {
		uiInfo.newBestTime = uiInfo.uiDC.realTime + 20000;
	}
 #endif
 
	// put back all the ui overrides
	trap_Cvar_SetValue("capturelimit", trap_Cvar_VariableValue("ui_saveCaptureLimit"));
#ifdef NOTRATEDM // frag to score
	trap_Cvar_SetValue("scorelimit", trap_Cvar_VariableValue("ui_saveScoreLimit"));
#else
	trap_Cvar_SetValue("fraglimit", trap_Cvar_VariableValue("ui_saveFragLimit"));
#endif
	trap_Cvar_SetValue("cg_drawTimer", trap_Cvar_VariableValue("ui_drawTimer"));
	trap_Cvar_SetValue("g_doWarmup", trap_Cvar_VariableValue("ui_doWarmup"));
	trap_Cvar_SetValue("g_Warmup", trap_Cvar_VariableValue("ui_Warmup"));
	trap_Cvar_SetValue("sv_pure", trap_Cvar_VariableValue("ui_pure"));
	trap_Cvar_SetValue("g_friendlyFire", trap_Cvar_VariableValue("ui_friendlyFire"));

#ifdef TA_SP
	UI_SetBestScores(&gamedata->scores[0], qtrue);
	UI_ShowPostGame(scoreIndex == 0);
#else
	UI_SetBestScores(&newInfo, qtrue);
	UI_ShowPostGame(newHigh);
#endif


}

static void UI_Test_f( void ) {
	UI_ShowPostGame(qtrue);
}

static void UI_Load_f( void ) {
#ifdef MISSIONPACK_HUD
	if ( cg.connected ) {
		// if hud scripts are loaded, UI_Load() will break it. So reload hud and ui using loadhud command.
		trap_Cmd_ExecuteText( EXEC_NOW, "loadhud\n" );
		return;
	}
#endif
	UI_Load();
}

consoleCommand_t	ui_commands[] = {
	{ "postgame", UI_CalcPostGameStats, CMD_INGAME },
	{ "ui_cache", UI_Cache_f, 0 },
	{ "ui_load", UI_Load_f, 0 },
	{ "ui_report", UI_Report, 0 },
	{ "ui_test", UI_Test_f, CMD_INGAME }
};

int ui_numCommands = ARRAY_LEN( ui_commands );

/*
=================
UI_ConsoleCommand

update frame time, commands are executed by CG_ConsoleCommand
=================
*/
void UI_ConsoleCommand( int realTime ) {
	uiInfo.uiDC.frameTime = realTime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realTime;
}

qboolean UI_CursorInRect (int x, int y, int width, int height)
{
	if (uiInfo.uiDC.cursorx < x ||
		uiInfo.uiDC.cursory < y ||
		uiInfo.uiDC.cursorx > x+width ||
		uiInfo.uiDC.cursory > y+height)
		return qfalse;

	return qtrue;
}
