/*
===========================================================================
Copyright (C) 2009-2010 Zack Middleton

This file is part of Turtle Arena source code.

Turtle Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Turtle Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Turtle Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

//#include "../qcommon/q_shared.h"
#include "g_local.h"

#ifdef TA_SP // Save/load

// NOTE: Make sure BG_SAVE_VERSIONS stays up to date
//         with current save code.

#define	SAVE_VERSION 7 // current version of save/load routines

typedef struct
{
	byte localPlayerNum;
	char model[MAX_QPATH];
	char headModel[MAX_QPATH];
	byte holdable[MAX_HOLDABLE];
	byte holdableSelected;
	int score;
	byte lives;
	byte continues;
} save_player_t;

typedef struct
{
	// server expects these in this order!
	//
    byte version; // Save file version.
    char mapname[MAX_QPATH];
    byte maxplayers;
    byte localPlayers;

	// game only, server doesn't read these.
	//
    byte skill;
    save_player_t players[MAX_SPLITVIEW];

} save_t;

save_t loadData;
qboolean savegameLoaded = qfalse;

extern vmCvar_t g_spSkill;

/*
============
G_SaveGame
============
*/
qboolean G_SaveGame(const char *savegame)
{
	char filename[MAX_QPATH];
	fileHandle_t f;
	save_t saveData;
	int player;
	int i;
	int j;

	if (!g_singlePlayer.integer || g_gametype.integer != GT_SINGLE_PLAYER) {
		G_Printf("Can't savegame, saving is for single player only!\n");
		return qfalse;
	}

	if (!savegame || !*savegame) {
		G_Printf("G_SaveGame: No save name.\n");
		return qfalse;
	}

	// Setup header
    memset(&saveData, 0, sizeof (save_t));
	saveData.version = SAVE_VERSION;

	// If savemap is set to the name of the next map we can save before level change
	trap_Cvar_VariableStringBuffer( "g_saveMapname", saveData.mapname, MAX_QPATH );
	trap_Cvar_Set("g_saveMapname", "");

	if (!saveData.mapname[0]) {
		if (!g_cheats.integer) {
			// Most likely a user trying to save, which can be used to cheat.
			G_Printf("The game autosaves, complete the level to save.\n");
			return qfalse;
		}

		trap_Cvar_VariableStringBuffer( "mapname", saveData.mapname, MAX_QPATH );
	}

	saveData.skill = g_spSkill.integer;
	saveData.maxplayers = level.maxplayers;

	// Bits get added are each local player in savegame.
	saveData.localPlayers = 0;

	player = 0;
	for (i = 0; i < level.maxplayers; i++)
	{
		if (g_entities[i].r.svFlags & SVF_BOT)
			continue;

		if (level.players[i].pers.connected != CON_CONNECTED)
			continue;

		// Don't save dead (Game Over) clients.
		if (level.players[i].ps.persistant[PERS_LIVES] < 1 && level.players[i].ps.persistant[PERS_CONTINUES] < 1)
			continue;

		saveData.players[player].localPlayerNum = level.players[i].pers.localPlayerNum;
		saveData.localPlayers |= (1<<saveData.players[player].localPlayerNum);

		// model/skin
		Q_strncpyz(saveData.players[player].model, level.players[i].pers.playercfg.model, MAX_QPATH);

		// headmodel/skin
		Q_strncpyz(saveData.players[player].headModel, level.players[i].pers.playercfg.headModel, MAX_QPATH);

		// holdable items
		for (j = 0; j < MAX_HOLDABLE; j++) {
			saveData.players[player].holdable[j] = level.players[i].ps.holdable[j];
		}
		saveData.players[player].holdableSelected = level.players[i].ps.holdableIndex;
		saveData.players[player].score = level.players[i].ps.persistant[PERS_SCORE];
		saveData.players[player].lives = level.players[i].ps.persistant[PERS_LIVES];
		saveData.players[player].continues = level.players[i].ps.persistant[PERS_CONTINUES];
		player++;
		if (player >= MAX_SPLITVIEW) {
			break;
		}
	}

	for ( ; player < MAX_SPLITVIEW; player++) {
		// Unused slots
		saveData.players[player].localPlayerNum = 0xff;
	}

	// Open savefile
	Com_sprintf( filename, MAX_QPATH, "saves/%s.sav", savegame );

	trap_FS_FOpenFile( filename, &f, FS_WRITE );
	
	if (!f) {
		G_Printf("WARNING: Failed to write savefile.\n");
		return qfalse;
	}

	// Write saveData
	trap_FS_Write(&saveData, sizeof (save_t), f);

	trap_FS_FCloseFile(f);

	return qtrue;
}

/*
============
G_LoadGame

Called after level is loaded.
============
*/
void G_LoadGame(void)
{
	int				len;
	fileHandle_t	f;

	len = trap_FS_FOpenFile( g_savegameFilename.string, &f, FS_READ );

	trap_Cvar_Set( "savegame_loading", "0" );
	trap_Cvar_Set( "savegame_filename", "" );

	if (len == -1) {
		trap_FS_FCloseFile(f);
		G_Error("Savegame file not found!");
	}

	// Read data
	trap_FS_Read(&loadData, sizeof (save_t), f);

	trap_FS_FCloseFile(f);

	// loadgame command should check but just in case...
	if (loadData.version != SAVE_VERSION) {
		savegameLoaded = qfalse;
		G_Error( "Unsupported savegame version, %i\n", loadData.version);
        return;
	}

	// Update cvars right away!
	if (g_gametype.integer != GT_SINGLE_PLAYER) {
		g_gametype.integer = GT_SINGLE_PLAYER;
		trap_Cvar_Set( "g_gametype", va("%d", g_gametype.integer) );
	}

	if (!g_singlePlayer.integer) {
		g_singlePlayer.integer = 1;
		trap_Cvar_Set( "ui_singlePlayerActive", va("%d", g_singlePlayer.integer) );
	}

    if (g_spSkill.integer != loadData.skill) {
		g_spSkill.integer = loadData.skill;
		trap_Cvar_Set("g_spSkill", va("%d", loadData.skill));
    }

    savegameLoaded = qtrue;
}

/*
==========
G_LoadGameClient

Figure out which player data to load from save file for this gamePlayer
There can be a different number of bots on different maps, the order of the players changes sometimes,
	so find the human(s) to get correct player.
==========
*/
void G_LoadGameClient(int gamePlayer)
{
	int localPlayerNum;
	int i, j;
	gplayer_t *player;
	save_player_t *saved;

	// Check if save game is loaded.
	if (!savegameLoaded) {
        return;
	}

	// Bots don't save data in save files.
	if (g_entities[gamePlayer].r.svFlags & SVF_BOT) {
		return;
	}

	localPlayerNum = level.players[gamePlayer].pers.localPlayerNum;

	// Not a valid local player.
	if (localPlayerNum < 0) {
		return;
	}

	// Find save slot.
	saved = NULL;
	for (i = 0; i < MAX_SPLITVIEW; i++) {
		if (loadData.players[i].localPlayerNum == localPlayerNum) {
			saved = &loadData.players[i];
			break;
		}
	}

	if (!saved) {
		// Player not in savefile.
		return;
	}

	player = &level.players[gamePlayer];

	// Set model/headmodel.
	trap_Cvar_Set(Com_LocalPlayerCvarName(localPlayerNum, "spmodel"), saved->model);
	trap_Cvar_Set(Com_LocalPlayerCvarName(localPlayerNum, "spheadmodel"), saved->headModel);

	for (j = 0; j < MAX_HOLDABLE; j++) {
		player->ps.holdable[j] = saved->holdable[j];
	}
	player->ps.holdableIndex = saved->holdableSelected;
	player->ps.persistant[PERS_SCORE] = saved->score;
	player->ps.persistant[PERS_LIVES] = saved->lives;
	player->ps.persistant[PERS_CONTINUES] = saved->continues;
}
#endif // TA_SP
