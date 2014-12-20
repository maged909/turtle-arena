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

// g_client.c -- client functions that don't happen every frame

#ifndef TA_PLAYERSYS
static vec3_t	playerMins = {-15, -15, -24};
static vec3_t	playerMaxs = {15, 15, 32};
#endif

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_deathmatch( gentity_t *ent ) {
	int		i;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
#ifndef TA_SP
equivelant to info_player_deathmatch
#endif
*/
void SP_info_player_start(gentity_t *ent) {
#ifndef TA_SP
	ent->classname = "info_player_deathmatch";
#endif
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
*/
void SP_info_player_intermission( gentity_t *ent ) {

}



/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
SpotWouldTelefrag

================
*/
qboolean SpotWouldTelefrag( gentity_t *spot
#ifdef TA_PLAYERSYS
	, gentity_t *ent
#endif
	)
{
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

#ifdef TA_PLAYERSYS
	if (ent && ent->player)
	{
		// ZTM: Use per-player bounding box for telefrag checking!
		VectorAdd( spot->s.origin, ent->player->pers.playercfg.bbmins, mins );
		VectorAdd( spot->s.origin, ent->player->pers.playercfg.bbmaxs, maxs );
	}
	else
	{
		// Use default Q3 mins and maxs. (Set in BG_LoadPlayerCFGFile)
		vec3_t	playerMins = {-15, -15, -24};
		vec3_t	playerMaxs = { 15,  15,  32};

		VectorAdd( spot->s.origin, playerMins, mins );
		VectorAdd( spot->s.origin, playerMaxs, maxs );
	}
#else
	VectorAdd( spot->s.origin, playerMins, mins );
	VectorAdd( spot->s.origin, playerMaxs, maxs );
#endif
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) {
		hit = &g_entities[touch[i]];
		//if ( hit->player && hit->player->ps.stats[STAT_HEALTH] > 0 ) {
		if ( hit->player) {
			return qtrue;
		}

	}

	return qfalse;
}

#ifdef TA_SP
gentity_t *SelectSinglePlayerSpawnPoint(
#ifdef TA_PLAYERSYS
	gentity_t *ent,
#endif
	int playerNum, vec3_t origin, vec3_t angles, qboolean isbot );
#endif

#ifndef IOQ3ZTM // ZTM: unused
/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectNearestDeathmatchSpawnPoint( vec3_t from ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist, nearestDist;
	gentity_t	*nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {

		VectorSubtract( spot->s.origin, from, delta );
		dist = VectorLength( delta );
		if ( dist < nearestDist ) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}


/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectRandomDeathmatchSpawnPoint(qboolean isbot) {
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	while((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL && count < MAX_SPAWN_POINTS)
	{
		if(SpotWouldTelefrag(spot))
			continue;

		if(((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
		{
			// spot is not for this human/bot player
			continue;
		}

		spots[count] = spot;
		count++;
	}

	if ( !count ) {	// no spots that won't telefrag
		return G_Find( NULL, FOFS(classname), "info_player_deathmatch");
	}

	selection = rand() % count;
	return spots[ selection ];
}
#else
#define	MAX_SPAWN_POINTS	128
#endif

/*
===========
SelectRandomFurthestSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
#ifdef TA_PLAYERSYS
gentity_t *SelectRandomFurthestSpawnPoint ( gentity_t *ent, vec3_t origin, vec3_t angles, qboolean isbot )
#else
gentity_t *SelectRandomFurthestSpawnPoint ( vec3_t avoidPoint, vec3_t origin, vec3_t angles, qboolean isbot )
#endif
{
	gentity_t	*spot;
	vec3_t		delta;
	float		dist;
	float		list_dist[MAX_SPAWN_POINTS];
	gentity_t	*list_spot[MAX_SPAWN_POINTS];
	int			numSpots, rnd, i, j;
#ifdef TA_PLAYERSYS
	vec3_t avoidPoint;

	if (ent && ent->player) {
		VectorCopy(ent->player->ps.origin, avoidPoint);
	} else {
		VectorCopy(vec3_origin, avoidPoint);
	}
#endif

	numSpots = 0;
	spot = NULL;

	while((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
#ifdef TA_PLAYERSYS
		if(SpotWouldTelefrag(spot, ent))
#else
		if(SpotWouldTelefrag(spot))
#endif
			continue;

		if(((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
		{
			// spot is not for this human/bot player
			continue;
		}

		VectorSubtract( spot->s.origin, avoidPoint, delta );
		dist = VectorLength( delta );

		for (i = 0; i < numSpots; i++)
		{
			if(dist > list_dist[i])
			{
				if (numSpots >= MAX_SPAWN_POINTS)
					numSpots = MAX_SPAWN_POINTS - 1;
					
				for(j = numSpots; j > i; j--)
				{
					list_dist[j] = list_dist[j-1];
					list_spot[j] = list_spot[j-1];
				}
				
				list_dist[i] = dist;
				list_spot[i] = spot;
				
				numSpots++;
				break;
			}
		}
		
		if(i >= numSpots && numSpots < MAX_SPAWN_POINTS)
		{
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}
	
	if(!numSpots)
	{
		spot = G_Find(NULL, FOFS(classname), "info_player_deathmatch");

#ifdef TA_SP
		// Check for info_player_start if we haven't checked already
		if (!spot && g_gametype.integer != GT_SINGLE_PLAYER)
		{
			spot = SelectSinglePlayerSpawnPoint(
#ifdef TA_PLAYERSYS
							ent,
#endif
							ent - g_entities, origin, angles,
							!!(ent->r.svFlags & SVF_BOT));

			if (spot) {
				return spot;
			}
		}
#endif

		if (!spot)
			G_Error( "Couldn't find a spawn point" );

		VectorCopy (spot->s.origin, origin);
		origin[2] += 9;
		VectorCopy (spot->s.angles, angles);
		return spot;
	}

	// select a random spot from the spawn points furthest away
	rnd = random() * (numSpots / 2);

	VectorCopy (list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	VectorCopy (list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
#ifdef TA_PLAYERSYS
gentity_t *SelectSpawnPoint ( gentity_t *ent, vec3_t origin, vec3_t angles, qboolean isbot )
#else
gentity_t *SelectSpawnPoint ( vec3_t avoidPoint, vec3_t origin, vec3_t angles, qboolean isbot )
#endif
{
#ifdef TA_PLAYERSYS
	return SelectRandomFurthestSpawnPoint( ent, origin, angles, isbot );
#else
	return SelectRandomFurthestSpawnPoint( avoidPoint, origin, angles, isbot );
#endif

	/*
	gentity_t	*spot;
	gentity_t	*nearestSpot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint( avoidPoint );

	spot = SelectRandomDeathmatchSpawnPoint ( );
	if ( spot == nearestSpot ) {
		// roll again if it would be real close to point of death
		spot = SelectRandomDeathmatchSpawnPoint ( );
		if ( spot == nearestSpot ) {
			// last try
			spot = SelectRandomDeathmatchSpawnPoint ( );
		}		
	}

	// find a single player start spot
	if (!spot) {
		G_Error( "Couldn't find a spawn point" );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
	*/
}

#ifdef TA_SP
/*
===========
SelectSinglePlayerSpawnPoint

Trys to find info_player_start with player num,
if none uses info_player_start for player 0,
if none uses last info_player_start found.
if none uses 'initial' deathmatch spawn point,
if none uses deathmatch spawn point.
============
*/
gentity_t *SelectSinglePlayerSpawnPoint(
#ifdef TA_PLAYERSYS
	gentity_t *ent,
#endif
	int playerNum, vec3_t origin, vec3_t angles, qboolean isbot )
{
	gentity_t	*spot = NULL;
	gentity_t	*spot0 = NULL;
	gentity_t	*spotlast = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_start")) != NULL) {
		spotlast = spot;
		// Save spot 0 as it will be the default there isn't one for this player.
		if (spot->count == 0) {
			spot0 = spot;
		}
		if ( spot->count == playerNum ) {
			break;
		}
	}
	if (!spot && spot0) {
		spot = spot0;
	}
	else if (!spot && spotlast) {
		spot = spotlast;
	}
	// There are no info_player_start, use a deathmatch start.
	else if (!spot)
	{
		G_Printf("Warning: Map missing info_player_start atemping to use deathmacth spawn.\n");

		while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL)
		{
			if(((spot->flags & FL_NO_BOTS) && isbot) ||
			   ((spot->flags & FL_NO_HUMANS) && !isbot))
			{
				continue;
			}

			if ( spot->spawnflags & 1 ) {
				break;
			}
		}
	}

	if (spot) {
		VectorCopy (spot->s.origin, origin);
		origin[2] += 9;
		VectorCopy (spot->s.angles, angles);

		return spot;
	}

	// Fall back to deathmatch starts in co-op,
	// other gametypes have already tried deathmatch at this point.
#ifdef TA_PLAYERSYS
	if ( !spot && g_gametype.integer == GT_SINGLE_PLAYER )
		spot = SelectSpawnPoint( ent, origin, angles, isbot );
#else
	if ( !spot && g_gametype.integer == GT_SINGLE_PLAYER )
		spot = SelectSpawnPoint( vec3_origin, origin, angles, isbot );
#endif

	if (!spot)
		G_Error("SelectSinglePlayerSpawnPoint: Failed to find player start\n");

	return spot;
}
#endif

/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
#ifdef TA_PLAYERSYS
gentity_t *SelectInitialSpawnPoint( gentity_t *ent, vec3_t origin, vec3_t angles, qboolean isbot )
#else
gentity_t *SelectInitialSpawnPoint( vec3_t origin, vec3_t angles, qboolean isbot )
#endif
{
	gentity_t	*spot;

	spot = NULL;
	
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		if(((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
		{
			continue;
		}
		
		if((spot->spawnflags & 0x01))
			break;
	}

#ifdef TA_PLAYERSYS
	if (!spot || SpotWouldTelefrag(spot, ent))
		return SelectSpawnPoint(ent, origin, angles, isbot);
#else
	if (!spot || SpotWouldTelefrag(spot))
		return SelectSpawnPoint(vec3_origin, origin, angles, isbot);
#endif

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*
===========
SelectSpectatorSpawnPoint

============
*/
gentity_t *SelectSpectatorSpawnPoint( vec3_t origin, vec3_t angles ) {
	FindIntermissionPoint();

	VectorCopy( level.intermission_origin, origin );
	VectorCopy( level.intermission_angle, angles );

	return NULL;
}

/*
=======================================================================

BODYQUE

=======================================================================
*/

/*
===============
InitBodyQue
===============
*/
void InitBodyQue (void) {
	int		i;
	gentity_t	*ent;

	level.bodyQueIndex = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++) {
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

/*
=============
BodyQueFree

The body ques are never actually freed, they are just unlinked
=============
*/
void BodyQueFree( gentity_t *ent ) {
	trap_UnlinkEntity( ent );
	ent->physicsObject = qfalse;
}

/*
=============
CopyToBodyQue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
void CopyToBodyQue( gentity_t *ent ) {
#if defined MISSIONPACK && !defined TURTLEARENA // NO_KAMIKAZE_ITEM
	gentity_t	*e;
	int i;
#endif
	gentity_t		*body;
	int			contents;

	trap_UnlinkEntity (ent);

	// if player is in a nodrop area, don't leave the body
	contents = trap_PointContents( ent->s.origin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		return;
	}

	// grab a body que and cycle to the next one
	body = level.bodyQue[ level.bodyQueIndex ];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;

	body->s = ent->s;
	body->s.eFlags = EF_DEAD;		// clear EF_TALK, etc
#if defined MISSIONPACK && !defined TURTLEARENA // NO_KAMIKAZE_ITEM
	if ( ent->s.eFlags & EF_KAMIKAZE ) {
		body->s.eFlags |= EF_KAMIKAZE;

		// check if there is a kamikaze timer around for this owner
		for (i = 0; i < level.num_entities; i++) {
			e = &g_entities[i];
			if (!e->inuse)
				continue;
			if (e->activator != ent)
				continue;
			if (strcmp(e->classname, "kamikaze timer"))
				continue;
			e->activator = body;
			break;
		}
	}
#endif
	body->s.powerups = 0;	// clear powerups
	body->s.loopSound = 0;	// clear lava burning
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	if ( body->s.groundEntityNum == ENTITYNUM_NONE ) {
		body->s.pos.trType = TR_GRAVITY;
		VectorCopy( ent->player->ps.velocity, body->s.pos.trDelta );
	} else {
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.pos.trTime = level.time;
	body->s.event = 0;

	// change the animation to the last-frame only, so the sequence
	// doesn't repeat anew for the body
	switch ( body->s.legsAnim & ~ANIM_TOGGLEBIT ) {
	case BOTH_DEATH1:
	case BOTH_DEAD1:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD1;
		break;
	case BOTH_DEATH2:
	case BOTH_DEAD2:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD2;
		break;
	case BOTH_DEATH3:
	case BOTH_DEAD3:
	default:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD3;
		break;
	}

	body->r.svFlags = ent->r.svFlags;
	VectorCopy (ent->s.mins, body->s.mins);
	VectorCopy (ent->s.maxs, body->s.maxs);
	VectorCopy (ent->r.absmin, body->r.absmin);
	VectorCopy (ent->r.absmax, body->r.absmax);

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.ownerNum = ent->s.number;

	body->nextthink = level.time + BODY_SINK_DELAY + BODY_SINK_TIME;
	body->think = BodyQueFree;

	body->die = body_die;

#ifdef NOTRATEDM // No gibs.
	body->takedamage = qtrue;
#else
	// don't take more damage if already gibbed
	if ( ent->health <= GIB_HEALTH ) {
		body->s.eFlags |= EF_GIBBED;
		body->s.contents = 0;
		body->takedamage = qfalse;
	} else {
		body->s.contents = CONTENTS_CORPSE;
		body->takedamage = qtrue;
	}
#endif


	VectorCopy ( body->s.pos.trBase, body->r.currentOrigin );
	trap_LinkEntity (body);
}

//======================================================================


/*
==================
SetPlayerViewAngle

==================
*/
void SetPlayerViewAngle( gentity_t *ent, vec3_t angle ) {
	int			i;

	// set the delta angle
	for (i=0 ; i<3 ; i++) {
		int		cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ent->player->ps.delta_angles[i] = cmdAngle - ent->player->pers.cmd.angles[i];
	}
	VectorCopy( angle, ent->s.angles );
	VectorCopy (ent->s.angles, ent->player->ps.viewangles);
}

/*
================
PlayerRespawn
================
*/
void PlayerRespawn( gentity_t *ent ) {

	CopyToBodyQue (ent);
	PlayerSpawn(ent);
}

/*
================
TeamCount

Returns number of players on a team
================
*/
int TeamCount( int ignorePlayerNum, team_t team ) {
	int		i;
	int		count = 0;

	for ( i = 0 ; i < level.maxplayers ; i++ ) {
		if ( i == ignorePlayerNum ) {
			continue;
		}
		if ( level.players[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.players[i].sess.sessionTeam == team ) {
			count++;
		}
	}

	return count;
}

/*
================
TeamLeader

Returns the player number of the team leader
================
*/
int TeamLeader( int team ) {
	int		i;

	for ( i = 0 ; i < level.maxplayers ; i++ ) {
		if ( level.players[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.players[i].sess.sessionTeam == team ) {
			if ( level.players[i].sess.teamLeader )
				return i;
		}
	}

	return -1;
}


/*
================
PickTeam

================
*/
team_t PickTeam( int ignorePlayerNum ) {
	int		counts[TEAM_NUM_TEAMS];

#ifdef TA_SP // SP_BOSS
	if (g_gametype.integer == GT_SINGLE_PLAYER)
		return TEAM_FREE;
#endif

	counts[TEAM_BLUE] = TeamCount( ignorePlayerNum, TEAM_BLUE );
	counts[TEAM_RED] = TeamCount( ignorePlayerNum, TEAM_RED );

	if ( counts[TEAM_BLUE] > counts[TEAM_RED] ) {
		return TEAM_RED;
	}
	if ( counts[TEAM_RED] > counts[TEAM_BLUE] ) {
		return TEAM_BLUE;
	}
	// equal team count, so join the team with the lowest score
	if ( level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED] ) {
		return TEAM_RED;
	}
	return TEAM_BLUE;
}

/*
===========
PlayerCleanName
============
*/
static void PlayerCleanName(const char *in, char *out, int outSize)
{
	int outpos = 0, colorlessLen = 0, spaces = 0;

	// discard leading spaces
	for(; *in == ' '; in++);
	
	for(; *in && outpos < outSize - 1; in++)
	{
		out[outpos] = *in;

		if(*in == ' ')
		{
			// don't allow too many consecutive spaces
			if(spaces > 2)
				continue;
			
			spaces++;
		}
		else if(outpos > 0 && out[outpos - 1] == Q_COLOR_ESCAPE)
		{
			if(Q_IsColorString(&out[outpos - 1]))
			{
				colorlessLen--;
				
				if(ColorIndex(*in) == 0)
				{
					// Disallow color black in names to prevent players
					// from getting advantage playing in front of black backgrounds
					outpos--;
					continue;
				}
			}
			else
			{
				spaces = 0;
				colorlessLen++;
			}
		}
		else
		{
			spaces = 0;
			colorlessLen++;
		}
		
		outpos++;
	}

	out[outpos] = '\0';

	// don't allow empty names
	if( *out == '\0' || colorlessLen == 0)
		Q_strncpyz(out, DEFAULT_PLAYER_NAME, outSize );
}

/*
===========
PlayerHandicap
============
*/
float PlayerHandicap( gplayer_t *player ) {
	char	userinfo[MAX_INFO_STRING];
	float	handicap;

	if (!player) {
		return 100;
	}

#ifdef TA_SP // HANDICAP
	if (g_singlePlayer.integer && !(g_entities[player - level.players].r.svFlags & SVF_BOT)) {
		// Humans don't use handicap in single player.
		return 100;
	}
#endif

	trap_GetUserinfo( player - level.players, userinfo, sizeof(userinfo) );

	handicap = atof( Info_ValueForKey( userinfo, "handicap" ) );
	if ( handicap < 1 || handicap > 100) {
		handicap = 100;
	}

	return handicap;
}


#ifdef TA_PLAYERSYS
/*
===========
G_LoadPlayer

Load animation.cfg
============
*/
void G_LoadPlayer(int playerNum, const char *inModelName, const char *inHeadModel)
{
    char *p;
    char model[MAX_QPATH]; // model name without skin
    char headModel[MAX_QPATH]; // head model name without skin
    gentity_t *ent;
    gplayer_t *player;
    bg_playercfg_t *playercfg;
#ifdef TA_WEAPSYS
    weapon_t oldDefault;
#endif
#ifdef TA_GAME_MODELS
    char filename[MAX_QPATH];
#endif

	ent = &g_entities[playerNum];
	player = ent->player;
	playercfg = &player->pers.playercfg;

    Q_strncpyz(model, inModelName, MAX_QPATH);
	Q_strncpyz(headModel, inHeadModel, MAX_QPATH);

    // Remove skin
	if ((p = strrchr(model, '/')) != 0) {
		*p = 0;
	}
	if ((p = strrchr(headModel, '/')) != 0) {
		*p = 0;
	}

#ifdef TA_GAME_MODELS
	// Load model (needed for tags / skeleton)
	// Game and cgame share the same models so an extra ClientUserinfoChanged is called on model reset (G_VidRestart),
	//   otherwise the model numbers may be incorrect after video restart.
	player->pers.torsoModel = 0;
	player->pers.legsModel = 0;

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper.md3", model );
	player->pers.torsoModel = trap_R_RegisterModel(filename);

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower.md3", model );
	player->pers.legsModel = trap_R_RegisterModel(filename);

	// Server doesn't have the player, fall back to default model.
	if (!player->pers.torsoModel) {
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper.md3", DEFAULT_MODEL );
		player->pers.torsoModel = trap_R_RegisterModel(filename);
	}
	if (!player->pers.legsModel) {
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower.md3", DEFAULT_MODEL );
		player->pers.legsModel = trap_R_RegisterModel(filename);
	}
#endif

	// Check if player has really changed!
	if ( Q_stricmpn(inModelName, playercfg->model, MAX_QPATH) == 0
		&& Q_stricmpn(inHeadModel, playercfg->headModel, MAX_QPATH) == 0 )
	{
		// no change
		return;
	}

	// ZTM: NOTE: This message was used to tell when a player get playercfg loaded.
	//G_Printf("DEBUG: Changed player old=%s, new=%s\n", playercfg->model, model);

	// Load animation.cfg
	if (!model[0] || !BG_LoadPlayerCFGFile(playercfg, model, headModel))
	{
		G_Printf("G_LoadPlayer: Loading player failed (%s)\n", inModelName);
		// Fall back to DEFAULT_MODEL
		Q_strncpyz(model, DEFAULT_MODEL, MAX_QPATH);
		if (!BG_LoadPlayerCFGFile(playercfg, model, headModel))
		{
			G_Printf("G_LoadPlayer: Loading default player failed (%s)\n", inModelName);
			// The defaults were loaded in BG_LoadPlayerCFGFile,
			//   so we should be able to continue...
		}
	}

	// Set model to "model[/skin]"
	Q_strncpyz(playercfg->model, inModelName, MAX_QPATH);
	Q_strncpyz(playercfg->headModel, inHeadModel, MAX_QPATH);

	VectorCopy (player->pers.playercfg.bbmins, player->ps.mins);
	VectorCopy (player->pers.playercfg.bbmaxs, player->ps.maxs);

#ifdef TA_WEAPSYS
	// DEFAULT_DEFAULT_WEAPON
	oldDefault = player->ps.stats[STAT_DEFAULTWEAPON];

	player->ps.stats[STAT_DEFAULTWEAPON] = playercfg->default_weapon;

#ifdef TA_WEAPSYS_EX
	// If not holding new default, change to it.
	if (player->ps.weapon != player->ps.stats[STAT_DEFAULTWEAPON])
	{
		// Check if it is the old default weapon
		if (player->ps.weapon == oldDefault)
		{
			// Change to new default
			player->ps.stats[STAT_PENDING_WEAPON] = player->ps.stats[STAT_DEFAULTWEAPON];

			// Don't allow the current weapon to be picked up, as it is the player's default.
			player->ps.stats[STAT_AMMO] = 0;
		}
	}
	else
	{
		// Only update "ammo"
		player->ps.stats[STAT_AMMO] = -1;
	}
#else
	// Set the ammo value.
	player->ps.ammo[playercfg->default_weapon] = -1;
#endif
#endif // TA_WEAPSYS
}
#endif

/*
===========
PlayerUserinfoChanged

Called from PlayerConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
*/
void PlayerUserinfoChanged( int playerNum ) {
	gentity_t *ent;
	int		teamTask, teamLeader;
	char	*s;
	char	model[MAX_QPATH];
	char	headModel[MAX_QPATH];
	char	oldname[MAX_STRING_CHARS];
	gplayer_t	*player;
	char	c1[MAX_INFO_STRING];
	char	c2[MAX_INFO_STRING];
	char	userinfo[MAX_INFO_STRING];

	ent = g_entities + playerNum;
	player = ent->player;

	trap_GetUserinfo( playerNum, userinfo, sizeof( userinfo ) );

	// check for malformed or illegal info strings
	if ( !Info_Validate(userinfo) ) {
		strcpy (userinfo, "\\name\\badinfo");
		// don't keep those players and userinfo
		trap_DropPlayer(playerNum, "Invalid userinfo");
	}

	// check the item prediction
	s = Info_ValueForKey( userinfo, "cg_predictItems" );
	if ( !atoi( s ) ) {
		player->pers.predictItemPickup = qfalse;
	} else {
		player->pers.predictItemPickup = qtrue;
	}

	// check the anti lag
	s = Info_ValueForKey( userinfo, "cg_antiLag" );
	player->pers.antiLag = atoi( s );

#ifdef TA_SP
	// Override names in main game and arcade mode.
	if (g_singlePlayer.integer && !(ent->r.svFlags & SVF_BOT)) {
		Info_SetValueForKey( userinfo, "name", va( "Player %d", player->pers.localPlayerNum + 1 ) );
	}
#endif

	// set name
	Q_strncpyz ( oldname, player->pers.netname, sizeof( oldname ) );
	s = Info_ValueForKey (userinfo, "name");
	PlayerCleanName( s, player->pers.netname, sizeof(player->pers.netname) );

	if ( player->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( player->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			Q_strncpyz( player->pers.netname, "scoreboard", sizeof(player->pers.netname) );
		}
	}

	if ( player->pers.connected == CON_CONNECTED ) {
		if ( strcmp( oldname, player->pers.netname ) ) {
			trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " renamed to %s\n\"", oldname, 
				player->pers.netname) );
		}
	}

	// set max health
#ifdef MISSIONPACK
	if (player->ps.powerups[PW_GUARD]) {
		player->pers.maxHealth = 200;
	} else {
		player->pers.maxHealth = PlayerHandicap( player );
	}
#else
	player->pers.maxHealth = PlayerHandicap( player );
#endif
	player->ps.stats[STAT_MAX_HEALTH] = player->pers.maxHealth;

	// set model
#ifdef TA_SP // SPMODEL
	if ( g_singlePlayer.integer ) {
		Q_strncpyz( model, Info_ValueForKey (userinfo, "spmodel"), sizeof( model ) );
		Q_strncpyz( headModel, Info_ValueForKey (userinfo, "spheadmodel"), sizeof( headModel ) );
	} else
#endif
#ifndef IOQ3ZTM_NO_TEAM_MODEL
	if( g_gametype.integer >= GT_TEAM ) {
		Q_strncpyz( model, Info_ValueForKey (userinfo, "team_model"), sizeof( model ) );
		Q_strncpyz( headModel, Info_ValueForKey (userinfo, "team_headmodel"), sizeof( headModel ) );
	} else
#endif
	{
		Q_strncpyz( model, Info_ValueForKey (userinfo, "model"), sizeof( model ) );
		Q_strncpyz( headModel, Info_ValueForKey (userinfo, "headmodel"), sizeof( headModel ) );
	}

#ifdef IOQ3ZTM // BLANK_HEADMODEL
	if (!headModel[0]) {
		Q_strncpyz( headModel, model, sizeof( headModel ) );
	}
#endif

#ifdef TA_PLAYERSYS
    G_LoadPlayer(playerNum, model, headModel);
#endif

#ifdef MISSIONPACK
	if (g_gametype.integer >= GT_TEAM) {
		player->pers.teamInfo = qtrue;
	} else {
		s = Info_ValueForKey( userinfo, "teamoverlay" );
		if ( ! *s || atoi( s ) != 0 ) {
			player->pers.teamInfo = qtrue;
		} else {
			player->pers.teamInfo = qfalse;
		}
	}
#else
	// teamInfo
	s = Info_ValueForKey( userinfo, "teamoverlay" );
	if ( ! *s || atoi( s ) != 0 ) {
		player->pers.teamInfo = qtrue;
	} else {
		player->pers.teamInfo = qfalse;
	}
#endif
	/*
	s = Info_ValueForKey( userinfo, "cg_pmove_fixed" );
	if ( !*s || atoi( s ) == 0 ) {
		player->pers.pmoveFixed = qfalse;
	}
	else {
		player->pers.pmoveFixed = qtrue;
	}
	*/

	// team task (0 = none, 1 = offence, 2 = defence)
	teamTask = atoi(Info_ValueForKey(userinfo, "teamtask"));
	// team Leader (1 = leader, 0 is normal player)
	teamLeader = player->sess.teamLeader;

	// colors
	strcpy(c1, Info_ValueForKey( userinfo, "color1" ));
	strcpy(c2, Info_ValueForKey( userinfo, "color2" ));

#ifdef TA_PLAYERSYS
	// In single player use player model's prefcolors
	// ZTM: TODO: Add option to use 'auto color' from player.
#ifdef TA_SP
	if (g_singlePlayer.integer)
#else
	if (g_gametype.integer == GT_SINGLE_PLAYER)
#endif
	{
		Com_sprintf(c1, sizeof (c1), "%d", player->pers.playercfg.prefcolor1);
		Com_sprintf(c2, sizeof (c2), "%d", player->pers.playercfg.prefcolor2);
	}
#endif

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	if (ent->r.svFlags & SVF_BOT)
	{
		s = va("n\\%s\\t\\%i\\model\\%s\\hmodel\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\skill\\%s\\tt\\%d\\tl\\%d",
			player->pers.netname, player->sess.sessionTeam, model, headModel, c1, c2, 
			player->pers.maxHealth, player->sess.wins, player->sess.losses,
			Info_ValueForKey( userinfo, "skill" ), teamTask, teamLeader );
	}
	else
	{
		s = va("n\\%s\\t\\%i\\model\\%s\\hmodel\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\tt\\%d\\tl\\%d",
			player->pers.netname, player->sess.sessionTeam, model, headModel, c1, c2, 
			player->pers.maxHealth, player->sess.wins, player->sess.losses, teamTask, teamLeader);
	}

	trap_SetConfigstring( CS_PLAYERS+playerNum, s );

	// this is not the userinfo, more like the configstring actually
	G_LogPrintf( "PlayerUserinfoChanged: %i %s\n", playerNum, s );
}


/*
===========
PlayerConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return NULL if the player should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the player will be sent the current gamestate
and will eventually get to PlayerBegin.

firstTime will be qtrue the very first time a player connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/
char *PlayerConnect( int playerNum, qboolean firstTime, qboolean isBot, int connectionNum, int localPlayerNum ) {
	char		*value;
//	char		*areabits;
	gplayer_t	*player;
	char		userinfo[MAX_INFO_STRING];
	gentity_t	*ent;
	qboolean	firstConnectionPlayer;

	ent = &g_entities[ playerNum ];

	trap_GetUserinfo( playerNum, userinfo, sizeof( userinfo ) );

	// Check if it's the first player on the client (i.e. not a splitscreen player)
	firstConnectionPlayer = ( level.connections[connectionNum].numLocalPlayers == 0 );

	if ( firstConnectionPlayer ) {
		// IP filtering
		// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=500
		// recommanding PB based IP / GUID banning, the builtin system is pretty limited
		// check to see if they are on the banned IP list
		value = Info_ValueForKey (userinfo, "ip");
		if ( G_FilterPacket( value ) ) {
			return "You are banned from this server.";
		}

		// we don't check password for bots and local client
		// NOTE: local client <-> "ip" "localhost"
		//   this means this client is not running in our current process
		if ( !isBot && (strcmp(value, "localhost") != 0) ) {
			// check for a password
			value = Info_ValueForKey (userinfo, "password");
			if ( g_password.string[0] && Q_stricmp( g_password.string, "none" ) &&
				strcmp( g_password.string, value) != 0) {
				return "Invalid password";
			}
		}
	} else {
#ifdef TA_SP
		// Don't allow extra splitscreen players in arcade mode.
		if ( g_singlePlayer.integer && g_gametype.integer != GT_SINGLE_PLAYER ) {
			return "Splitscreen not allowed in arcade mode.";
		}
#else
		// Don't allow splitscreen players in single player.
		if ( g_singlePlayer.integer ) {
			return "Splitscreen not allowed in single player.";
		}
#endif
	}

	// if a player reconnects quickly after a disconnect, the player disconnect may never be called, thus flag can get lost in the ether
	if (ent->inuse) {
		G_LogPrintf("Forcing disconnect on active player: %i\n", playerNum);
		// so lets just fix up anything that should happen on a disconnect
		PlayerDisconnect(playerNum);
	}
	// they can connect
	ent->player = level.players + playerNum;
	player = ent->player;

//	areabits = player->areabits;

	memset( player, 0, sizeof(*player) );

	player->pers.connected = CON_CONNECTING;
	player->pers.initialSpawn = qtrue;

	// update player connection info
	level.connections[connectionNum].numLocalPlayers++;
	level.connections[connectionNum].localPlayerNums[localPlayerNum] = playerNum;
	player->pers.connectionNum = connectionNum;
	player->pers.localPlayerNum = localPlayerNum;

	// check for local client
	value = Info_ValueForKey( userinfo, "ip" );
	if ( !strcmp( value, "localhost" ) ) {
		player->pers.localClient = qtrue;
	}

	if( isBot ) {
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if( !G_BotConnect( playerNum, !firstTime ) ) {
			return "BotConnectfailed";
		}
	}

	// read or initialize the session data
	if ( firstTime || level.newSession ) {
		G_InitSessionData( player, userinfo );
#ifdef TA_SP
		G_InitCoopSessionData( player );
#endif
	}
	G_ReadSessionData( player );

	// get and distribute relevent paramters
	G_LogPrintf( "PlayerConnect: %i\n", playerNum );
	PlayerUserinfoChanged( playerNum );

	// don't do the "xxx connected" messages if they were caried over from previous level
	if ( firstTime ) {
#ifdef TA_SP
    	// If it's single player don't add message.
		if (g_singlePlayer.integer) {
		} else
#endif
		if ( firstConnectionPlayer ) {
			trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " connected\n\"", player->pers.netname) );
		} else {
			trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " dropped in\n\"", player->pers.netname) );
		}
	}

	// count current players and rank for scoreboard
	CalculateRanks();

	// for statistics
//	player->areabits = areabits;
//	if ( !player->areabits )
//		player->areabits = trap_Alloc( (trap_AAS_PointReachabilityAreaIndex( NULL ) + 7) / 8, NULL );

	return NULL;
}

/*
===========
PlayerBegin

called when a player has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
void PlayerBegin( int playerNum ) {
	gentity_t	*ent;
	gplayer_t	*player;
	int			flags;

	ent = g_entities + playerNum;

	player = level.players + playerNum;

	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	}
	G_InitGentity( ent );
	ent->touch = 0;
	ent->pain = 0;
	ent->player = player;

	player->pers.connected = CON_CONNECTED;
	player->pers.enterTime = level.time;
	player->pers.teamState.state = TEAM_BEGIN;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	flags = player->ps.eFlags;
	memset( &player->ps, 0, sizeof( player->ps ) );
	player->ps.eFlags = flags;

#ifdef TA_PLAYERSYS
	G_LoadPlayer(playerNum, player->pers.playercfg.model, player->pers.playercfg.headModel);
#endif

#ifdef TA_SP
	// Drop dead clients on level load in main game.
	if (g_singlePlayer.integer && g_gametype.integer == GT_SINGLE_PLAYER)
	{
		// Get number of lives/continues.
		G_ReadCoopSessionData( player );

		// Check if player is dead.
		if (player->ps.persistant[PERS_LIVES] < 1 && player->ps.persistant[PERS_CONTINUES] < 1) {
			trap_DropPlayer(playerNum, "was dropped, game over.");
			return;
		}
	}
#endif

	// locate ent at a spawn point
	PlayerSpawn( ent );

#ifdef TA_SP
	G_ReadCoopSessionData( player );

	// In multiplayer make sure players have at least 3 lives.
	if (!g_singlePlayer.integer && g_gametype.integer == GT_SINGLE_PLAYER
		&& player->ps.persistant[PERS_LIVES] < 3)
	{
		player->ps.persistant[PERS_LIVES] = 3;
	}
#endif

	if ( player->pers.initialSpawn && !g_singlePlayer.integer ) {
		if ( g_gametype.integer != GT_TOURNAMENT ) {
			BroadcastTeamChange( player, -1 );
		}
	}
	player->pers.initialSpawn = qfalse;
	G_LogPrintf( "PlayerBegin: %i\n", playerNum );

	// count current players and rank for scoreboard
	CalculateRanks();
}

/*
===========
PlayerSpawn

Called every time a player is placed fresh in the world:
after the first PlayerBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
void PlayerSpawn(gentity_t *ent) {
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gplayer_t	*player;
	int		i;
	playerPersistant_t	saved;
	playerSession_t		savedSess;
	int		persistant[MAX_PERSISTANT];
	gentity_t	*spawnPoint;
	gentity_t *tent;
	int		flags;
	int		savedPing;
//	char	*savedAreaBits;
	int		accuracy_hits, accuracy_shots;
	int		eventSequence;

	index = ent - g_entities;
	player = ent->player;

	VectorClear(spawn_origin);

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this player
	if ( player->sess.sessionTeam == TEAM_SPECTATOR ) {
		spawnPoint = SelectSpectatorSpawnPoint ( 
						spawn_origin, spawn_angles);
	} else if (g_gametype.integer >= GT_CTF
#ifdef TA_SP // SP_BOSS
	|| (g_gametype.integer == GT_SINGLE_PLAYER && player->sess.sessionTeam != TEAM_FREE)
#endif
	) {
		// all base oriented team games use the CTF spawn points
		spawnPoint = SelectCTFSpawnPoint ( 
#ifdef TA_PLAYERSYS
						ent,
#endif
						player->sess.sessionTeam, 
						player->pers.teamState.state, 
						spawn_origin, spawn_angles,
						!!(ent->r.svFlags & SVF_BOT));
#ifdef TA_SP
	} else if (g_gametype.integer == GT_SINGLE_PLAYER) {
		spawnPoint = SelectSinglePlayerSpawnPoint(
#ifdef TA_PLAYERSYS
						ent,
#endif
						index, spawn_origin, spawn_angles,
						!!(ent->r.svFlags & SVF_BOT));
#endif
	}
	else
	{
		// the first spawn should be at a good looking spot
		if ( player->pers.initialSpawn && player->pers.localClient )
		{
			spawnPoint = SelectInitialSpawnPoint(
#ifdef TA_PLAYERSYS
								ent,
#endif
								spawn_origin, spawn_angles,
							     !!(ent->r.svFlags & SVF_BOT));
		}
		else
		{
			// don't spawn near existing origin if possible
			spawnPoint = SelectSpawnPoint ( 
#ifdef TA_PLAYERSYS
				ent,
#else
				player->ps.origin, 
#endif
				spawn_origin, spawn_angles, !!(ent->r.svFlags & SVF_BOT));
		}
	}
	player->pers.teamState.state = TEAM_ACTIVE;

#ifndef TURTLEARENA // NO_KAMIKAZE_ITEM
	// always clear the kamikaze flag
	ent->s.eFlags &= ~EF_KAMIKAZE;
#endif

	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = ent->player->ps.eFlags & (EF_TELEPORT_BIT | EF_VOTED | EF_TEAMVOTED);
	flags ^= EF_TELEPORT_BIT;

	// clear everything but the persistant data

	saved = player->pers;
	savedSess = player->sess;
	savedPing = player->ps.ping;
//	savedAreaBits = player->areabits;
	accuracy_hits = player->accuracy_hits;
	accuracy_shots = player->accuracy_shots;
	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		persistant[i] = player->ps.persistant[i];
	}
	eventSequence = player->ps.eventSequence;

	Com_Memset (player, 0, sizeof(*player));

	player->pers = saved;
	player->sess = savedSess;
	player->ps.ping = savedPing;
//	player->areabits = savedAreaBits;
	player->accuracy_hits = accuracy_hits;
	player->accuracy_shots = accuracy_shots;
	player->lastkilled_player = -1;

	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		player->ps.persistant[i] = persistant[i];
	}
	player->ps.eventSequence = eventSequence;
	// increment the spawncount so the client will detect the respawn
	player->ps.persistant[PERS_SPAWN_COUNT]++;
	player->ps.persistant[PERS_TEAM] = player->sess.sessionTeam;

#ifdef TURTLEARENA // POWERS
	if (g_teleportFluxTime.integer && player->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR
#ifdef TA_SP // In single-player/co-op, boss bots don't get PW_FLASHING
		&& !(g_gametype.integer == GT_SINGLE_PLAYER && player->ps.persistant[PERS_TEAM] != TEAM_FREE)
#endif
		)
	{
		player->ps.powerups[PW_FLASHING] = level.time + g_teleportFluxTime.integer * 1000;
	}
#endif

#ifdef TURTLEARENA // DROWNING
	player->ps.powerups[PW_AIR] = level.time + 31000;
#else
	player->airOutTime = level.time + 12000;
#endif

	// set max health
	player->pers.maxHealth = PlayerHandicap( player );
	// clear entity values
	player->ps.stats[STAT_MAX_HEALTH] = player->pers.maxHealth;
	player->ps.eFlags = flags;
	player->ps.contents = CONTENTS_BODY;
	player->ps.collisionType = ( g_playerCapsule.integer == 1 ) ? CT_CAPSULE : CT_AABB;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->player = &level.players[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;
	
#ifdef TA_PLAYERSYS
	VectorCopy (player->pers.playercfg.bbmins, player->ps.mins);
	VectorCopy (player->pers.playercfg.bbmaxs, player->ps.maxs);
#else
	VectorCopy (playerMins, player->ps.mins);
	VectorCopy (playerMaxs, player->ps.maxs);
#endif

	player->ps.playerNum = index;

#ifdef TA_SP
	if (g_gametype.integer == GT_SINGLE_PLAYER) {
		player->ps.holdableIndex = HI_NONE;
	}
	else
#endif
	{
#ifdef TA_HOLDSYS
#ifdef TURTLEARENA // HOLDABLE
		// Start with 10 shurikens!
		player->ps.holdable[HI_SHURIKEN] = 10;
		player->ps.holdableIndex = HI_SHURIKEN;
#else
		player->ps.holdableIndex = HI_NONE;
#endif
#endif
	}

#ifdef TA_WEAPSYS // ZTM: Respawn code. Start with default weapon. Set ammo values.
	// Set default weapon
#if defined TA_PLAYERSYS
	player->ps.stats[STAT_DEFAULTWEAPON] = player->pers.playercfg.default_weapon;
#else
	player->ps.stats[STAT_DEFAULTWEAPON] = DEFAULT_DEFAULT_WEAPON;
#endif

#ifdef TA_WEAPSYS_EX
	// Set default ammo values.
	player->ps.stats[STAT_AMMO] = -1;
	player->ps.stats[STAT_PENDING_AMMO] = -1;
	player->ps.stats[STAT_DROP_AMMO] = 0;
#else
	// Set default ammo values.
	{
		int i;
#ifdef TA_WEAPSYS
		int max = BG_NumWeaponGroups();
		for (i = 0; i < max; i++)
#else
		for (i = 0; i < MAX_WEAPONS; i++)
#endif
		{
			if (BG_WeapUseAmmo(i))
			{
				player->ps.ammo[i] = 0;
			}
			else
			{
				// Doesn't use ammo
				player->ps.ammo[i] = -1;
			}
		}
	}
#endif

	{
		// ZTM: Start with default weapon.
#ifndef TA_WEAPSYS_EX
		gitem_t *item = NULL;
#endif
		weapon_t weapon;

		// DEFAULT_DEFAULT_WEAPON
		weapon = player->ps.stats[STAT_DEFAULTWEAPON];
#ifdef TA_WEAPSYS_EX
		player->ps.stats[STAT_PENDING_WEAPON] = weapon;
#else
		player->ps.stats[STAT_WEAPONS] = ( 1 << weapon);
#endif

#ifdef TA_WEAPSYS_EX
		// Default weapon doesn't use ammo.
		player->ps.stats[STAT_AMMO] = -1;
#else
		if (weapon > 0)
		{
			item = BG_FindItemForWeapon( weapon );
		}

		if (!item || (item && item->quantity == 0))
		{
			// Weapon that doesn't use ammo.
			player->ps.ammo[weapon] = -1;
		}
		else
		{
			player->ps.ammo[weapon] = item->quantity;

			if ( g_gametype.integer == GT_TEAM )
			{
				player->ps.ammo[weapon] /= 2;
			}
		}
#endif
	}
#else
	player->ps.stats[STAT_WEAPONS] = ( 1 << WP_MACHINEGUN );
	if ( g_gametype.integer == GT_TEAM ) {
		player->ps.ammo[WP_MACHINEGUN] = 50;
	} else {
		player->ps.ammo[WP_MACHINEGUN] = 100;
	}

	player->ps.stats[STAT_WEAPONS] |= ( 1 << WP_GAUNTLET );
	player->ps.ammo[WP_GAUNTLET] = -1;
	player->ps.ammo[WP_GRAPPLING_HOOK] = -1;
#endif

#ifdef TURTLEARENA // no health countdown
	ent->health = player->ps.stats[STAT_HEALTH] = player->ps.stats[STAT_MAX_HEALTH];
#else
	// health will count down towards max_health
	ent->health = player->ps.stats[STAT_HEALTH] = player->ps.stats[STAT_MAX_HEALTH] + 25;
#endif

	G_SetOrigin( ent, spawn_origin );
	VectorCopy( spawn_origin, player->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	player->ps.pm_flags |= PMF_RESPAWNED;

	trap_GetUsercmd( player - level.players, &ent->player->pers.cmd );
	SetPlayerViewAngle( ent, spawn_angles );
	// don't allow full run speed for a bit
	player->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	player->ps.pm_time = 100;

	player->respawnTime = level.time;
	player->inactivityTime = level.time + g_inactivity.integer * 1000;
	player->latched_buttons = 0;

	// set default animations
#ifdef TA_WEAPSYS
	player->ps.torsoAnim = BG_TorsoStandForPlayerState(&player->ps, &player->pers.playercfg);
	player->ps.legsAnim = BG_LegsStandForPlayerState(&player->ps, &player->pers.playercfg);
#else
	player->ps.torsoAnim = TORSO_STAND;
	player->ps.legsAnim = LEGS_IDLE;
#endif

	if (!level.intermissiontime) {
		if (ent->player->sess.sessionTeam != TEAM_SPECTATOR) {
#ifdef TURTLEARENA // POWERS
			// Only kill box if player is solid.
			if (!player->ps.powerups[PW_FLASHING])
#endif
			G_KillBox(ent);
			// force the base weapon up
#ifdef TA_WEAPSYS // ZTM: Set ready weapon to default weapon.
			player->ps.weapon = player->ps.stats[STAT_DEFAULTWEAPON];
			player->ps.weaponHands = BG_WeaponHandsForWeaponNum(player->ps.stats[STAT_DEFAULTWEAPON]);
#else
			player->ps.weapon = WP_MACHINEGUN;
#endif
			player->ps.weaponstate = WEAPON_READY;
			// fire the targets of the spawn point
			G_UseTargets(spawnPoint, ent);
#ifndef TA_WEAPSYS_EX
			// select the highest weapon number available, after any spawn given items have fired
			player->ps.weapon = 1;

#ifdef TA_WEAPSYS
			for (i = BG_NumWeaponGroups() - 1 ; i > 0 ; i--)
#else
			for (i = WP_NUM_WEAPONS - 1 ; i > 0 ; i--)
#endif
			{
				if (player->ps.stats[STAT_WEAPONS] & (1 << i)) {
					player->ps.weapon = i;
					break;
				}
			}
#endif
			// positively link the player, even if the command times are weird
			VectorCopy(ent->player->ps.origin, ent->r.currentOrigin);

			tent = G_TempEntity(ent->player->ps.origin, EV_PLAYER_TELEPORT_IN);
			tent->s.playerNum = ent->s.playerNum;

			trap_LinkEntity (ent);
		}
	} else {
		// move players to intermission
		MovePlayerToIntermission(ent);
	}
	// run a player frame to drop exactly to the floor,
	// initialize animations and other things
	player->ps.commandTime = level.time - 100;
	ent->player->pers.cmd.serverTime = level.time;
	PlayerThink( ent-g_entities );

#if 1 // ZTM: FIXME: !!! is this suppose to exist? wasn't it removed in ioq3 !!!
	// positively link the player, even if the command times are weird
	if ( ent->player->sess.sessionTeam != TEAM_SPECTATOR ) {
		BG_PlayerStateToEntityState( &player->ps, &ent->s, qtrue );
		VectorCopy( ent->player->ps.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}
#endif

	// run the presend to set anything else, follow spectators wait
	// until all players have been reconnected after map_restart
	if ( ent->player->sess.spectatorState != SPECTATOR_FOLLOW ) {
		PlayerEndFrame( ent );
	}

	// clear entity state values
	BG_PlayerStateToEntityState( &player->ps, &ent->s, qtrue );

	// we don't want players being backward-reconciled to the place they died
	G_ResetHistory( ent );
}


/*
===========
PlayerDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropPlayer(), which will call this and do
server system housekeeping.
============
*/
void PlayerDisconnect( int playerNum ) {
	gentity_t	*ent;
	gentity_t	*tent;
	int			i;

	// cleanup if we are kicking a bot that
	// hasn't spawned yet
	G_RemoveQueuedBotBegin( playerNum );

	ent = g_entities + playerNum;
	if (!ent->player || ent->player->pers.connected == CON_DISCONNECTED) {
		return;
	}

	// stop any following players
	for ( i = 0 ; i < level.maxplayers ; i++ ) {
		if ( level.players[i].sess.sessionTeam == TEAM_SPECTATOR
			&& level.players[i].sess.spectatorState == SPECTATOR_FOLLOW
			&& level.players[i].sess.spectatorPlayer == playerNum ) {
			StopFollowing( &g_entities[i] );
		}
	}

	// send effect if they were completely connected
	if ( ent->player->pers.connected == CON_CONNECTED 
		&& ent->player->sess.sessionTeam != TEAM_SPECTATOR ) {
		tent = G_TempEntity( ent->player->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.playerNum = ent->s.playerNum;

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		TossPlayerItems( ent );
#ifdef MISSIONPACK
		TossPlayerPersistantPowerups( ent );
#ifdef MISSIONPACK_HARVESTER
		if( g_gametype.integer == GT_HARVESTER ) {
			TossPlayerCubes( ent );
		}
#endif
#endif

	}

	G_LogPrintf( "PlayerDisconnect: %i\n", playerNum );

	// if we are playing in tourney mode and losing, give a win to the other player
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& !level.intermissiontime
		&& !level.warmupTime && level.sortedPlayers[1] == playerNum ) {
		level.players[ level.sortedPlayers[0] ].sess.wins++;
		PlayerUserinfoChanged( level.sortedPlayers[0] );
	}

	if( g_gametype.integer == GT_TOURNAMENT &&
		ent->player->sess.sessionTeam == TEAM_FREE &&
		level.intermissiontime ) {

		trap_Cmd_ExecuteText( EXEC_APPEND, "map_restart 0\n" );
		level.restarted = qtrue;
		level.changemap = NULL;
		level.intermissiontime = 0;
	}

	trap_UnlinkEntity (ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->player->pers.connected = CON_DISCONNECTED;
	ent->player->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->player->sess.sessionTeam = TEAM_FREE;

	trap_SetConfigstring( CS_PLAYERS + playerNum, "");

	CalculateRanks();

	if ( ent->r.svFlags & SVF_BOT ) {
		BotAIShutdownPlayer( playerNum, qfalse );
	}

	// clear player connection info
	level.connections[ent->player->pers.connectionNum].numLocalPlayers--;
	level.connections[ent->player->pers.connectionNum].localPlayerNums[ent->player->pers.localPlayerNum] = -1;
	ent->player->pers.localPlayerNum = ent->player->pers.connectionNum = -1;
}

#ifdef TA_SP
/*
===========
G_ClientCompletedLevel
============
*/
qboolean G_ClientCompletedLevel(gentity_t *activator, char *nextMap)
{
	char buf[MAX_QPATH];

	if (g_gametype.integer != GT_SINGLE_PLAYER) {
		// Only exit in single player.
		return qfalse;
	}

	// If trigered by a player, that isn't a spectator or boss,
	//  and that hasn't finished the level.
	if (!(activator && activator->player
		&& activator->player->sess.sessionTeam == TEAM_FREE
		&& !activator->player->finishTime))
	{
		// Not a valid player.
		return qfalse;
	}

	// Set finish time.
	activator->player->finishTime = level.time;
	activator->player->ps.eFlags = EF_FINISHED;
	activator->s.eFlags |= EF_FINISHED;

	// Print message
	if (!g_singlePlayer.integer)
	{
		G_Printf("%s finished the level.\n", activator->player->pers.netname );
	}

	if (nextMap == NULL || *nextMap == '\0')
	{
		char	mapname[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer( "mapname", mapname, sizeof(mapname) );

		// Save mapname
		trap_Cvar_Set("g_saveMapname", mapname);

		trap_Cvar_Set("nextmap", "map_restart");
		return qtrue;
	}

	// Reached the end of the single player levels
	if (Q_stricmp(nextMap, "sp_end") == 0)
	{
		if (g_singlePlayer.integer)
		{
			// Return to the title screen.
			trap_Cvar_Set("nextmap", "disconnect; sp_complete");
			return qtrue;
		}
		else
		{
			const char *info;

			nextMap = NULL;

#ifdef IOQ3ZTM // MAP_ROTATION
			info = G_GetNextArenaInfoByGametype(NULL, GT_SINGLE_PLAYER);
			if (info) {
				nextMap = Info_ValueForKey(info, "map");
			}
#endif

			if (!nextMap || !strlen(nextMap))
			{
				// Default to sp1a1
				nextMap = "sp1a1";
			}
		}
	}

	// Save name of next map, for coop session data and save games.
	trap_Cvar_Set("g_saveMapname", nextMap);

	// Set cvar for level change.
	Com_sprintf(buf, MAX_QPATH, "map %s", nextMap);
	trap_Cvar_Set("nextmap", buf);
	return qtrue;
}
#endif

#ifdef NIGHTSMODE
/*
===========
NiGHTS mode
============
*/

#define MAX_MARES 10

// Go out of Nights mode, nights time ran out.
void G_DeNiGHTSizePlayer( gentity_t *ent )
{
	if (!ent || !ent->player)
		return;

	if (ent->player->ps.powerups[PW_FLIGHT]) {
		// Clear score
		ent->player->ps.persistant[PERS_SCORE] = 0;
	}

	ent->player->mare = 0;
	ent->player->ps.powerups[PW_FLIGHT] = 0;
	ent->player->ps.eFlags &= ~EF_BONUS;

	G_SetupPath(ent, NULL);
}

int G_NumMares(void)
{
	int mare;
	int i;

	mare = 0;

	for (i = 1; i < MAX_MARES; i++)
	{
		if (!G_Find( NULL, FOFS(targetname), va("mare_start_%d", i) ))
			break;

		mare++;
	}

	return mare;
}

// Go into Nights mode
void G_NiGHTSizePlayer( gentity_t *ent, gentity_t *drone )
{
	int mare;
	gentity_t *target;

	if (!ent || !ent->player)
		return;

	// Find lowest mare with a undead nights_target
	//     find mare_start_1 thought mare_start_9 and look for nights_target,
	//		if found and alive use path.
	for (mare = 1; mare < MAX_MARES; mare++)
	{
		if ((target = G_Find( NULL, FOFS(targetname), va("nights_target%d", mare) )))
		{
			if (target->health)
				break;
		}
	}

	if (mare == MAX_MARES)
	{
		// Use targets when all nights_target are destroyed
		G_UseTargets2(drone, ent, drone->paintarget);

		// No nights_targets left, do level end.
		if (!G_ClientCompletedLevel(ent, drone->message)) {
			G_DeNiGHTSizePlayer(ent);
		}
		return;
	}

	if (mare == ent->player->mare)
		return;

	// Use targets
	G_UseTargets2(target, ent, target->paintarget);

	ent->player->mare = mare;

	//G_Printf("DEBUG: Entered new mare %d\n", mare);

	if (!G_Find( NULL, FOFS(targetname), va("mare_start_%d", mare) )
		|| G_SetupPath(ent, va("mare_start_%d", mare)) == PATH_ERROR)
	{
		if (ent->player->mare == 1)
			return;

		ent->player->mare = 1;
		if (G_SetupPath(ent, "mare_start_1") == PATH_ERROR)
		{
			return;
		}
	}

	ent->player->ps.powerups[PW_FLIGHT] = level.time + 120 * 1000;
	G_ReachedPath(ent, qfalse);
}

void Drone_Touch(gentity_t *self, gentity_t *other, trace_t *trace )
{
	if (!other->player)
		return;

	if (!other->player->ps.powerups[PW_FLIGHT]) {
		// Use targets first time a player NiGHTSizes
		G_UseTargets(self, other);
	}

	other->player->ps.eFlags &= ~EF_BONUS;
	G_NiGHTSizePlayer(other, self);
}

// Ideya Drone
void SP_nights_start( gentity_t *ent )
{
	gitem_t *item;

	// Touch this to go into NiGHTS mode
	//    and go to current mare.

	VectorSet( ent->s.mins, -15, -15, -15 );
	VectorSet( ent->s.maxs, 15, 15, 15 );

	ent->s.eType = ET_GENERAL;
	ent->flags = FL_NO_KNOCKBACK;
	ent->s.contents = CONTENTS_TRIGGER;
	ent->touch = Drone_Touch;

	// Check for invalid map name on spawn, easier to find bugs in maps.
	if (ent->message == NULL || *ent->message == '\0')
	{
		// Invalid map name.
		G_Printf("nights_start: Missing map name.\n");
	}

	// ZTM: TODO: In Single Player remove model when player is NiGHTS, and put it back when they lose NiGHTS mode
	item = BG_FindItemForPowerup(PW_FLIGHT);
	if (item) {
		ent->s.modelindex = G_ModelIndex(item->world_model[0]);
	}

	G_SetOrigin( ent, ent->s.origin );

	trap_LinkEntity(ent);
}

void Capture_Touch(gentity_t *self, gentity_t *other, trace_t *trace )
{
	int amount;

	if (!other->player)
		return;

	amount = other->player->ps.stats[STAT_SPHERES];
	if (amount > self->health)
		amount = self->health;

	self->health -= amount;
	other->player->ps.stats[STAT_SPHERES] -= amount;

	if (self->health <= 0)
	{
		self->s.modelindex = 0; // remove model to show it is dead.

		if (other->player->ps.powerups[PW_FLIGHT])
		{
			// Bonus time!
			other->player->ps.eFlags |= EF_BONUS;
		}

		// Use targets
		G_UseTargets2(self, other, self->paintarget);
	}
}

// Ideya Capture
// Touch to use collected Spheres to damage Ideya Capture
void SP_nights_target( gentity_t *ent )
{
	gitem_t *item;

	VectorSet( ent->s.mins, -15, -15, -15 );
	VectorSet( ent->s.maxs, 15, 15, 15 );

	ent->s.eType = ET_GENERAL;
	ent->flags = FL_NO_KNOCKBACK;
	ent->s.contents = CONTENTS_TRIGGER;
	ent->touch = Capture_Touch;

	if (!ent->health) {
		ent->health = 20;
	}

	// Temporary model
	item = BG_FindItemForPowerup(PW_INVUL);
	if (item) {
		ent->s.modelindex = G_ModelIndex(item->world_model[0]);
	}

	G_SetOrigin( ent, ent->s.origin );

	trap_LinkEntity(ent);
}
#endif
