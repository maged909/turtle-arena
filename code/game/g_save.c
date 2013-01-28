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
	byte localClientNum;
	char model[MAX_QPATH];
	char headModel[MAX_QPATH];
	byte holdable[MAX_HOLDABLE];
	byte holdableSelected;
	int score;
	byte lives;
	byte continues;
} save_client_t;

typedef struct
{
	// server expects these in this order!
	//
    byte version; // Save file version.
    char mapname[MAX_QPATH];
    byte maxclients;
    byte localClients;

	// game only, server doesn't read these.
	//
    byte skill;
    save_client_t clients[MAX_SPLITVIEW];

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
	int client;
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
	saveData.maxclients = level.maxclients;

	// Bits get added are each local client in savegame.
	saveData.localClients = 0;

	client = 0;
	for (i = 0; i < level.maxclients; i++)
	{
		if (g_entities[i].r.svFlags & SVF_BOT)
			continue;

		if (level.clients[i].pers.connected != CON_CONNECTED)
			continue;

		// Don't save dead (Game Over) clients.
		if (level.clients[i].ps.persistant[PERS_LIVES] < 1 && level.clients[i].ps.persistant[PERS_CONTINUES] < 1)
			continue;

		saveData.clients[client].localClientNum = level.clients[i].pers.localPlayerNum;
		saveData.localClients |= (1<<saveData.clients[client].localClientNum);

		// model/skin
		Q_strncpyz(saveData.clients[client].model, level.clients[i].pers.playercfg.model, MAX_QPATH);

		// headmodel/skin
		Q_strncpyz(saveData.clients[client].headModel, level.clients[i].pers.playercfg.headModel, MAX_QPATH);

		// holdable items
		for (j = 0; j < MAX_HOLDABLE; j++) {
			saveData.clients[client].holdable[j] = level.clients[i].ps.holdable[j];
		}
		saveData.clients[client].holdableSelected = level.clients[i].ps.holdableIndex;
		saveData.clients[client].score = level.clients[i].ps.persistant[PERS_SCORE];
		saveData.clients[client].lives = level.clients[i].ps.persistant[PERS_LIVES];
		saveData.clients[client].continues = level.clients[i].ps.persistant[PERS_CONTINUES];
		client++;
		if (client >= MAX_SPLITVIEW) {
			break;
		}
	}

	for ( ; client < MAX_SPLITVIEW; client++) {
		// Unused slots
		saveData.clients[client].localClientNum = 0xff;
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

	if (len == -1) {
		trap_FS_FCloseFile(f);
		G_Error("Savegame file not found!");
	}

	// Read data
	trap_FS_Read(&loadData, sizeof (save_t), f);

	trap_FS_FCloseFile(f);

	trap_Cvar_Set( "savegame_loading", "0" );
	trap_Cvar_Set( "savegame_filename", "" );

	// The server should check but just in case...
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

Figure out which client data to load from save file for this gameClient
There can be a different number of bots on different maps, the order of the clients changes sometimes,
	so find the human(s) to get correct client.
==========
*/
void G_LoadGameClient(int gameClient)
{
	int localClientNum;
	int i, j;
	gclient_t *client;
	save_client_t *saved;

	// Check if save game is loaded.
	if (!savegameLoaded) {
        return;
	}

	// Bots don't save data in save files.
	if (g_entities[gameClient].r.svFlags & SVF_BOT) {
		return;
	}

	localClientNum = level.clients[gameClient].pers.localPlayerNum;

	// Not a valid local client.
	if (localClientNum < 0) {
		return;
	}

	// Find save slot.
	saved = NULL;
	for (i = 0; i < MAX_SPLITVIEW; i++) {
		if (loadData.clients[i].localClientNum == localClientNum) {
			saved = &loadData.clients[i];
			break;
		}
	}

	if (!saved) {
		// Client not in savefile.
		return;
	}

	client = &level.clients[gameClient];

	// Set model/headmodel.
	trap_Cvar_Set(Com_LocalClientCvarName(localClientNum, "spmodel"), saved->model);
	trap_Cvar_Set(Com_LocalClientCvarName(localClientNum, "spheadmodel"), saved->headModel);

	for (j = 0; j < MAX_HOLDABLE; j++) {
		client->ps.holdable[j] = saved->holdable[j];
	}
	client->ps.holdableIndex = saved->holdableSelected;
	client->ps.persistant[PERS_SCORE] = saved->score;
	client->ps.persistant[PERS_LIVES] = saved->lives;
	client->ps.persistant[PERS_CONTINUES] = saved->continues;
}
#endif // TA_SP
