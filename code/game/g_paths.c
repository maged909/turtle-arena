/*
===========================================================================
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
Copyright (C) 2009-2010 Zack Middleton

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

#include "g_local.h"

/* ZTM: TODO: Finish new general path code,
		for trains, camera "scripts", NPCs, NiGHTS mode?, 2D mode?
		and what not.

	Supports trains
		players in NiGHTS mode are mostly-working (I don't think the problems are in g_path)
		others are untested
*/

#ifdef TA_PATHSYS
typedef enum
{
	PATHF_BEGIN		= 0x01, // path can start on this entity
	PATHF_POINT		= 0x02, // unused
	PATHF_AXIS		= 0x04, // unused Rotate around point See func_rotating
	PATHF_FINISH	= 0x08, // Last entity in path
	PATHF_CANFINISH	= 0x10, // Doesn't give error if its last in the path

	PATHF_DUMMY
} pathinfoflags_e;

typedef struct
{
	char *classname;
	int flags; // pathinfoflags_e

} gpathinfo_t;

// The first must have a non-NULL classname
gpathinfo_t gpathinfo[] =
{
	// Quake 3 (and etc) compatibility
	{ "path_corner", PATHF_BEGIN | PATHF_POINT },

	// TDC_NPC compatiblity...
	{ "npcpath", PATHF_BEGIN | PATHF_POINT | PATHF_CANFINISH },

	// Turtle Arena path entities
	{ "path_start", PATHF_BEGIN | PATHF_POINT }, // Is there a reason for "start"?
	{ "path_point", PATHF_POINT },
	{ "path_axis", PATHF_AXIS }, // ZTM: TODO: Like SRB2's MT_AXIS, for NiGHTS mode. (entity is point to be rotated around)
	{ "path_end", PATHF_FINISH | PATHF_POINT }, // A end is not needed for circit paths

#ifdef NIGHTSMODE
	// NiGHTS
	{ "nights_start", PATHF_BEGIN | PATHF_POINT },
	{ "nights_target", PATHF_POINT },
#endif

	// end of list marker
	{ NULL, 0 }
};

void SP_path_start( gentity_t *ent )
{
	const char *styles[] = {"none","side","top","back", NULL}; // PATHMODE_*
	char *s;
	int i;

	// Set default path movement style
	ent->moverState = PATHMODE_SIDE;

	G_SpawnString("style", NULL, &s);
	if (s && *s) {
		for (i = 0; styles[i] != NULL; i++) {
			if (Q_stricmpn(s, styles[i], strlen(styles[i])) == 0) {
				ent->moverState = i;
				break;
			}
		}
		if (styles[i] == NULL) {
			Com_Printf("Warning: Unknown path style %s\n", s);
		}
	}

	// Setup path to avoid freeze in game?
}

/*
===========
G_SetupPath

ent is the who is going to use the path
target is the targetname of the first path entity in the path

This function also supports pervTrain, so that paths can be two way.

Based on Q3's g_mover.c's Think_SetupTrainTargets
===========
*/
static gpathtype_e G_SetupPath2(gentity_t *ent, const char *target)
{
	gentity_t		*path, *next, *start;
	int 			i = 0, prev_flags = 0;
	qboolean pathNotBegin = qfalse; // qtrue it trying to start a path entity without PATHF_BEGIN

	if (!ent) {
		return PATH_ERROR;
	}

	ent->prevTrain = NULL;
	ent->nextTrain = NULL;

	if (!target || target[0] == '\0') {
		// This is supported, so its not really an error...
		return PATH_ERROR;
	}

	// Only start path on a path entity not just the first one we find,
	//    also support PATHF_BEGIN
	do {
		ent->nextTrain = G_Find( ent->nextTrain, FOFS(targetname), target );
		if ( !ent->nextTrain )
		{
			if (pathNotBegin)
				G_Printf( "Entity using paths at %s with a target of wrong path type\n",
					vtos(ent->r.absmin) );
			else
				G_Printf( "Entity using paths at %s with an unfound target\n",
					vtos(ent->r.absmin) );
			return PATH_ERROR;
		}

		// Check for all path types, not just "path_corner"
		for (i=0; gpathinfo[i].classname; i++)
		{
			if (!strcmp( ent->nextTrain->classname, gpathinfo[i].classname ))
			{
				if (gpathinfo[i].flags & PATHF_BEGIN)
				{
					pathNotBegin = qfalse;
					break;
				}
				else
				{
					pathNotBegin = qtrue;
				}
			}
		}
	} while ( !gpathinfo[i].classname );

	// First two path entities are linked, and the second one's prev is the first one.
	// This means that the path was already setup,
	if (ent->nextTrain->nextTrain &&
		ent->nextTrain->nextTrain->prevTrain == ent->nextTrain
		&& (ent->nextTrain->pathflags & PATH_FIRST))
	{
		// The path's nextTrain should only be set here is this should be fine,
		//  unless somehow a mapper changed the path after the train was started...

		if (ent->nextTrain->pathflags & PATH_CIRCIT)
			return PATH_CIRCIT;
		else if (ent->nextTrain->pathflags & PATH_LINE)
			return PATH_LINE;
		else
			return PATH_ERROR;
	}

	start = NULL;
	next = NULL;
	for ( path = ent->nextTrain ; path != start ; path = next ) {
		if ( !start ) {
			start = path;
			start->prevTrain = NULL;
			start->pathflags = PATH_FIRST;
		}

		if ( !path->target )
		{
			// End path properly
			start->pathflags = PATH_LINE | PATH_FIRST;
			path->pathflags = PATH_LINE | PATH_LAST;
			path->nextTrain = NULL;

			if (prev_flags & PATHF_CANFINISH)
			{
				return PATH_LINE; // path ended normally.
			}

			G_Printf( "Path entity at %s without a target\n",
				vtos(path->s.origin) );
			return PATH_ERROR;
		}

		// find a path_corner among the targets
		// there may also be other targets that get fired when the corner
		// is reached
		next = NULL;
		do {
			next = G_Find( next, FOFS(targetname), path->target );
			if ( !next )
			{
				// End path properly
				start->pathflags = PATH_LINE | PATH_FIRST;
				path->pathflags = PATH_LINE | PATH_LAST;
				path->nextTrain = NULL;

				if (prev_flags & PATHF_CANFINISH)
				{
					return PATH_LINE; // path ended normally.
				}

				G_Printf( "Path entity at %s without a target path entity\n",
					vtos(path->s.origin) );
				return PATH_ERROR;
			}

			// Check for all path types, not just "path_corner"
			for (i=0; gpathinfo[i].classname; i++)
			{
				if (!strcmp( next->classname, gpathinfo[i].classname ) )
					break;
			}
		} while ( !gpathinfo[i].classname );

		path->nextTrain = next;
		// Link prevTrain so we can go backward on the path too.
		next->prevTrain = path;

		// If it found a valid path entity and its end the path,
		//  quit linking.
		if (gpathinfo[i].flags & PATHF_FINISH)
		{
			start->pathflags = PATH_LINE | PATH_FIRST;
			next->pathflags = PATH_LINE | PATH_LAST;
			next->nextTrain = NULL;
			ent->prevTrain = NULL;
			return PATH_LINE;
		}
		if ((gpathinfo[i].flags & PATHF_CANFINISH) && !next->target)
		{
			start->pathflags = PATH_LINE | PATH_FIRST;
			next->pathflags = PATH_LINE | PATH_LAST;
			next->nextTrain = NULL;
			ent->prevTrain = NULL;
			return PATH_LINE;
		}

		prev_flags = gpathinfo[i].flags;
	}

	// If we made it here the path is a circit, so link the first and last.
	start->pathflags = PATH_CIRCIT | PATH_FIRST;
	start->prevTrain->pathflags = PATH_CIRCIT | PATH_LAST;

	ent->nextTrain = start;
	ent->prevTrain = start->prevTrain;

	return PATH_CIRCIT;
}

// ZTM: NOTE: G_SetupPath had too many return statements, so moved EF_PATHMODE bit to a seperate function.
gpathtype_e G_SetupPath(gentity_t *ent, const char *target)
{
	gpathtype_e type;

	// ZTM: TODO: Don't setup path if client is already following it.
	type = G_SetupPath2(ent, target);

	if (type == PATH_ERROR) {
		// Disable path mode
		if (ent->client) {
			ent->client->ps.pathMode = PATHMODE_NONE;
			ent->client->ps.eFlags &= ~EF_PATHMODE;
		}
		ent->s.eFlags &= ~EF_PATHMODE;
	} else {
		// Enable path mode
		if (ent->client) {
			// Set pathMode to the mode of the first entity in the path
			ent->client->ps.pathMode = ent->nextTrain->moverState;
			ent->client->ps.eFlags |= EF_PATHMODE;
		}
		ent->s.eFlags |= EF_PATHMODE;
	}

	return type;
}

// g_mover.c
void Think_BeginMoving( gentity_t *ent );
void SetMoverState( gentity_t *ent, moverState_t moverState, int time );

/*
===========
G_ReachedPath
===========
*/
qboolean G_ReachedPath(gentity_t *ent, qboolean check)
{
	gentity_t	*point;
	qboolean	backward;

	if (!(ent->s.eFlags & EF_PATHMODE)) {
		return qfalse;
	}

	if (ent->client) {
		backward = (ent->client->ps.eFlags & EF_TRAINBACKWARD);
	} else {
		backward = (ent->s.eFlags & EF_TRAINBACKWARD);
	}

	if (backward) {
		point = ent->prevTrain;
	} else {
		point = ent->nextTrain;
	}

	if (!point && ent->client) {
		// Previous point
		if (backward)
			point = ent->nextTrain;
		else
			point = ent->prevTrain;

		// Stop train
		return qfalse;
	} else if (!point && !ent->client) {
		// end of path
		if (!(ent->spawnflags & 1)) {
			// Stop train
			return qfalse;
		}

		// Previous point
		if (backward)
			point = ent->nextTrain;
		else
			point = ent->prevTrain;

		// Go back the way you came
		backward = !backward;
		ent->s.eFlags ^= EF_TRAINBACKWARD;

		// Next point
		if (backward)
			point = ent->prevTrain;
		else
			point = ent->nextTrain;

		if (!point) {
			return qfalse;
		}
	}

	if ((!backward && point == point->nextTrain) || (backward && point == point->prevTrain)) {
		// Entity points to self...
		G_Printf("DEBUG: Entity points to self!\n");
		return qfalse;
	}

	// ZTM: Check if we have made it to the next train
	//               Doesn't work with PATHF_AXIS!
	if (check)
	{
		vec3_t targetPos;
		vec3_t origin;
		vec_t dist;

		VectorCopy(point->s.origin, targetPos);

		if (ent->client)
			VectorCopy(ent->client->ps.origin, origin);
		else
			VectorCopy(ent->s.origin, origin);

		if (ent->client && ent->client->ps.pathMode == PATHMODE_SIDE) {
			// "2D" path
			origin[2] = targetPos[2] = 0; // Don't compare Z
		}

		dist = Distance(origin, targetPos);

		if (dist > 20.0f) {
			return qfalse;
		}
	}

	// fire all other targets
	G_UseTargets( point, ent );

	// Setup next move
	if (backward) {
		ent->prevTrain = point->prevTrain;
		if (ent->prevTrain) {
			ent->nextTrain = point;
		}
	} else {
		ent->nextTrain = point->nextTrain;
		if (ent->nextTrain) {
			ent->prevTrain = point;
		}
	}

	// set the new trajectory
	if (ent->prevTrain) {
		VectorCopy( ent->prevTrain->s.origin, ent->pos1 );
	}
	if (ent->nextTrain) {
		VectorCopy( ent->nextTrain->s.origin, ent->pos2 );
	}

	if (ent->s.eType == ET_MOVER)
	{
		float			speed;
		vec3_t			move;
		float			length;

		// if the path_corner has a speed, use that
		if ( point->speed ) {
			speed = point->speed;
		} else {
			// otherwise use the train's speed
			speed = ent->speed;
		}
		if ( speed < 1 ) {
			speed = 1;
		}

		// calculate duration
		VectorSubtract( ent->pos2, ent->pos1, move );
		length = VectorLength( move );

		ent->s.pos.trDuration = length * 1000 / speed;

		// Tequila comment: Be sure to send to clients after any fast move case
		ent->r.svFlags &= ~SVF_NOCLIENT;

		// Tequila comment: Fast move case
		if(ent->s.pos.trDuration<1) {
			// Tequila comment: As trDuration is used later in a division, we need to avoid that case now
			// With null trDuration,
			// the calculated rocks bounding box becomes infinite and the engine think for a short time
			// any entity is riding that mover but not the world entity... In rare case, I found it
			// can also stuck every map entities after func_door are used.
			// The desired effect with very very big speed is to have instant move, so any not null duration
			// lower than a frame duration should be sufficient.
			// Afaik, the negative case don't have to be supported.
			ent->s.pos.trDuration=1;

			// Tequila comment: Don't send entity to clients so it becomes really invisible 
			ent->r.svFlags |= SVF_NOCLIENT;
		}

		// looping sound
		ent->s.loopSound = point->soundLoop;

		// start it going
		if (backward)
			SetMoverState( ent, MOVER_2TO1, level.time );
		else
			SetMoverState( ent, MOVER_1TO2, level.time );

		// if there is a "wait" value on the target, don't start moving yet
		if ( point->wait ) {
			ent->nextthink = level.time + point->wait * 1000;
			ent->think = Think_BeginMoving;
			ent->s.pos.trType = TR_STATIONARY;
		}
	}
	else if (ent->client)
	{
		vec3_t dir;
		vec3_t viewAngles;
		int mode;

		// Set path movement style
		mode = point->moverState;

		// Unset style means use pervious style.
		if (!mode) {
			mode = ent->client->ps.pathMode;
		}

		VectorCopy( ent->pos1, ent->client->ps.grapplePoint );
		VectorCopy( ent->pos2, ent->client->ps.nextPoint );

		if (backward)
			VectorSubtract( ent->pos1, ent->client->ps.origin, dir );
		else
			VectorSubtract( ent->pos2, ent->client->ps.origin, dir );

		vectoangles( dir, viewAngles );
		viewAngles[ROLL] = ent->client->ps.viewangles[ROLL];
		viewAngles[PITCH] = ent->client->ps.viewangles[PITCH];
		SetClientViewAngle(ent, viewAngles);

		if (mode == PATHMODE_SIDE) {
			if (backward) {
				ent->client->ps.stats[STAT_DEAD_YAW] = viewAngles[YAW]-90;
			} else {
				ent->client->ps.stats[STAT_DEAD_YAW] = viewAngles[YAW]+90;
			}
		} else {
			ent->client->ps.stats[STAT_DEAD_YAW] = viewAngles[YAW];
		}

		if (mode != ent->client->ps.pathMode) {
			// ZTM: Do we need to do anything when the mode changes?
			ent->client->ps.pathMode = mode;
		}
	}

	return qtrue;
}

void G_MoveOnPath(gentity_t *ent)
{
	if (!ent)
		return;

	G_ReachedPath(ent, qtrue);
}
#endif
