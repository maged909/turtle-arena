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

#include "g_local.h"

qboolean	G_SpawnString( const char *key, const char *defaultString, char **out ) {
	int		i;

	if ( !level.spawning ) {
		*out = (char *)defaultString;
//		G_Error( "G_SpawnString() called while not spawning" );
	}

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		if ( !Q_stricmp( key, level.spawnVars[i][0] ) ) {
			*out = level.spawnVars[i][1];
			return qtrue;
		}
	}

	*out = (char *)defaultString;
	return qfalse;
}

qboolean	G_SpawnFloat( const char *key, const char *defaultString, float *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atof( s );
	return present;
}

qboolean	G_SpawnInt( const char *key, const char *defaultString, int *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atoi( s );
	return present;
}

qboolean	G_SpawnVector( const char *key, const char *defaultString, float *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f", &out[0], &out[1], &out[2] );
	return present;
}



//
// fields are needed for spawning from the entity string
//
typedef enum {
	F_INT,
	F_FLOAT,
	F_STRING,
	F_VECTOR,
	F_ANGLEHACK
} fieldtype_t;

typedef struct
{
	char	*name;
	size_t	ofs;
	fieldtype_t	type;
} field_t;

field_t fields[] = {
    {"classname", FOFS(classname), F_STRING},
    {"origin", FOFS(s.origin), F_VECTOR},
    {"model", FOFS(model), F_STRING},
    {"model2", FOFS(model2), F_STRING},
    {"spawnflags", FOFS(spawnflags), F_INT},
    {"speed", FOFS(speed), F_FLOAT},
    {"target", FOFS(target), F_STRING},
#ifdef TA_ENTSYS
	{"paintarget", FOFS(paintarget), F_STRING},
#endif
    {"targetname", FOFS(targetname), F_STRING},
    {"message", FOFS(message), F_STRING},
    {"team", FOFS(team), F_STRING},
    {"wait", FOFS(wait), F_FLOAT},
    {"random", FOFS(random), F_FLOAT},
    {"count", FOFS(count), F_INT},
    {"health", FOFS(health), F_INT},
    {"dmg", FOFS(damage), F_INT},
    {"angles", FOFS(s.angles), F_VECTOR},
    {"angle", FOFS(s.angles), F_ANGLEHACK},
    {"targetShaderName", FOFS(targetShaderName), F_STRING},
    {"targetShaderNewName", FOFS(targetShaderNewName), F_STRING},
#ifdef TA_WEAPSYS // WIF_CUTS
	{"mustcut", FOFS(mustcut), F_INT},
#endif

	{NULL}
};


typedef struct {
	char	*name;
	void	(*spawn)(gentity_t *ent);
} spawn_t;

void SP_info_player_start (gentity_t *ent);
void SP_info_player_deathmatch (gentity_t *ent);
void SP_info_player_intermission (gentity_t *ent);

void SP_func_plat (gentity_t *ent);
void SP_func_static (gentity_t *ent);
void SP_func_rotating (gentity_t *ent);
void SP_func_bobbing (gentity_t *ent);
void SP_func_pendulum( gentity_t *ent );
void SP_func_button (gentity_t *ent);
void SP_func_door (gentity_t *ent);
void SP_func_train (gentity_t *ent);
void SP_func_timer (gentity_t *self);
#ifdef TA_ENTSYS // BREAKABLE
void SP_func_breakable (gentity_t *self);
#endif
#ifdef TA_ENTSYS // FUNC_USE
void SP_func_use (gentity_t *ent);
#endif
#ifdef TA_ENTSYS // FUNC_VOODOO
void SP_func_voodoo (gentity_t *ent);
#endif

void SP_trigger_always (gentity_t *ent);
void SP_trigger_multiple (gentity_t *ent);
void SP_trigger_push (gentity_t *ent);
void SP_trigger_teleport (gentity_t *ent);
void SP_trigger_hurt (gentity_t *ent);

void SP_target_remove_powerups( gentity_t *ent );
void SP_target_give (gentity_t *ent);
void SP_target_delay (gentity_t *ent);
void SP_target_speaker (gentity_t *ent);
void SP_target_print (gentity_t *ent);
void SP_target_laser (gentity_t *self);
void SP_target_score( gentity_t *ent );
void SP_target_teleporter( gentity_t *ent );
void SP_target_relay (gentity_t *ent);
void SP_target_kill (gentity_t *ent);
void SP_target_position (gentity_t *ent);
void SP_target_location (gentity_t *ent);
void SP_target_push (gentity_t *ent);
#ifdef TA_SP
void SP_target_level_end (gentity_t *ent);
#endif

void SP_light (gentity_t *self);
void SP_info_null (gentity_t *self);
void SP_info_notnull (gentity_t *self);
void SP_info_camp (gentity_t *self);
void SP_path_corner (gentity_t *self);

void SP_misc_teleporter_dest (gentity_t *self);
void SP_misc_model(gentity_t *ent);
#ifdef TA_ENTSYS // MISC_OBJECT
void SP_misc_object(gentity_t *ent);
#endif
void SP_misc_portal_camera(gentity_t *ent);
void SP_misc_portal_surface(gentity_t *ent);

#ifdef TA_WEAPSYS
void SP_misc_shooter( gentity_t *ent );
#endif
void SP_shooter_rocket( gentity_t *ent );
void SP_shooter_plasma( gentity_t *ent );
void SP_shooter_grenade( gentity_t *ent );

void SP_team_CTF_redplayer( gentity_t *ent );
void SP_team_CTF_blueplayer( gentity_t *ent );

void SP_team_CTF_redspawn( gentity_t *ent );
void SP_team_CTF_bluespawn( gentity_t *ent );

#ifdef MISSIONPACK
void SP_team_blueobelisk( gentity_t *ent );
void SP_team_redobelisk( gentity_t *ent );
void SP_team_neutralobelisk( gentity_t *ent );
#endif
void SP_item_botroam( gentity_t *ent ) { }
#ifdef TA_PATHSYS
void SP_path_start( gentity_t *ent );
#elif !defined TA_PATHSYS
void SP_npcpath( gentity_t *ent );
#endif
#ifdef TA_WEAPSYS
void SP_weapon_random( gentity_t *ent );
#endif
#ifdef NIGHTSMODE
void SP_nights_start( gentity_t *ent ); // Ideya Drone
void SP_nights_target( gentity_t *ent ); // Ideya Capture
#endif
#ifdef TA_PATHSYS
void SP_target_setpath( gentity_t *ent );
#endif


spawn_t	spawns[] = {
	// info entities don't do anything at all, but provide positional
	// information for things controlled by other processes
	{"info_player_start", SP_info_player_start},
	{"info_player_deathmatch", SP_info_player_deathmatch},
	{"info_player_intermission", SP_info_player_intermission},
	{"info_null", SP_info_null},
	{"info_notnull", SP_info_notnull},		// use target_position instead
	{"info_camp", SP_info_camp},

	{"func_plat", SP_func_plat},
	{"func_button", SP_func_button},
	{"func_door", SP_func_door},
	{"func_static", SP_func_static},
	{"func_rotating", SP_func_rotating},
	{"func_bobbing", SP_func_bobbing},
	{"func_pendulum", SP_func_pendulum},
	{"func_train", SP_func_train},
	{"func_group", SP_info_null},
	{"func_timer", SP_func_timer},			// rename trigger_timer?
#ifdef TA_ENTSYS // BREAKABLE
	{"func_breakable", SP_func_breakable},
#endif
#ifdef TA_ENTSYS // FUNC_USE
	{"func_use", SP_func_use},
#endif
#ifdef TA_ENTSYS // FUNC_VOODOO
	{"func_voodoo", SP_func_voodoo},
#endif

	// Triggers are brush objects that cause an effect when contacted
	// by a living player, usually involving firing targets.
	// While almost everything could be done with
	// a single trigger class and different targets, triggered effects
	// could not be client side predicted (push and teleport).
	{"trigger_always", SP_trigger_always},
	{"trigger_multiple", SP_trigger_multiple},
	{"trigger_push", SP_trigger_push},
	{"trigger_teleport", SP_trigger_teleport},
	{"trigger_hurt", SP_trigger_hurt},

	// targets perform no action by themselves, but must be triggered
	// by another entity
	{"target_give", SP_target_give},
	{"target_remove_powerups", SP_target_remove_powerups},
	{"target_delay", SP_target_delay},
	{"target_speaker", SP_target_speaker},
	{"target_print", SP_target_print},
	{"target_laser", SP_target_laser},
	{"target_score", SP_target_score},
	{"target_teleporter", SP_target_teleporter},
	{"target_relay", SP_target_relay},
	{"target_kill", SP_target_kill},
	{"target_position", SP_target_position},
	{"target_location", SP_target_location},
	{"target_push", SP_target_push},
#ifdef TA_SP
	{"target_level_end", SP_target_level_end},
#endif

	{"light", SP_light},
	{"path_corner", SP_path_corner},
#ifdef TA_PATHSYS
	{"path_start", SP_path_start},
	{"path_point", SP_path_corner},
	{"path_axis", SP_path_corner},
	{"path_end", SP_path_corner},
#endif

	{"misc_teleporter_dest", SP_misc_teleporter_dest},
	{"misc_model", SP_misc_model},
#ifdef TA_ENTSYS // MISC_OBJECT
	{"misc_gamemodel", SP_misc_object}, // Shows model in NetRadiant
	{"misc_object", SP_misc_object},
#endif
	{"misc_portal_surface", SP_misc_portal_surface},
	{"misc_portal_camera", SP_misc_portal_camera},

#ifdef TA_WEAPSYS
	{"misc_shooter", SP_misc_shooter},
#endif
	{"shooter_rocket", SP_shooter_rocket},
	{"shooter_grenade", SP_shooter_grenade},
	{"shooter_plasma", SP_shooter_plasma},

	{"team_CTF_redplayer", SP_team_CTF_redplayer},
	{"team_CTF_blueplayer", SP_team_CTF_blueplayer},

	{"team_CTF_redspawn", SP_team_CTF_redspawn},
	{"team_CTF_bluespawn", SP_team_CTF_bluespawn},

#ifdef MISSIONPACK
	{"team_redobelisk", SP_team_redobelisk},
	{"team_blueobelisk", SP_team_blueobelisk},
	{"team_neutralobelisk", SP_team_neutralobelisk},
#endif
	{"item_botroam", SP_item_botroam},

#ifdef TA_WEAPSYS
	{"weapon_random", SP_weapon_random},
#endif
#if defined TA_NPCSYS && !defined TA_PATHSYS
	{"npcpath", SP_npcpath},
#endif
#ifdef NIGHTSMODE
	{"nights_start", SP_nights_start},
	{"nights_target", SP_nights_target},
#endif
#ifdef TA_PATHSYS
	{"target_setpath", SP_target_setpath},
#endif

	{NULL, 0}
};

/*
===============
G_CallSpawn

Finds the spawn function for the entity and calls it,
returning qfalse if not found
===============
*/
qboolean G_CallSpawn( gentity_t *ent ) {
	spawn_t	*s;
	gitem_t	*item;
#if defined TA_ITEMSYS || defined TA_NPCSYS
	int i;
#endif

	if ( !ent->classname ) {
		G_Printf ("G_CallSpawn: NULL classname\n");
		return qfalse;
	}


#ifdef TA_NPCSYS
	// check NPC spawn functions
	for ( i = 1; i < BG_NumNPCs(); i++ ) {
		if ( !strcmp(bg_npcinfo[i].classname, ent->classname) ) {
			G_SpawnNPC( ent, &bg_npcinfo[i] );
			return qtrue;
		}
	}
#endif

#ifdef IOQ3ZTM // LASERTAG
	if (g_laserTag.integer) {
		if ( !strcmp("weapon_random", ent->classname) ) {
			return qfalse;
		}
	} else {
#endif
#ifdef TA_ITEMSYS
	// check item spawn functions
	for ( i = 1; i < BG_NumItems(); i++ ) {
		item = &bg_iteminfo[i];
		if ( item->classname[0] == '\0' ) {
			continue;
		}
		if ( !strcmp(item->classname, ent->classname) ) {
			G_SpawnItem( ent, item );
			return qtrue;
		}
	}
#else
	// check item spawn functions
	for ( item=bg_itemlist+1 ; item->classname ; item++ ) {
		if ( !strcmp(item->classname, ent->classname) ) {
			G_SpawnItem( ent, item );
			return qtrue;
		}
	}
#endif
#ifdef IOQ3ZTM // LASERTAG
	}
#endif

	// check normal spawn functions
	for ( s=spawns ; s->name ; s++ ) {
		if ( !strcmp(s->name, ent->classname) ) {
			// found it
			s->spawn(ent);
			return qtrue;
		}
	}

#ifdef TA_WEAPSYS
	// Unknown weapon, spawn weapon_random
	if (Q_stricmpn(ent->classname, "weapon_", 7) == 0)
	{
		G_Printf ("%s doesn't have a spawn function, using weapon_random instead\n", ent->classname);
		SP_weapon_random(ent);
		return qtrue;
	}
#endif
	G_Printf ("%s doesn't have a spawn function\n", ent->classname);
	return qfalse;
}

/*
=============
G_NewString

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
char *G_NewString( const char *string ) {
	char	*newb, *new_p;
	int		i,l;
	
	l = strlen(string) + 1;

	newb = G_Alloc( l );

	new_p = newb;

	// turn \n into a real linefeed
	for ( i=0 ; i< l ; i++ ) {
		if (string[i] == '\\' && i < l-1) {
			i++;
			if (string[i] == 'n') {
				*new_p++ = '\n';
			} else {
				*new_p++ = '\\';
			}
		} else {
			*new_p++ = string[i];
		}
	}
	
	return newb;
}




/*
===============
G_ParseField

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
void G_ParseField( const char *key, const char *value, gentity_t *ent ) {
	field_t	*f;
	byte	*b;
	float	v;
	vec3_t	vec;

	for ( f=fields ; f->name ; f++ ) {
		if ( !Q_stricmp(f->name, key) ) {
			// found it
			b = (byte *)ent;

			switch( f->type ) {
			case F_STRING:
				*(char **)(b+f->ofs) = G_NewString (value);
				break;
			case F_VECTOR:
				sscanf (value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
				((float *)(b+f->ofs))[0] = vec[0];
				((float *)(b+f->ofs))[1] = vec[1];
				((float *)(b+f->ofs))[2] = vec[2];
				break;
			case F_INT:
				*(int *)(b+f->ofs) = atoi(value);
				break;
			case F_FLOAT:
				*(float *)(b+f->ofs) = atof(value);
				break;
			case F_ANGLEHACK:
				v = atof(value);
				((float *)(b+f->ofs))[0] = 0;
				((float *)(b+f->ofs))[1] = v;
				((float *)(b+f->ofs))[2] = 0;
				break;
			}
			return;
		}
	}
}

#define ADJUST_AREAPORTAL() \
	if(ent->s.eType == ET_MOVER) \
	{ \
		trap_LinkEntity(ent); \
		trap_AdjustAreaPortalState(ent, qtrue); \
	}

/*
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnVars[], then call the class specfic spawn function
===================
*/
void G_SpawnGEntityFromSpawnVars( void ) {
	int			i;
	gentity_t	*ent;
	char		*s, *value, *gametypeName;
	static char *gametypeNames[GT_MAX_GAME_TYPE] = {
		"ffa",
#ifdef TA_MISC // tornament to duel
		"duel",
#else
		"tournament",
#endif
#ifdef TA_SP
		"coop",
#else
		"single",
#endif
		"team",
		"ctf",
#ifdef MISSIONPACK
		"oneflag",
#ifdef TA_MISC // tornament to duel, obelisk to overload
		"overload",
#else
		"obelisk",
#endif
#ifdef MISSIONPACK_HARVESTER
		"harvester"
#endif
#endif
	};

	// get the next free entity
	ent = G_Spawn();

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		G_ParseField( level.spawnVars[i][0], level.spawnVars[i][1], ent );
	}

	// check for "notsingle" flag
#ifdef TA_SP
	if ( g_singlePlayer.integer && g_gametype.integer == GT_SINGLE_PLAYER )
#else
	if ( g_gametype.integer == GT_SINGLE_PLAYER )
#endif
	{
		G_SpawnInt( "notsingle", "0", &i );
		if ( i ) {
			ADJUST_AREAPORTAL();
			G_FreeEntity( ent );
			return;
		}
	}
	// check for "notteam" flag (GT_FFA, GT_TOURNAMENT, GT_SINGLE_PLAYER)
	if ( g_gametype.integer >= GT_TEAM ) {
		G_SpawnInt( "notteam", "0", &i );
		if ( i ) {
			ADJUST_AREAPORTAL();
			G_FreeEntity( ent );
			return;
		}
	} else {
		G_SpawnInt( "notfree", "0", &i );
		if ( i ) {
			ADJUST_AREAPORTAL();
			G_FreeEntity( ent );
			return;
		}
	}

#ifdef TURTLEARENA
	G_SpawnInt( "notturtlearena", "0", &i );
	if ( i ) {
		ADJUST_AREAPORTAL();
		G_FreeEntity( ent );
		return;
	}
#else
#ifdef MISSIONPACK
	G_SpawnInt( "notta", "0", &i );
	if ( i ) {
		ADJUST_AREAPORTAL();
		G_FreeEntity( ent );
		return;
	}
#else
	G_SpawnInt( "notq3a", "0", &i );
	if ( i ) {
		ADJUST_AREAPORTAL();
		G_FreeEntity( ent );
		return;
	}
#endif
#endif

#ifdef TA_SP // ZTM: Support single player and coop separately.
	if ( g_singlePlayer.integer && g_gametype.integer == GT_SINGLE_PLAYER )
		gametypeName = "single";
	else if ( g_gametype.integer >= 0 && g_gametype.integer < GT_MAX_GAME_TYPE ) {
		gametypeName = gametypeNames[g_gametype.integer];
	} else {
		gametypeName = NULL;
	}
#endif

#ifdef IOQ3ZTM // ZTM: Allow not spawning in only some gametypes. Copied from OpenArena (oax)
	if( G_SpawnString( "!gametype", NULL, &value ) ) {
#ifndef TA_SP
		if( g_gametype.integer >= 0 && g_gametype.integer < GT_MAX_GAME_TYPE ) {
			gametypeName = gametypeNames[g_gametype.integer];
#endif

			s = strstr( value, gametypeName );
			if( s ) {
				ADJUST_AREAPORTAL();
				G_FreeEntity( ent );
				return;
			}
#ifndef TA_SP
		}
#endif
	}
#endif

	if( G_SpawnString( "gametype", NULL, &value ) ) {
#ifndef TA_SP
		if( g_gametype.integer >= 0 && g_gametype.integer < GT_MAX_GAME_TYPE ) {
			gametypeName = gametypeNames[g_gametype.integer];
#endif

			s = strstr( value, gametypeName );
			if( !s ) {
				ADJUST_AREAPORTAL();
				G_FreeEntity( ent );
				return;
			}
#ifndef TA_SP
		}
#endif
	}

	// move editor origin to pos
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	// if we didn't get a classname, don't bother spawning anything
	if ( !G_CallSpawn( ent ) ) {
		G_FreeEntity( ent );
	}
}



/*
====================
G_AddSpawnVarToken
====================
*/
char *G_AddSpawnVarToken( const char *string ) {
	int		l;
	char	*dest;

	l = strlen( string );
	if ( level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
		G_Error( "G_AddSpawnVarToken: MAX_SPAWN_VARS_CHARS" );
	}

	dest = level.spawnVarChars + level.numSpawnVarChars;
	memcpy( dest, string, l+1 );

	level.numSpawnVarChars += l + 1;

	return dest;
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean G_ParseSpawnVars( void ) {
	char		keyname[MAX_TOKEN_CHARS];
	char		com_token[MAX_TOKEN_CHARS];

	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	// parse the opening brace
	if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
		// end of spawn string
		return qfalse;
	}
	if ( com_token[0] != '{' ) {
		G_Error( "G_ParseSpawnVars: found %s when expecting {",com_token );
	}

	// go through all the key / value pairs
	while ( 1 ) {	
		// parse key
		if ( !trap_GetEntityToken( keyname, sizeof( keyname ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( keyname[0] == '}' ) {
			break;
		}
		
		// parse value	
		if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( com_token[0] == '}' ) {
			G_Error( "G_ParseSpawnVars: closing brace without data" );
		}
		if ( level.numSpawnVars == MAX_SPAWN_VARS ) {
			G_Error( "G_ParseSpawnVars: MAX_SPAWN_VARS" );
		}
		level.spawnVars[ level.numSpawnVars ][0] = G_AddSpawnVarToken( keyname );
		level.spawnVars[ level.numSpawnVars ][1] = G_AddSpawnVarToken( com_token );
		level.numSpawnVars++;
	}

	return qtrue;
}



/*QUAKED worldspawn (0 0 0) ?

Every map should have exactly one worldspawn.
"music"		music wav file
"gravity"	800 is default gravity
"message"	Text to print during connection process
*/
void SP_worldspawn( void ) {
	char	*s;

	G_SpawnString( "classname", "", &s );
	if ( Q_stricmp( s, "worldspawn" ) ) {
		G_Error( "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}

	// make some data visible to connecting client
	trap_SetConfigstring( CS_GAME_VERSION, GAME_VERSION );

	trap_SetConfigstring( CS_LEVEL_START_TIME, va("%i", level.startTime ) );

	G_SpawnString( "music", "", &s );
	trap_SetConfigstring( CS_MUSIC, s );

	G_SpawnString( "message", "", &s );
	trap_SetConfigstring( CS_MESSAGE, s );				// map specific message

	trap_SetConfigstring( CS_MOTD, g_motd.string );		// message of the day

	G_SpawnString( "gravity", "800", &s );
	trap_Cvar_Set( "g_gravity", s );
#ifdef IOQ3ZTM // Don't print message about map changing gravity.
	G_CvarClearModification(&g_gravity);
#endif

#ifdef MISSIONPACK
	G_SpawnString( "enableDust", "0", &s );
	trap_Cvar_Set( "g_enableDust", s );
#ifdef IOQ3ZTM // Don't print message about map changing dust.
	G_CvarClearModification(&g_enableDust);
#endif

	G_SpawnString( "enableBreath", "0", &s );
	trap_Cvar_Set( "g_enableBreath", s );
#ifdef IOQ3ZTM // Don't print message about map changing breath.
	G_CvarClearModification(&g_enableBreath);
#endif
#endif

#ifdef TA_PATHSYS // 2DMODE
	G_SpawnString( "2dmode", "0", &s );
	trap_Cvar_Set( "g_2dmode", s );
#ifdef IOQ3ZTM // Don't print message about map changing 2d mode.
	G_CvarClearModification(&g_2dmode);
#endif
#endif

#if 0 // ZTM: Currently game doesn't need the tracemap
	level.mapcoordsValid = qfalse;
	if ( G_SpawnVector2D( "mapcoordsmins", "-128 128", level.mapcoordsMins ) &&  // top left
		 G_SpawnVector2D( "mapcoordsmaxs", "128 -128", level.mapcoordsMaxs ) ) { // bottom right
		level.mapcoordsValid = qtrue;
	}
#endif

	g_entities[ENTITYNUM_WORLD].s.number = ENTITYNUM_WORLD;
	g_entities[ENTITYNUM_WORLD].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_WORLD].classname = "worldspawn";

	g_entities[ENTITYNUM_NONE].s.number = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_NONE].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_NONE].classname = "nothing";

	// see if we want a warmup time
	trap_SetConfigstring( CS_WARMUP, "" );
	if ( g_restarted.integer ) {
		trap_Cvar_Set( "g_restarted", "0" );
		level.warmupTime = 0;
	} else if ( g_doWarmup.integer ) { // Turn it on
		level.warmupTime = -1;
		trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
		G_LogPrintf( "Warmup:\n" );
	}

}


/*
==============
G_SpawnEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void G_SpawnEntitiesFromString( void ) {
	// allow calls to G_Spawn*()
	level.spawning = qtrue;
	level.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if ( !G_ParseSpawnVars() ) {
		G_Error( "SpawnEntities: no entities" );
	}
	SP_worldspawn();

	// parse ents
	while( G_ParseSpawnVars() ) {
		G_SpawnGEntityFromSpawnVars();
	}	

	level.spawning = qfalse;			// any future calls to G_Spawn*() will be errors
}

