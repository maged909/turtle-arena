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



/*
===============================================================================

PUSHMOVE

===============================================================================
*/

typedef struct {
	gentity_t	*ent;
	vec3_t	origin;
	vec3_t	angles;
	float	deltayaw;
} pushed_t;
pushed_t	pushed[MAX_GENTITIES], *pushed_p;


/*
============
G_TestEntityPosition

============
*/
gentity_t	*G_TestEntityPosition( gentity_t *ent ) {
	trace_t	tr;
	qboolean	capsule;
	vec3_t	origin;
	int		mask;

#ifdef IOQ3ZTM
	// shrink bounds so it is not coplanar,
	// otherwise may result in startsolid when it should not.
	if (ent->s.bmodel) {
		ent->s.mins[0] += 2;
		ent->s.mins[1] += 2;
		ent->s.mins[2] += 2;
		ent->s.maxs[0] -= 2;
		ent->s.maxs[1] -= 2;
		ent->s.maxs[2] -= 2;
	}
#endif

	if ( ent->client ) {
		VectorCopy( ent->client->ps.origin, origin );
		capsule = ent->client->ps.capsule;
	} else {
		VectorCopy( ent->s.pos.trBase, origin );
		capsule = ent->s.capsule;
	}

	if ( ent->clipmask ) {
		mask = ent->clipmask;
	} else {
#ifdef IOQ3ZTM // Don't push brushes inside of players!
		mask = MASK_SOLID | CONTENTS_BODY;
#else
		mask = MASK_SOLID;
#endif
	}

	if ( capsule ) {
		trap_TraceCapsule( &tr, origin, ent->s.mins, ent->s.maxs, origin, ent->s.number, mask );
	} else {
		trap_Trace( &tr, origin, ent->s.mins, ent->s.maxs, origin, ent->s.number, mask );
	}
	
#ifdef IOQ3ZTM
	if (ent->s.bmodel) {
		ent->s.mins[0] -= 2;
		ent->s.mins[1] -= 2;
		ent->s.mins[2] -= 2;
		ent->s.maxs[0] += 2;
		ent->s.maxs[1] += 2;
		ent->s.maxs[2] += 2;
	}
#endif

	if (tr.startsolid)
		return &g_entities[ tr.entityNum ];
		
	return NULL;
}

/*
================
G_CreateRotationMatrix
================
*/
void G_CreateRotationMatrix(vec3_t angles, vec3_t matrix[3]) {
	AngleVectors(angles, matrix[0], matrix[1], matrix[2]);
	VectorInverse(matrix[1]);
}

/*
================
G_TransposeMatrix
================
*/
void G_TransposeMatrix(vec3_t matrix[3], vec3_t transpose[3]) {
	int i, j;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			transpose[i][j] = matrix[j][i];
		}
	}
}

/*
================
G_RotatePoint
================
*/
void G_RotatePoint(vec3_t point, vec3_t matrix[3]) {
	vec3_t tvec;

	VectorCopy(point, tvec);
	point[0] = DotProduct(matrix[0], tvec);
	point[1] = DotProduct(matrix[1], tvec);
	point[2] = DotProduct(matrix[2], tvec);
}

/*
==================
G_TryPushingEntity

Returns qfalse if the move is blocked
==================
*/
qboolean	G_TryPushingEntity( gentity_t *check, gentity_t *pusher, vec3_t move, vec3_t amove ) {
	vec3_t		matrix[3], transpose[3];
	vec3_t		org, org2, move2;
	gentity_t	*block;

	// EF_MOVER_STOP will just stop when contacting another entity
	// instead of pushing it, but entities can still ride on top of it
	if ( ( pusher->s.eFlags & EF_MOVER_STOP ) && 
		check->s.groundEntityNum != pusher->s.number ) {
		return qfalse;
	}

	// save off the old position
	if (pushed_p > &pushed[MAX_GENTITIES]) {
		G_Error( "pushed_p > &pushed[MAX_GENTITIES]" );
	}
	pushed_p->ent = check;
	VectorCopy (check->s.pos.trBase, pushed_p->origin);
	VectorCopy (check->s.apos.trBase, pushed_p->angles);
	if ( check->client ) {
		pushed_p->deltayaw = check->client->ps.delta_angles[YAW];
		VectorCopy (check->client->ps.origin, pushed_p->origin);
	}
	pushed_p++;

	// try moving the contacted entity 
	// figure movement due to the pusher's amove
	G_CreateRotationMatrix( amove, transpose );
	G_TransposeMatrix( transpose, matrix );
	if ( check->client ) {
		VectorSubtract (check->client->ps.origin, pusher->r.currentOrigin, org);
	}
	else {
		VectorSubtract (check->s.pos.trBase, pusher->r.currentOrigin, org);
	}
	VectorCopy( org, org2 );
	G_RotatePoint( org2, matrix );
	VectorSubtract (org2, org, move2);
	// add movement
	VectorAdd (check->s.pos.trBase, move, check->s.pos.trBase);
	VectorAdd (check->s.pos.trBase, move2, check->s.pos.trBase);
	if ( check->client ) {
		VectorAdd (check->client->ps.origin, move, check->client->ps.origin);
		VectorAdd (check->client->ps.origin, move2, check->client->ps.origin);
		// make sure the client's view rotates when on a rotating mover
		check->client->ps.delta_angles[YAW] += ANGLE2SHORT(amove[YAW]);
	}

	// may have pushed them off an edge
	if ( check->s.groundEntityNum != pusher->s.number ) {
		check->s.groundEntityNum = ENTITYNUM_NONE;
	}

	block = G_TestEntityPosition( check );
	if (!block) {
		// pushed ok
		if ( check->client ) {
			VectorCopy( check->client->ps.origin, check->r.currentOrigin );
		} else {
			VectorCopy( check->s.pos.trBase, check->r.currentOrigin );
		}
		trap_LinkEntity (check);
		return qtrue;
	}

	// if it is ok to leave in the old position, do it
	// this is only relevent for riding entities, not pushed
	// Sliding trapdoors can cause this.
	VectorCopy( (pushed_p-1)->origin, check->s.pos.trBase);
	if ( check->client ) {
		VectorCopy( (pushed_p-1)->origin, check->client->ps.origin);
	}
	VectorCopy( (pushed_p-1)->angles, check->s.apos.trBase );
	block = G_TestEntityPosition (check);
	if ( !block ) {
		check->s.groundEntityNum = ENTITYNUM_NONE;
		pushed_p--;
		return qtrue;
	}

	// blocked
	return qfalse;
}

/*
==================
G_CheckProxMinePosition
==================
*/
qboolean G_CheckProxMinePosition( gentity_t *check ) {
	vec3_t		start, end;
	trace_t	tr;

	VectorMA(check->s.pos.trBase, 0.125, check->movedir, start);
	VectorMA(check->s.pos.trBase, 2, check->movedir, end);
	trap_Trace( &tr, start, NULL, NULL, end, check->s.number, MASK_SOLID );
	
	if (tr.startsolid || tr.fraction < 1)
		return qfalse;

	return qtrue;
}

/*
==================
G_TryPushingProxMine
==================
*/
qboolean G_TryPushingProxMine( gentity_t *check, gentity_t *pusher, vec3_t move, vec3_t amove ) {
	vec3_t		forward, right, up;
	vec3_t		org, org2, move2;
	int ret;

	// we need this for pushing things later
	VectorSubtract (vec3_origin, amove, org);
	AngleVectors (org, forward, right, up);

	// try moving the contacted entity 
	VectorAdd (check->s.pos.trBase, move, check->s.pos.trBase);

	// figure movement due to the pusher's amove
	VectorSubtract (check->s.pos.trBase, pusher->r.currentOrigin, org);
	org2[0] = DotProduct (org, forward);
	org2[1] = -DotProduct (org, right);
	org2[2] = DotProduct (org, up);
	VectorSubtract (org2, org, move2);
	VectorAdd (check->s.pos.trBase, move2, check->s.pos.trBase);

	ret = G_CheckProxMinePosition( check );
	if (ret) {
		VectorCopy( check->s.pos.trBase, check->r.currentOrigin );
		trap_LinkEntity (check);
	}
	return ret;
}

void G_ExplodeMissile( gentity_t *ent );

/*
============
G_MoverPush

Objects need to be moved back on a failed push,
otherwise riders would continue to slide.
If qfalse is returned, *obstacle will be the blocking entity
============
*/
qboolean G_MoverPush( gentity_t *pusher, vec3_t move, vec3_t amove, gentity_t **obstacle ) {
	int			i, e;
	gentity_t	*check;
	vec3_t		mins, maxs;
	pushed_t	*p;
	int			entityList[MAX_GENTITIES];
	int			listedEntities;
	vec3_t		totalMins, totalMaxs;

	*obstacle = NULL;


	// mins/maxs are the bounds at the destination
	// totalMins / totalMaxs are the bounds for the entire move
	if ( pusher->r.currentAngles[0] || pusher->r.currentAngles[1] || pusher->r.currentAngles[2]
		|| amove[0] || amove[1] || amove[2] ) {
		float		radius;

		radius = RadiusFromBounds( pusher->s.mins, pusher->s.maxs );
		for ( i = 0 ; i < 3 ; i++ ) {
			mins[i] = pusher->r.currentOrigin[i] + move[i] - radius;
			maxs[i] = pusher->r.currentOrigin[i] + move[i] + radius;
			totalMins[i] = mins[i] - move[i];
			totalMaxs[i] = maxs[i] - move[i];
		}
	} else {
		for (i=0 ; i<3 ; i++) {
			mins[i] = pusher->r.absmin[i] + move[i];
			maxs[i] = pusher->r.absmax[i] + move[i];
		}

		VectorCopy( pusher->r.absmin, totalMins );
		VectorCopy( pusher->r.absmax, totalMaxs );
		for (i=0 ; i<3 ; i++) {
			if ( move[i] > 0 ) {
				totalMaxs[i] += move[i];
			} else {
				totalMins[i] += move[i];
			}
		}
	}

	// unlink the pusher so we don't get it in the entityList
	trap_UnlinkEntity( pusher );

	listedEntities = trap_EntitiesInBox( totalMins, totalMaxs, entityList, MAX_GENTITIES );

	// move the pusher to its final position
	VectorAdd( pusher->r.currentOrigin, move, pusher->r.currentOrigin );
	VectorAdd( pusher->r.currentAngles, amove, pusher->r.currentAngles );
	trap_LinkEntity( pusher );

	// see if any solid entities are inside the final position
	for ( e = 0 ; e < listedEntities ; e++ ) {
		check = &g_entities[ entityList[ e ] ];

#if defined MISSIONPACK || defined TA_WEAPSYS
		if ( check->s.eType == ET_MISSILE
#ifdef TA_WEAPSYS // GRAPPLE_MOVE
			|| check->s.eType == ET_GRAPPLE
#endif
			)
		{
#ifdef TA_WEAPSYS
			if (bg_projectileinfo[check->s.weapon].stickOnImpact
				// GRAPPLE_MOVE
				|| check->s.eType == ET_GRAPPLE)
#else
			// if it is a prox mine
			if ( !strcmp(check->classname, "prox mine") )
#endif
			{
				// if this prox mine is attached to this mover try to move it with the pusher
				if ( check->enemy == pusher ) {
					if (!G_TryPushingProxMine( check, pusher, move, amove )) {
						//explode
						check->s.loopSound = 0;
#ifdef TA_WEAPSYS
						if (bg_projectileinfo[check->s.weapon].explosionType == PE_PROX) {
							G_AddEvent( check, EV_PROJECTILE_TRIGGER, 0 );
							if (check->activator) {
								G_FreeEntity(check->activator);
								check->activator = NULL;
							}
						}

						// GRAPPLE_MOVE
						if (bg_projectileinfo[check->s.weapon].grappling
							&& check->parent && check->parent->client && check->parent->client->hook == check)
						{
							Weapon_HookFree(check);
						}
						else
						{
							G_ExplodeMissile(check);
						}
#else
						G_AddEvent( check, EV_PROXIMITY_MINE_TRIGGER, 0 );
						G_ExplodeMissile(check);
						if (check->activator) {
							G_FreeEntity(check->activator);
							check->activator = NULL;
						}
						//G_Printf("prox mine explodes\n");
#endif
					}
				}
				else {
					//check if the prox mine is crushed by the mover
					if (!G_CheckProxMinePosition( check )) {
						//explode
						check->s.loopSound = 0;
#ifdef TA_WEAPSYS
						if (bg_projectileinfo[check->s.weapon].explosionType == PE_PROX) {
							G_AddEvent( check, EV_PROJECTILE_TRIGGER, 0 );
							if (check->activator) {
								G_FreeEntity(check->activator);
								check->activator = NULL;
							}
						}

						// GRAPPLE_MOVE
						if (bg_projectileinfo[check->s.weapon].grappling
							&& check->parent && check->parent->client && check->parent->client->hook == check)
						{
							Weapon_HookFree(check);
						}
						else
						{
							G_ExplodeMissile(check);
						}
#else
						G_AddEvent( check, EV_PROXIMITY_MINE_TRIGGER, 0 );
						G_ExplodeMissile(check);
						if (check->activator) {
							G_FreeEntity(check->activator);
							check->activator = NULL;
						}
						//G_Printf("prox mine explodes\n");
#endif
					}
				}
				continue;
			}
		}
#endif
		// only push items and players
		if ( check->s.eType != ET_ITEM && check->s.eType != ET_PLAYER &&
#ifdef TA_ENTSYS // PUSHABLE
			!(check->flags & FL_PUSHABLE) &&
#endif
			!check->physicsObject ) {
			continue;
		}

		// if the entity is standing on the pusher, it will definitely be moved
		if ( check->s.groundEntityNum != pusher->s.number ) {
			// see if the ent needs to be tested
			if ( check->r.absmin[0] >= maxs[0]
			|| check->r.absmin[1] >= maxs[1]
			|| check->r.absmin[2] >= maxs[2]
			|| check->r.absmax[0] <= mins[0]
			|| check->r.absmax[1] <= mins[1]
			|| check->r.absmax[2] <= mins[2] ) {
				continue;
			}
			// see if the ent's bbox is inside the pusher's final position
			// this does allow a fast moving object to pass through a thin entity...
			if (!G_TestEntityPosition (check)) {
				continue;
			}
		}

		// the entity needs to be pushed
		if ( G_TryPushingEntity( check, pusher, move, amove ) ) {
			continue;
		}

		// the move was blocked an entity

		// bobbing entities are instant-kill and never get blocked
		if ( pusher->s.pos.trType == TR_SINE || pusher->s.apos.trType == TR_SINE ) {
			G_Damage( check, pusher, pusher, NULL, NULL, 99999, 0, MOD_CRUSH );
			continue;
		}

		
		// save off the obstacle so we can call the block function (crush, etc)
		*obstacle = check;

		// move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		for ( p=pushed_p-1 ; p>=pushed ; p-- ) {
			VectorCopy (p->origin, p->ent->s.pos.trBase);
			VectorCopy (p->angles, p->ent->s.apos.trBase);
			if ( p->ent->client ) {
				p->ent->client->ps.delta_angles[YAW] = p->deltayaw;
				VectorCopy (p->origin, p->ent->client->ps.origin);
			}
			trap_LinkEntity (p->ent);
		}
		return qfalse;
	}

	return qtrue;
}


/*
=================
G_MoverTeam
=================
*/
void G_MoverTeam( gentity_t *ent ) {
	vec3_t		move, amove;
	gentity_t	*part, *obstacle;
	vec3_t		origin, angles;

	obstacle = NULL;

	// make sure all team slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	pushed_p = pushed;
	for (part = ent ; part ; part=part->teamchain) {
		// get current position
		BG_EvaluateTrajectory( &part->s.pos, level.time, origin );
		BG_EvaluateTrajectory( &part->s.apos, level.time, angles );
		VectorSubtract( origin, part->r.currentOrigin, move );
		VectorSubtract( angles, part->r.currentAngles, amove );
		if ( !G_MoverPush( part, move, amove, &obstacle ) ) {
			break;	// move was blocked
		}
	}

	if (part) {
		// go back to the previous position
		for ( part = ent ; part ; part = part->teamchain ) {
			part->s.pos.trTime += level.time - level.previousTime;
			part->s.apos.trTime += level.time - level.previousTime;
			BG_EvaluateTrajectory( &part->s.pos, level.time, part->r.currentOrigin );
			BG_EvaluateTrajectory( &part->s.apos, level.time, part->r.currentAngles );
			trap_LinkEntity( part );
		}

		// if the pusher has a "blocked" function, call it
		if (ent->blocked) {
			ent->blocked( ent, obstacle );
		}
		return;
	}

	// the move succeeded
	for ( part = ent ; part ; part = part->teamchain ) {
		// call the reached function if time is at or past end point
		if ( part->s.pos.trType == TR_LINEAR_STOP ) {
			if ( level.time >= part->s.pos.trTime + part->s.pos.trDuration ) {
				if ( part->reached ) {
					part->reached( part );
				}
			}
		}
	}
}

/*
================
G_RunMover

================
*/
void G_RunMover( gentity_t *ent ) {
	// if not a team captain, don't do anything, because
	// the captain will handle everything
	if ( ent->flags & FL_TEAMSLAVE ) {
		return;
	}

	// if stationary at one of the positions, don't move anything
	if ( ent->s.pos.trType != TR_STATIONARY || ent->s.apos.trType != TR_STATIONARY ) {
		G_MoverTeam( ent );
	}

	// check think function
	G_RunThink( ent );
}

/*
============================================================================

GENERAL MOVERS

Doors, plats, and buttons are all binary (two position) movers
Pos1 is "at rest", pos2 is "activated"
============================================================================
*/

/*
===============
SetMoverState
===============
*/
void SetMoverState( gentity_t *ent, moverState_t moverState, int time ) {
	vec3_t			delta;
	float			f;

	ent->moverState = moverState;

	ent->s.pos.trTime = time;
	switch( moverState ) {
	case MOVER_POS1:
		VectorCopy( ent->pos1, ent->s.pos.trBase );
		ent->s.pos.trType = TR_STATIONARY;
		break;
	case MOVER_POS2:
		VectorCopy( ent->pos2, ent->s.pos.trBase );
		ent->s.pos.trType = TR_STATIONARY;
		break;
	case MOVER_1TO2:
		VectorCopy( ent->pos1, ent->s.pos.trBase );
		VectorSubtract( ent->pos2, ent->pos1, delta );
		f = 1000.0 / ent->s.pos.trDuration;
		VectorScale( delta, f, ent->s.pos.trDelta );
		ent->s.pos.trType = TR_LINEAR_STOP;
		break;
	case MOVER_2TO1:
		VectorCopy( ent->pos2, ent->s.pos.trBase );
		VectorSubtract( ent->pos1, ent->pos2, delta );
		f = 1000.0 / ent->s.pos.trDuration;
		VectorScale( delta, f, ent->s.pos.trDelta );
		ent->s.pos.trType = TR_LINEAR_STOP;
		break;
	}
	BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->r.currentOrigin );	
	trap_LinkEntity( ent );
}

/*
================
MatchTeam

All entities in a mover team will move from pos1 to pos2
in the same amount of time
================
*/
void MatchTeam( gentity_t *teamLeader, int moverState, int time ) {
	gentity_t		*slave;

	for ( slave = teamLeader ; slave ; slave = slave->teamchain ) {
		SetMoverState( slave, moverState, time );
	}
}



/*
================
ReturnToPos1
================
*/
void ReturnToPos1( gentity_t *ent ) {
	MatchTeam( ent, MOVER_2TO1, level.time );

	// looping sound
	ent->s.loopSound = ent->soundLoop;

	// starting sound
	if ( ent->sound2to1 ) {
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );
	}
}


/*
================
Reached_BinaryMover
================
*/
void Reached_BinaryMover( gentity_t *ent ) {

	// stop the looping sound
	ent->s.loopSound = ent->soundLoop;

	if ( ent->moverState == MOVER_1TO2 ) {
		// reached pos2
		SetMoverState( ent, MOVER_POS2, level.time );

		// play sound
		if ( ent->soundPos2 ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos2 );
		}

		// return to pos1 after a delay
		ent->think = ReturnToPos1;
		ent->nextthink = level.time + ent->wait;

		// fire targets
		if ( !ent->activator ) {
			ent->activator = ent;
		}
		G_UseTargets( ent, ent->activator );
	} else if ( ent->moverState == MOVER_2TO1 ) {
		// reached pos1
		SetMoverState( ent, MOVER_POS1, level.time );

		// play sound
		if ( ent->soundPos1 ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos1 );
		}

		// close areaportals
		if ( ent->teammaster == ent || !ent->teammaster ) {
			trap_AdjustAreaPortalState( ent, qfalse );
		}
	} else {
		G_Error( "Reached_BinaryMover: bad moverState" );
	}
}


/*
================
Use_BinaryMover
================
*/
void Use_BinaryMover( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	int		total;
	int		partial;

	// only the master should be used
	if ( ent->flags & FL_TEAMSLAVE ) {
		Use_BinaryMover( ent->teammaster, other, activator );
		return;
	}

	ent->activator = activator;

	if ( ent->moverState == MOVER_POS1 ) {
		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam( ent, MOVER_1TO2, level.time + 50 );

		// starting sound
		if ( ent->sound1to2 ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
		}

		// looping sound
		ent->s.loopSound = ent->soundLoop;

		// open areaportal
		if ( ent->teammaster == ent || !ent->teammaster ) {
			trap_AdjustAreaPortalState( ent, qtrue );
		}
		return;
	}

	// if all the way up, just delay before coming down
	if ( ent->moverState == MOVER_POS2 ) {
		ent->nextthink = level.time + ent->wait;
		return;
	}

	// only partway down before reversing
	if ( ent->moverState == MOVER_2TO1 ) {
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.pos.trTime;
		if ( partial > total ) {
			partial = total;
		}

		MatchTeam( ent, MOVER_1TO2, level.time - ( total - partial ) );

		if ( ent->sound1to2 ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
		}
		return;
	}

	// only partway up before reversing
	if ( ent->moverState == MOVER_1TO2 ) {
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.pos.trTime;
		if ( partial > total ) {
			partial = total;
		}

		MatchTeam( ent, MOVER_2TO1, level.time - ( total - partial ) );

		if ( ent->sound2to1 ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );
		}
		return;
	}
}


#ifdef TA_ENTSYS // BREAKABLE
/*
================
G_SeenByHumans

Based on Smokin' Guns G_BreakableRespawn
================
*/
qboolean G_SeenByHumans( gentity_t *ent )
{
	gentity_t	*player;
	gclient_t	*client;
	int			i;
	const float fov = 100;
	float		diff;
	float		angle;
	vec3_t		angles;
	vec3_t		dir;
	vec3_t		eye;
	qboolean	cont;
	int			j;
	vec3_t		origin;

	if (ent->s.eType == ET_MISCOBJECT)
	{
		VectorCopy(ent->r.currentOrigin, origin);
	}
	else
	{
		vec3_t pos1, pos2;

        // Tequila comment: set breakable center as origin for G_BreakableRespawn needs
        VectorSubtract(ent->r.absmax, ent->r.absmin, pos1);
        VectorScale(pos1, 0.5f, pos2);
        VectorAdd(pos2, ent->r.absmin, origin);
	}

	// cycle through all players and see if the breakable respawn would be visible to them
	for (i = 0; i < level.maxclients; i++) {
		player = &g_entities[i];
		client = &level.clients[i];

		if (client->pers.connected != CON_CONNECTED)
			continue;

		// if it's too near abort
		cont = (Distance(client->ps.origin, origin) >= 300);

		if (cont)
		{
			// first check if player could be stuck in the breakable
			if ( player->r.absmin[0] > ent->r.absmax[0]
				|| player->r.absmin[1] > ent->r.absmax[1]
				|| player->r.absmin[2] > ent->r.absmax[2]
				|| player->r.absmax[0] < ent->r.absmin[0]
				|| player->r.absmax[1] < ent->r.absmin[1]
				|| player->r.absmax[2] < ent->r.absmin[2] ) {
				cont = qtrue;
			} else {
				cont = qfalse;
			}
		}

		// Process field of vision tests
		if (cont) // && !g_forcebreakrespawn.integer
		{
			// Tequila comment: Minor server optimization, don't check if breakable is in a bot FOV
			// They really don't care to "see" a breakable respawn, so we won't delay the respawn
			// because of bot proximity.
			if (player->r.svFlags & SVF_BOT)
				continue;

			// check if its in field of vision
			VectorCopy(client->ps.origin, eye);
			eye[2] += client->ps.viewheight;

			VectorSubtract(origin, eye, dir);
			vectoangles(dir, angles);

			cont = qfalse;

			for (j = 0; j < 2; j++) {
				angle = AngleMod(client->ps.viewangles[j]);
				angles[j] = AngleMod(angles[j]);
				diff = fabs(angles[j] - angle);

				if (diff > 180.0)
					diff -= 360.0;

				// if not in field of vision continue;
				if ( fabs(diff) > fov/2 ) {
					cont = qtrue;
					break;
				}
			}
		}

		if (!cont) {
			// it might be seen
			return qtrue;
		}
	}

	// now while nobody can see it, respawn the breakable
	return qfalse;
}

/*
================
G_BreakableRespawn
================
*/
void G_BreakableRespawn( gentity_t *self )
{
	// Don't let the humans see it respawn
	if (G_SeenByHumans(self))
	{
		// Defer for a max of the total respawn time
		if (self->random < self->wait)
		{
			self->random++;

			// Try again later
			self->nextthink = level.time + 1000;
			self->think = G_BreakableRespawn;
			return;
		}
	}
	self->random = 0; // clear defer count

	// Kill players so they don't get stuck
	G_KillBox(self);

	// Remove dropped item
	if (self->chain) {
		G_FreeEntity(self->chain);
	}

	self->health = self->splashRadius;

	VectorCopy(self->pos1, self->s.origin); // SMOKIN_GUNS

	trap_LinkEntity(self);
}

/*
================
G_BreakableDie
================
*/
void G_BreakableDie( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath )
{
	gentity_t *target;

	// If a stickOnImpact projectile is stuck into this breakable have it fall!
	for (target = g_entities; target < &g_entities[level.num_entities]; target++)
	{
		if (target->s.eType == ET_MISSILE && (target->count & 2) && target->enemy == self)
		{
			target->count &= ~2; // Remove impact flag
			target->s.pos.trType = TR_GRAVITY;
			target->s.pos.trTime = level.time;
		}
	}

	// ZTM: NOTE: Death sound is sent to cgame by EV_SPAWN_DEBRIS in G_BreakableDebris

	G_UseTargets(self, attacker);

	// Spawn item
	self->chain = NULL;
	if (self->message)
	{
		vec3_t origin, pos1, pos2;

		// Tequila comment: set breakable center as origin for G_BreakableRespawn needs
		VectorSubtract(self->r.absmax, self->r.absmin, pos1);
		VectorScale(pos1, 0.5f, pos2);
		VectorAdd(pos2, self->r.absmin, origin);

#ifdef TA_WEAPSYS
		if (Q_stricmp(self->message, "weapon_random") == 0) {
			// weapon_random: Change item!
			if (!(self->s.eFlags & EF_VOTED) && !self->item) {
				self->item = G_RandomWeaponItem(self, self->spawnflags>>7);
			}
		} else
#endif
		{
			self->item = BG_FindItemForClassname(self->message);
		}

		if (self->item) {
			self->chain = LaunchItem(self->item, origin, vec3_origin);
		}
	}

	// if respawn
	if (self->wait > 0) {
		// Respawn after X seconds
		self->nextthink = level.time + (self->wait * 1000);
		self->think = G_BreakableRespawn;
		trap_UnlinkEntity(self);
	} else {
		// Good bye!
		G_FreeEntity(self);
	}
}
#endif

#if 0 //#ifdef TA_ENTSYS // PUSHABLE
/*
============
G_PlayerPush

Objects need to be moved back on a failed push,
otherwise riders would continue to slide.
If qfalse is returned, *obstacle will be the blocking entity
============
*/
qboolean G_PlayerPush( gentity_t *pusher, vec3_t move, vec3_t amove, gentity_t **obstacle ) {
	int			i, e;
	gentity_t	*check;
	vec3_t		mins, maxs;
	pushed_t	*p;
	int			entityList[MAX_GENTITIES];
	int			listedEntities;
	vec3_t		totalMins, totalMaxs;

	*obstacle = NULL;

	pushed_p = pushed;

	// mins/maxs are the bounds at the destination
	// totalMins / totalMaxs are the bounds for the entire move
	if ( pusher->r.currentAngles[0] || pusher->r.currentAngles[1] || pusher->r.currentAngles[2]
		|| amove[0] || amove[1] || amove[2] ) {
		float		radius;

		radius = RadiusFromBounds( pusher->s.mins, pusher->s.maxs );
		for ( i = 0 ; i < 3 ; i++ ) {
			mins[i] = pusher->r.currentOrigin[i] + move[i] - radius;
			maxs[i] = pusher->r.currentOrigin[i] + move[i] + radius;
			totalMins[i] = mins[i] - move[i];
			totalMaxs[i] = maxs[i] - move[i];
		}
	} else {
		for (i=0 ; i<3 ; i++) {
			mins[i] = pusher->r.absmin[i] + move[i];
			maxs[i] = pusher->r.absmax[i] + move[i];
		}

		VectorCopy( pusher->r.absmin, totalMins );
		VectorCopy( pusher->r.absmax, totalMaxs );
		for (i=0 ; i<3 ; i++) {
			if ( move[i] > 0 ) {
				totalMaxs[i] += move[i];
			} else {
				totalMins[i] += move[i];
			}
		}
	}

	// unlink the pusher so we don't get it in the entityList
	trap_UnlinkEntity( pusher );

	listedEntities = trap_EntitiesInBox( totalMins, totalMaxs, entityList, MAX_GENTITIES );

	// move the pusher to its final position
	VectorAdd( pusher->r.currentOrigin, move, pusher->r.currentOrigin );
	VectorAdd( pusher->r.currentAngles, amove, pusher->r.currentAngles );
	trap_LinkEntity( pusher );

	// see if any solid entities are inside the final position
	for ( e = 0 ; e < listedEntities ; e++ ) {
		check = &g_entities[ entityList[ e ] ];

		// only push the pushable objects
		if (!(check->flags & FL_PUSHABLE) && !check->physicsObject ) {
			continue;
		}

		// if object is heavy must have strength to push it
		if ((check->flags & FL_HEAVY) && (!pusher->client || pusher->client->pers.playercfg.ability != ABILITY_STRENGTH)) {
			continue;
		}

		// if the entity is standing on the pusher, it will definitely be moved
		if ( check->s.groundEntityNum != pusher->s.number ) {
			// see if the ent needs to be tested
			if ( check->r.absmin[0] >= maxs[0]
			|| check->r.absmin[1] >= maxs[1]
			|| check->r.absmin[2] >= maxs[2]
			|| check->r.absmax[0] <= mins[0]
			|| check->r.absmax[1] <= mins[1]
			|| check->r.absmax[2] <= mins[2] ) {
				continue;
			}
			// see if the ent's bbox is inside the pusher's final position
			// this does allow a fast moving object to pass through a thin entity...
			if (!G_TestEntityPosition (check)) {
				continue;
			}
		}

		// the entity needs to be pushed
		if ( G_TryPushingEntity( check, pusher, move, amove ) ) {
			continue;
		}

		// the move was blocked an entity

		// move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		for ( p=pushed_p-1 ; p>=pushed ; p-- ) {
			VectorCopy (p->origin, p->ent->s.pos.trBase);
			VectorCopy (p->angles, p->ent->s.apos.trBase);
			if ( p->ent->client ) {
				p->ent->client->ps.delta_angles[YAW] = p->deltayaw;
				VectorCopy (p->origin, p->ent->client->ps.origin);
			}
			trap_LinkEntity (p->ent);
		}

		// save off the obstacle so we can call the block function (crush, etc)
		*obstacle = check;
		return qfalse;
	}

	return qtrue;
}
#endif


/*
================
InitMover

"pos1", "pos2", and "speed" should be set before calling,
so the movement delta can be calculated
================
*/
void InitMover( gentity_t *ent ) {
	vec3_t		move;
	float		distance;
	float		light;
	vec3_t		color;
	qboolean	lightSet, colorSet;
	char		*sound;
#ifdef TA_ENTSYS // PUSHABLE
	int			pushable;
	int			heavy;
#endif

	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if ( ent->model2 ) {
		ent->s.modelindex2 = G_ModelIndex( ent->model2 );
	}

	// if the "loopsound" key is set, use a constant looping sound when moving
	if ( G_SpawnString( "noise", "100", &sound ) ) {
		ent->s.loopSound = G_SoundIndex( sound );
	}

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = G_SpawnFloat( "light", "100", &light );
	colorSet = G_SpawnVector( "color", "1 1 1", color );
	if ( lightSet || colorSet ) {
		int		r, g, b, i;

		r = color[0] * 255;
		if ( r > 255 ) {
			r = 255;
		}
		g = color[1] * 255;
		if ( g > 255 ) {
			g = 255;
		}
		b = color[2] * 255;
		if ( b > 255 ) {
			b = 255;
		}
		i = light / 4;
		if ( i > 255 ) {
			i = 255;
		}
		ent->s.constantLight = r | ( g << 8 ) | ( b << 16 ) | ( i << 24 );
	}


	ent->use = Use_BinaryMover;
	ent->reached = Reached_BinaryMover;

	ent->moverState = MOVER_POS1;
	ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->s.eType = ET_MOVER;
	VectorCopy (ent->pos1, ent->r.currentOrigin);
	trap_LinkEntity (ent);

	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy( ent->pos1, ent->s.pos.trBase );

	// calculate time to reach second position from speed
	VectorSubtract( ent->pos2, ent->pos1, move );
	distance = VectorLength( move );
	if ( ! ent->speed ) {
		ent->speed = 100;
	}
	VectorScale( move, ent->speed, ent->s.pos.trDelta );
	ent->s.pos.trDuration = distance * 1000 / ent->speed;
	if ( ent->s.pos.trDuration <= 0 ) {
		ent->s.pos.trDuration = 1;
	}

#ifdef TA_ENTSYS // BREAKABLE
	// Setup breakable ET_MOVER
	if (ent->health > 0)
	{
#ifdef TA_MISC // MATERIALS
		char *mat;
		int i;
#endif

		// No not constant random weapon...
		if (!(ent->spawnflags & 8<<7))
		{
			// Change weapons on respawn
			ent->s.eFlags |= EF_VOTED;
		}

		// Save health for respawning
		ent->splashRadius = ent->health;

		ent->takedamage = qtrue;
		ent->die = G_BreakableDie;

		// Per-entity explosion sound
		if ( G_SpawnString( "deathSound", "100", &sound ) ) {
			ent->noise_index = G_SoundIndex( sound );
		} else {
			// EV_SPAWN_DEBRIS will select sound based on surfaceFlags (ent->deathMaterial)
			ent->noise_index = MAX_SOUNDS-1;
		}

#ifdef TA_MISC // MATERIALS
		ent->deathMaterial = -1; // auto surfaceFlags
		if( G_SpawnString( "material", "none", &mat ) ) {
			ent->deathMaterial = 0;
			for ( i = 1; i < NUM_MATERIAL_TYPES; i++)
			{
				if ( strstr( mat, materialInfo[i].name ) != NULL ) {
					ent->deathMaterial |= materialInfo[i].surfaceFlag;
				}
			}
		}

		ent->damageMaterial = -1; // auto surfaceFlags
		if( G_SpawnString( "damage_material", "none", &mat ) ) {
			ent->damageMaterial = 0;
			for ( i = 1; i < NUM_MATERIAL_TYPES; i++)
			{
				if ( strstr( mat, materialInfo[i].name ) != NULL ) {
					ent->damageMaterial |= materialInfo[i].surfaceFlag;
				}
			}
		}

		if (ent->deathMaterial == -1 && ent->damageMaterial > 0) {
			ent->deathMaterial = ent->damageMaterial;
		}
#endif
	}
#endif
#ifdef TA_ENTSYS // PUSHABLE
	G_SpawnInt( "pushable", "0", &pushable );
	if (pushable) {
		ent->flags |= FL_PUSHABLE;
		if (ent->s.pos.trBase[0] == 0 && ent->s.pos.trBase[1] == 0 && ent->s.pos.trBase[2] == 0)
		{
			G_Printf("Warning: Pushable brush entity %d doesn't look like it has an origin set! Try adding an origin brush.\n", ent->s.number);
		}
	}
	G_SpawnInt( "heavy", "0", &heavy );
	if (heavy) {
		ent->flags |= FL_HEAVY;
	}
#endif
}


/*
===============================================================================

DOOR

A use can be triggered either by a touch function, by being shot, or by being
targeted by another entity.

===============================================================================
*/

/*
================
Blocked_Door
================
*/
void Blocked_Door( gentity_t *ent, gentity_t *other ) {
	// remove anything other than a client
	if ( !other->client ) {
		// except CTF flags!!!!
		if( other->s.eType == ET_ITEM && other->item->giType == IT_TEAM ) {
			Team_DroppedFlagThink( other );
			return;
		}
		G_TempEntity( other->s.origin, EV_ITEM_POP );
		G_FreeEntity( other );
		return;
	}

	if ( ent->damage ) {
		G_Damage( other, ent, ent, NULL, NULL, ent->damage, 0, MOD_CRUSH );
	}
	if ( ent->spawnflags & 4 ) {
		return;		// crushers don't reverse
	}

	// reverse direction
	Use_BinaryMover( ent, ent, other );
}

/*
================
Touch_DoorTriggerSpectator
================
*/
static void Touch_DoorTriggerSpectator( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	int axis;
	float doorMin, doorMax;
	vec3_t origin;

	axis = ent->count;
	// the constants below relate to constants in Think_SpawnNewDoorTrigger()
	doorMin = ent->r.absmin[axis] + 100;
	doorMax = ent->r.absmax[axis] - 100;

	VectorCopy(other->client->ps.origin, origin);

	if (origin[axis] < doorMin || origin[axis] > doorMax) return;

	if (fabs(origin[axis] - doorMax) < fabs(origin[axis] - doorMin)) {
		origin[axis] = doorMin - 10;
	} else {
		origin[axis] = doorMax + 10;
	}

	TeleportPlayer(other, origin, tv(10000000.0, 0, 0));
}

/*
================
Touch_DoorTrigger
================
*/
void Touch_DoorTrigger( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( other->client && other->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		// if the door is not open and not opening
		if ( ent->parent->moverState != MOVER_1TO2 &&
			ent->parent->moverState != MOVER_POS2) {
			Touch_DoorTriggerSpectator( ent, other, trace );
		}
	}
	else if ( ent->parent->moverState != MOVER_1TO2 ) {
		Use_BinaryMover( ent->parent, ent, other );
	}
}


/*
======================
Think_SpawnNewDoorTrigger

All of the parts of a door have been spawned, so create
a trigger that encloses all of them
======================
*/
void Think_SpawnNewDoorTrigger( gentity_t *ent ) {
	gentity_t		*other;
	vec3_t		mins, maxs;
	int			i, best;

	// set all of the slaves as shootable
	for ( other = ent ; other ; other = other->teamchain ) {
		other->takedamage = qtrue;
	}

	// find the bounds of everything on the team
	VectorCopy (ent->r.absmin, mins);
	VectorCopy (ent->r.absmax, maxs);

	for (other = ent->teamchain ; other ; other=other->teamchain) {
		AddPointToBounds (other->r.absmin, mins, maxs);
		AddPointToBounds (other->r.absmax, mins, maxs);
	}

	// find the thinnest axis, which will be the one we expand
	best = 0;
	for ( i = 1 ; i < 3 ; i++ ) {
		if ( maxs[i] - mins[i] < maxs[best] - mins[best] ) {
			best = i;
		}
	}
	maxs[best] += 120;
	mins[best] -= 120;

	// create a trigger with this size
	other = G_Spawn ();
	other->classname = "door_trigger";
	VectorCopy (mins, other->s.mins);
	VectorCopy (maxs, other->s.maxs);
	other->parent = ent;
	other->s.contents = CONTENTS_TRIGGER;
	other->touch = Touch_DoorTrigger;
	// remember the thinnest axis
	other->count = best;
	trap_LinkEntity (other);

	MatchTeam( ent, ent->moverState, level.time );
}

void Think_MatchTeam( gentity_t *ent ) {
	MatchTeam( ent, ent->moverState, level.time );
}


/*QUAKED func_door (0 .5 .8) ? START_OPEN x CRUSHER
TOGGLE		wait in both the start and end states for a trigger event.
START_OPEN	the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
NOMONSTER	monsters will not trigger this door

"model2"	.md3 model to also draw
"angle"		determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"speed"		movement speed (100 default)
"wait"		wait before returning (3 default, -1 = never return)
"lip"		lip remaining at end of move (8 default)
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
"health"	if set, the door must be shot open
#ifdef IOQ3ZTM
"noiseStart"door start move sound (default: sound/movers/doors/dr1_strt.wav)
"noiseEnd"	door end move sound (default: sound/movers/doors/dr1_end.wav)
#endif
*/
void SP_func_door (gentity_t *ent) {
	vec3_t	abs_movedir;
	float	distance;
	vec3_t	size;
	float	lip;
#ifdef IOQ3ZTM // Allow per-entity door sounds
	char *sound;

	G_SpawnString( "noiseStart", "sound/movers/doors/dr1_strt.wav", &sound );
	ent->sound1to2 = ent->sound2to1 = G_SoundIndex(sound);

	G_SpawnString( "noiseEnd", "sound/movers/doors/dr1_end.wav", &sound );
	ent->soundPos1 = ent->soundPos2 = G_SoundIndex(sound);
#else
	ent->sound1to2 = ent->sound2to1 = G_SoundIndex("sound/movers/doors/dr1_strt.wav");
	ent->soundPos1 = ent->soundPos2 = G_SoundIndex("sound/movers/doors/dr1_end.wav");
#endif

	ent->blocked = Blocked_Door;

	// default speed of 400
	if (!ent->speed)
		ent->speed = 400;

	// default wait of 2 seconds
	if (!ent->wait)
		ent->wait = 2;
	ent->wait *= 1000;

	// default lip of 8 units
	G_SpawnFloat( "lip", "8", &lip );

	// default damage of 2 points
	G_SpawnInt( "dmg", "2", &ent->damage );

	// first position at start
	VectorCopy( ent->s.origin, ent->pos1 );

	// calculate second position
	trap_SetBrushModel( ent, ent->model );
	G_SetMovedir (ent->s.angles, ent->movedir);
	abs_movedir[0] = fabs(ent->movedir[0]);
	abs_movedir[1] = fabs(ent->movedir[1]);
	abs_movedir[2] = fabs(ent->movedir[2]);
	VectorSubtract( ent->s.maxs, ent->s.mins, size );
	distance = DotProduct( abs_movedir, size ) - lip;
	VectorMA( ent->pos1, distance, ent->movedir, ent->pos2 );

	// if "start_open", reverse position 1 and 2
	if ( ent->spawnflags & 1 ) {
		vec3_t	temp;

		VectorCopy( ent->pos2, temp );
		VectorCopy( ent->s.origin, ent->pos2 );
		VectorCopy( temp, ent->pos1 );
	}

	InitMover( ent );

	ent->nextthink = level.time + FRAMETIME;

	if ( ! (ent->flags & FL_TEAMSLAVE ) ) {
		int health;

		G_SpawnInt( "health", "0", &health );
		if ( health ) {
			ent->takedamage = qtrue;
		}
		if ( ent->targetname || health )
		{
			// non touch/shoot doors
			ent->think = Think_MatchTeam;
		} else {
			ent->think = Think_SpawnNewDoorTrigger;
		}
	}
#ifdef TA_ENTSYS // BREAKABLE // Doors are not killable...
	ent->health = -1;
#endif


}

/*
===============================================================================

PLAT

===============================================================================
*/

/*
==============
Touch_Plat

Don't allow decent if a living player is on it
===============
*/
void Touch_Plat( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client || other->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	// delay return-to-pos1 by one second
	if ( ent->moverState == MOVER_POS2 ) {
		ent->nextthink = level.time + 1000;
	}
}

/*
==============
Touch_PlatCenterTrigger

If the plat is at the bottom position, start it going up
===============
*/
void Touch_PlatCenterTrigger(gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client ) {
		return;
	}

	if ( ent->parent->moverState == MOVER_POS1 ) {
		Use_BinaryMover( ent->parent, ent, other );
	}
}


/*
================
SpawnPlatTrigger

Spawn a trigger in the middle of the plat's low position
Elevator cars require that the trigger extend through the entire low position,
not just sit on top of it.
================
*/
void SpawnPlatTrigger( gentity_t *ent ) {
	gentity_t	*trigger;
	vec3_t	tmin, tmax;

	// the middle trigger will be a thin trigger just
	// above the starting position
	trigger = G_Spawn();
	trigger->classname = "plat_trigger";
	trigger->touch = Touch_PlatCenterTrigger;
	trigger->s.contents = CONTENTS_TRIGGER;
	trigger->parent = ent;
	
	tmin[0] = ent->pos1[0] + ent->s.mins[0] + 33;
	tmin[1] = ent->pos1[1] + ent->s.mins[1] + 33;
	tmin[2] = ent->pos1[2] + ent->s.mins[2];

	tmax[0] = ent->pos1[0] + ent->s.maxs[0] - 33;
	tmax[1] = ent->pos1[1] + ent->s.maxs[1] - 33;
	tmax[2] = ent->pos1[2] + ent->s.maxs[2] + 8;

	if ( tmax[0] <= tmin[0] ) {
		tmin[0] = ent->pos1[0] + (ent->s.mins[0] + ent->s.maxs[0]) *0.5;
		tmax[0] = tmin[0] + 1;
	}
	if ( tmax[1] <= tmin[1] ) {
		tmin[1] = ent->pos1[1] + (ent->s.mins[1] + ent->s.maxs[1]) *0.5;
		tmax[1] = tmin[1] + 1;
	}
	
	VectorCopy (tmin, trigger->s.mins);
	VectorCopy (tmax, trigger->s.maxs);

	trap_LinkEntity (trigger);
}


/*QUAKED func_plat (0 .5 .8) ?
Plats are always drawn in the extended position so they will light correctly.

"lip"		default 8, protrusion above rest position
"height"	total height of movement, defaults to model height
"speed"		overrides default 200.
"dmg"		overrides default 2
"model2"	.md3 model to also draw
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_plat (gentity_t *ent) {
	float		lip, height;

#ifdef IOQ3ZTM // Allow per-entity plat sounds
	char *sound;

	G_SpawnString( "noiseStart", "sound/movers/plats/pt1_strt.wav", &sound );
	ent->sound1to2 = ent->sound2to1 = G_SoundIndex(sound);

	G_SpawnString( "noiseEnd", "sound/movers/plats/pt1_end.wav", &sound );
	ent->soundPos1 = ent->soundPos2 = G_SoundIndex(sound);
#else
	ent->sound1to2 = ent->sound2to1 = G_SoundIndex("sound/movers/plats/pt1_strt.wav");
	ent->soundPos1 = ent->soundPos2 = G_SoundIndex("sound/movers/plats/pt1_end.wav");
#endif

	VectorClear (ent->s.angles);

	G_SpawnFloat( "speed", "200", &ent->speed );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "wait", "1", &ent->wait );
	G_SpawnFloat( "lip", "8", &lip );

	ent->wait = 1000;

	// create second position
	trap_SetBrushModel( ent, ent->model );

	if ( !G_SpawnFloat( "height", "0", &height ) ) {
		height = (ent->s.maxs[2] - ent->s.mins[2]) - lip;
	}

	// pos1 is the rest (bottom) position, pos2 is the top
	VectorCopy( ent->s.origin, ent->pos2 );
	VectorCopy( ent->pos2, ent->pos1 );
	ent->pos1[2] -= height;

	InitMover( ent );

	// touch function keeps the plat from returning while
	// a live player is standing on it
	ent->touch = Touch_Plat;

	ent->blocked = Blocked_Door;

	ent->parent = ent;	// so it can be treated as a door

	// spawn the trigger if one hasn't been custom made
	if ( !ent->targetname ) {
		SpawnPlatTrigger(ent);
	}
}


/*
===============================================================================

BUTTON

===============================================================================
*/

/*
==============
Touch_Button

===============
*/
void Touch_Button(gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client ) {
		return;
	}

	if ( ent->moverState == MOVER_POS1 ) {
		Use_BinaryMover( ent, other, other );
	}
}


/*QUAKED func_button (0 .5 .8) ?
When a button is touched, it moves some distance in the direction of its angle, triggers all of its targets, waits some time, then returns to its original position where it can be triggered again.

"model2"	.md3 model to also draw
"angle"		determines the opening direction
"target"	all entities with a matching targetname will be used
"speed"		override the default 40 speed
"wait"		override the default 1 second wait (-1 = never return)
"lip"		override the default 4 pixel lip remaining at end of move
"health"	if set, the button must be killed instead of touched
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_button( gentity_t *ent ) {
	vec3_t		abs_movedir;
	float		distance;
	vec3_t		size;
	float		lip;

#ifdef IOQ3ZTM // Allow per-entity button sounds
	char *sound;

	G_SpawnString( "noiseStart", "sound/movers/switches/butn2.wav", &sound );
	ent->sound1to2 = G_SoundIndex(sound);

	// Optional, no default
	if ( G_SpawnString( "noiseEnd", "100", &sound ) ) {
		ent->soundPos2 = G_SoundIndex(sound);
	}

	// ZTM: TODO: Sounds for sound2to1 and soundPos1 ?
#else
	ent->sound1to2 = G_SoundIndex("sound/movers/switches/butn2.wav");
#endif
	
	if ( !ent->speed ) {
		ent->speed = 40;
	}

	if ( !ent->wait ) {
		ent->wait = 1;
	}
	ent->wait *= 1000;

	// first position
	VectorCopy( ent->s.origin, ent->pos1 );

	// calculate second position
	trap_SetBrushModel( ent, ent->model );

	G_SpawnFloat( "lip", "4", &lip );

	G_SetMovedir( ent->s.angles, ent->movedir );
	abs_movedir[0] = fabs(ent->movedir[0]);
	abs_movedir[1] = fabs(ent->movedir[1]);
	abs_movedir[2] = fabs(ent->movedir[2]);
	VectorSubtract( ent->s.maxs, ent->s.mins, size );
	distance = abs_movedir[0] * size[0] + abs_movedir[1] * size[1] + abs_movedir[2] * size[2] - lip;
	VectorMA (ent->pos1, distance, ent->movedir, ent->pos2);

	if (ent->health) {
		// shootable button
		ent->takedamage = qtrue;
#ifdef TA_ENTSYS // BREAKABLE // Buttons are not killable...
		ent->health = -1;
#endif
	} else {
		// touchable button
		ent->touch = Touch_Button;
	}

	InitMover( ent );
}



/*
===============================================================================

TRAIN

===============================================================================
*/


#define TRAIN_START_ON		1
#define TRAIN_TOGGLE		2
#define TRAIN_BLOCK_STOPS	4

/*
===============
Think_BeginMoving

The wait time at a corner has completed, so start moving again
===============
*/
void Think_BeginMoving( gentity_t *ent ) {
	ent->s.pos.trTime = level.time;
	ent->s.pos.trType = TR_LINEAR_STOP;
}

/*
===============
Reached_Train
===============
*/
void Reached_Train( gentity_t *ent ) {
#ifdef TA_PATHSYS
	G_ReachedPath(ent, qfalse);
#else
	gentity_t		*next;
	float			speed;
	vec3_t			move;
	float			length;

	// copy the apropriate values
	next = ent->nextTrain;
	if ( !next || !next->nextTrain ) {
		return;		// just stop
	}

	// fire all other targets
	G_UseTargets( next, NULL );

	// set the new trajectory
	ent->nextTrain = next->nextTrain;
	VectorCopy( next->s.origin, ent->pos1 );
	VectorCopy( next->nextTrain->s.origin, ent->pos2 );

	// if the path_corner has a speed, use that
	if ( next->speed ) {
		speed = next->speed;
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
	ent->s.loopSound = next->soundLoop;

	// start it going
	SetMoverState( ent, MOVER_1TO2, level.time );

	// if there is a "wait" value on the target, don't start moving yet
	if ( next->wait ) {
		ent->nextthink = level.time + next->wait * 1000;
		ent->think = Think_BeginMoving;
		ent->s.pos.trType = TR_STATIONARY;
	}
#endif
}


/*
===============
Think_SetupTrainTargets

Link all the corners together
===============
*/
void Think_SetupTrainTargets( gentity_t *ent ) {
#ifdef TA_PATHSYS
	if (G_SetupPath(ent, ent->target) != PATH_ERROR) {
		// start the train moving from the first corner
		G_ReachedPath(ent, qfalse);
	}
#else
	gentity_t		*path, *next, *start;

	ent->nextTrain = G_Find( NULL, FOFS(targetname), ent->target );
	if ( !ent->nextTrain ) {
		G_Printf( "func_train at %s with an unfound target\n",
			vtos(ent->r.absmin) );
		return;
	}

	start = NULL;
	for ( path = ent->nextTrain ; path != start ; path = next ) {
		if ( !start ) {
			start = path;
		}

		if ( !path->target ) {
			G_Printf( "Train corner at %s without a target\n",
				vtos(path->s.origin) );
			return;
		}

		// find a path_corner among the targets
		// there may also be other targets that get fired when the corner
		// is reached
		next = NULL;
		do {
			next = G_Find( next, FOFS(targetname), path->target );
			if ( !next ) {
				G_Printf( "Train corner at %s without a target path_corner\n",
					vtos(path->s.origin) );
				return;
			}
		} while ( strcmp( next->classname, "path_corner" ) );

		path->nextTrain = next;
	}

	// start the train moving from the first corner
	Reached_Train( ent );
#endif
}



/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8)
Train path corners.
Target: next path corner and other targets to fire
"speed" speed to move to the next corner
"wait" seconds to wait before behining move to next corner
*/
void SP_path_corner( gentity_t *self ) {
	if ( !self->targetname ) {
		G_Printf ("path_corner with no targetname at %s\n", vtos(self->s.origin));
		G_FreeEntity( self );
		return;
	}
	// path corners don't need to be linked in
}



/*QUAKED func_train (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS
A train is a mover that moves between path_corner target points.
Trains MUST HAVE AN ORIGIN BRUSH.
The train spawns at the first target it is pointing at.
"model2"	.md3 model to also draw
"speed"		default 100
"dmg"		default	2
"noise"		looping sound to play when the train is in motion
"target"	next path corner
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_train (gentity_t *self) {
	VectorClear (self->s.angles);

	if (self->spawnflags & TRAIN_BLOCK_STOPS) {
		self->damage = 0;
	} else {
		if (!self->damage) {
			self->damage = 2;
		}
	}

	if ( !self->speed ) {
		self->speed = 100;
	}

	if ( !self->target ) {
		G_Printf ("func_train without a target at %s\n", vtos(self->r.absmin));
		G_FreeEntity( self );
		return;
	}

	trap_SetBrushModel( self, self->model );
	InitMover( self );

	self->reached = Reached_Train;

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->nextthink = level.time + FRAMETIME;
	self->think = Think_SetupTrainTargets;
}

/*
===============================================================================

STATIC

===============================================================================
*/


/*QUAKED func_static (0 .5 .8) ?
A bmodel that just sits there, doing nothing.  Can be used for conditional walls and models.
"model2"	.md3 model to also draw
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_static( gentity_t *ent ) {
	trap_SetBrushModel( ent, ent->model );
#ifdef IOQ3ZTM // BREAKABLE
	VectorCopy( ent->s.origin, ent->pos1);
	VectorCopy( ent->s.origin, ent->pos2);
#endif
	
	InitMover( ent );
	
#ifdef IOQ3ZTM // BREAKABLE
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );
#endif
	
	trap_LinkEntity(ent);
}


#ifdef TA_ENTSYS // BREAKABLE
/*
===============================================================================

BREAKABLE

All movers (less func_door and func_button) can be killed,
	but func_breakables are the only ones that bots will attack.

/ *QUAKED all func_* (less func_door and func_button)
"health" Mover's health, if 0 can't be killed (default 0)
"paintarget" Trigers target ent on pain.
* /

===============================================================================
*/


/*QUAKED func_breakable (0 .5 .8) ?
Damageable entity that when killed can spawn glass and other materials.
It's like func_static, except that bots will attack it.

"model2"	.md3 model to also draw
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_breakable( gentity_t *ent ) {
	trap_SetBrushModel( ent, ent->model );
	VectorCopy( ent->s.origin, ent->pos1);
	VectorCopy( ent->s.origin, ent->pos2);

	InitMover( ent );

	trap_LinkEntity(ent);
}
#endif


/*
===============================================================================

ROTATING

===============================================================================
*/


/*QUAKED func_rotating (0 .5 .8) ? START_ON - X_AXIS Y_AXIS
You need to have an origin brush as part of this entity.  The center of that brush will be
the point around which it is rotated. It will rotate around the Z axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.

"model2"	.md3 model to also draw
"speed"		determines how fast it moves; default value is 100.
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_rotating (gentity_t *ent) {
	if ( !ent->speed ) {
		ent->speed = 100;
	}

	// set the axis of rotation
	ent->s.apos.trType = TR_LINEAR;
	if ( ent->spawnflags & 4 ) {
		ent->s.apos.trDelta[2] = ent->speed;
	} else if ( ent->spawnflags & 8 ) {
		ent->s.apos.trDelta[0] = ent->speed;
	} else {
		ent->s.apos.trDelta[1] = ent->speed;
	}

	if (!ent->damage) {
		ent->damage = 2;
	}

	trap_SetBrushModel( ent, ent->model );
	InitMover( ent );

	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );
	VectorCopy( ent->s.apos.trBase, ent->r.currentAngles );

	trap_LinkEntity( ent );
}


/*
===============================================================================

BOBBING

===============================================================================
*/


/*QUAKED func_bobbing (0 .5 .8) ? X_AXIS Y_AXIS
Normally bobs on the Z axis
"model2"	.md3 model to also draw
"height"	amplitude of bob (32 default)
"speed"		seconds to complete a bob cycle (4 default)
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_bobbing (gentity_t *ent) {
	float		height;
	float		phase;

	G_SpawnFloat( "speed", "4", &ent->speed );
	G_SpawnFloat( "height", "32", &height );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "phase", "0", &phase );

	trap_SetBrushModel( ent, ent->model );
	InitMover( ent );

	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	ent->s.pos.trDuration = ent->speed * 1000;
	ent->s.pos.trTime = ent->s.pos.trDuration * phase;
	ent->s.pos.trType = TR_SINE;

	// set the axis of bobbing
	if ( ent->spawnflags & 1 ) {
		ent->s.pos.trDelta[0] = height;
	} else if ( ent->spawnflags & 2 ) {
		ent->s.pos.trDelta[1] = height;
	} else {
		ent->s.pos.trDelta[2] = height;
	}
}

/*
===============================================================================

PENDULUM

===============================================================================
*/


/*QUAKED func_pendulum (0 .5 .8) ?
You need to have an origin brush as part of this entity.
Pendulums always swing north / south on unrotated models.  Add an angles field to the model to allow rotation in other directions.
Pendulum frequency is a physical constant based on the length of the beam and gravity.
"model2"	.md3 model to also draw
"speed"		the number of degrees each way the pendulum swings, (30 default)
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_pendulum(gentity_t *ent) {
	float		freq;
	float		length;
	float		phase;
	float		speed;

	G_SpawnFloat( "speed", "30", &speed );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "phase", "0", &phase );

	trap_SetBrushModel( ent, ent->model );

	// find pendulum length
	length = fabs( ent->s.mins[2] );
	if ( length < 8 ) {
		length = 8;
	}

	freq = 1 / ( M_PI * 2 ) * sqrt( g_gravity.value / ( 3 * length ) );

	ent->s.pos.trDuration = ( 1000 / freq );

	InitMover( ent );

	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	VectorCopy( ent->s.angles, ent->s.apos.trBase );

	ent->s.apos.trDuration = 1000 / freq;
	ent->s.apos.trTime = ent->s.apos.trDuration * phase;
	ent->s.apos.trType = TR_SINE;
	ent->s.apos.trDelta[2] = speed;
}

#ifdef TA_ENTSYS // FUNC_USE
/*
===============================================================================

USE

===============================================================================
*/

void multi_trigger( gentity_t *ent, gentity_t *activator );

void Use_FuncUse( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	multi_trigger( ent, activator );
}

/*QUAKED func_use (.5 .5 .5) ? RED_ONLY BLUE_ONLY TECH
"wait" : Seconds between triggerings, 0.5 default, -1 = one time only.
"random"	wait variance, default is 0
"model2"	.md3 model to also draw
"color"		constantLight color
"light"		constantLight radius
Variable sized repeatable trigger.  Must be targeted at one or more entities.
so, the basic time between firing is a random time between
(wait - random) and (wait + random)
*/
void SP_func_use( gentity_t *ent ) {
	G_SpawnFloat( "wait", "0.5", &ent->wait );
	G_SpawnFloat( "random", "0", &ent->random );

	if ( ent->random >= ent->wait && ent->wait >= 0 ) {
		ent->random = ent->wait - FRAMETIME;
		G_Printf( "func_use has random >= wait\n" );
	}

	trap_SetBrushModel( ent, ent->model );
	ent->s.contents = CONTENTS_SOLID;		// replaces the -1 from trap_SetBrushModel
	InitMover( ent );
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	ent->use = Use_FuncUse;
}
#endif

#ifdef TA_ENTSYS // FUNC_VOODOO
/*QUAKED func_voodoo (.5 .5 .5) ? STAY_SOLID
"target"  Entities to target.

If only one target and not STAY_SOLID, attemps to act as a bounding box for the target.
*/
#define VOODOO_STAY_SOLID 1

void FuncVooodooThink(gentity_t *self)
{
	self->nextthink = level.time + FRAMETIME;

	// Check if we did the initial count
	if (self->count > 0) {
		gentity_t *ent = self->target_ent;

		// If target was set, using single target mode
		if (ent) {
			// Check if we should change the solidness of self
			if ((self->spawnflags & VOODOO_STAY_SOLID)) {
				return;
			}

			// Check if object was removed
			if (ent-g_entities >= MAX_CLIENTS && !ent->inuse) {
				//G_Printf("DEBUG: func_voodoo: target removed.\n");
				self->target_ent = NULL;
				G_FreeEntity(self);
				return;
			}

			// If entity become solid
			if ((ent->s.contents & (CONTENTS_BODY|CONTENTS_SOLID)) && !(self->s.contents & CONTENTS_SOLID)) {
				//G_Printf("DEBUG: func_voodoo's target became solid!\n");
				self->s.contents |= CONTENTS_SOLID;
				G_KillBox(self);
				trap_LinkEntity(self);
			}
		} else {
			// multiple target mode currently does nothing.
		}
	} else {
		gentity_t *ent = NULL;

		while(1)
		{
			ent = G_Find (ent, FOFS(targetname), self->target);
			if (!ent)
				break;

			// Count targets
			self->count++;
		}

		if (self->count > 1) {
			// Multiple targets
			self->target_ent = NULL;
		} else if (self->count == 1) {
			// Single target
			self->target_ent = G_PickTarget(self->target);
		} else if (Q_stricmpn(self->target, "client", 6) == 0 && self->target[6] >= '0' && self->target[6] <= '9') {
			// Voodoo doll
			int client = atoi(&self->target[6]);

			if (client >= 0 && client < MAX_CLIENTS) {
				self->target_ent = &g_entities[client];
			}

			if (self->target_ent && self->target_ent->client) {
				self->client = self->target_ent->client;
			}
		} else {
			// No targets
			G_FreeEntity(self);
		}
	}
}

void VoodooReached(gentity_t *self)	// movers call this when hitting endpoint
{
	if (self->target_ent) {
		if (self->target_ent->reached) {
			self->target_ent->reached(self->target_ent);
		}
	} else {
		gentity_t *ent = NULL;

		while(1)
		{
			ent = G_Find (ent, FOFS(targetname), self->target);
			if (!ent)
				break;

			if (ent->reached) {
				ent->reached(ent);
			}
		}
	}
}

void VoodooBlocked(gentity_t *self, gentity_t *other)
{
	if (self->target_ent) {
		if (self->target_ent->blocked) {
			self->target_ent->blocked(self->target_ent, other);
		}
	} else {
		gentity_t *ent = NULL;

		while(1)
		{
			ent = G_Find (ent, FOFS(targetname), self->target);
			if (!ent)
				break;

			if (ent->blocked) {
				ent->blocked(ent, other);
			}
		}
	}
}

void VoodooTouch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	if (self->target_ent) {
		if (self->target_ent->touch) {
			self->target_ent->touch(self->target_ent, other, trace);
		}
	} else {
		gentity_t *ent = NULL;

		while(1)
		{
			ent = G_Find (ent, FOFS(targetname), self->target);
			if (!ent)
				break;

			if (ent->touch) {
				ent->touch(ent, other, trace);
			}
		}
	}
}

void VoodooUse(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if (self->target_ent) {
		if (self->target_ent->use) {
			self->target_ent->use(self->target_ent, other, activator);
		}
	} else {
		gentity_t *ent = NULL;

		while(1)
		{
			ent = G_Find (ent, FOFS(targetname), self->target);
			if (!ent)
				break;

			if (ent->use) {
				ent->use(ent, other, activator);
			}
		}
	}
}

void VoodooDamage(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	if (self->target_ent) {

		self->target_ent->health = self->target_ent->health - damage;
		if ( self->target_ent->client ) {
			self->target_ent->client->ps.stats[STAT_HEALTH] = self->target_ent->health;
		}

		if ( self->target_ent->health <= 0 ) {
			if ( self->target_ent->client )
				self->target_ent->flags |= FL_NO_KNOCKBACK;

			if (self->target_ent->health < -999)
				self->target_ent->health = -999;

			self->target_ent->enemy = attacker;
			if (self->target_ent->die) {
				self->target_ent->die(self->target_ent, inflictor, attacker, damage, mod);
			}

			//G_BreakableDie?

			// If entity become non-solid
			if (!(self->spawnflags & VOODOO_STAY_SOLID) && (self->s.contents & CONTENTS_SOLID)
				&& !(self->target_ent->s.contents & (CONTENTS_BODY|CONTENTS_SOLID))) {
				//G_Printf("DEBUG: func_voodoo's target went non-solid\n");
				self->s.contents &= ~CONTENTS_SOLID;
				trap_UnlinkEntity(self);
			}
		} else if ( self->target_ent->pain ) {
			self->target_ent->pain (self->target_ent, attacker, damage);
		}
	} else {
		gentity_t *ent = NULL;

		while(1)
		{
			ent = G_Find (ent, FOFS(targetname), self->target);
			if (!ent)
				break;

			ent->health = ent->health - damage;
			if ( ent->client ) {
				ent->client->ps.stats[STAT_HEALTH] = ent->health;
			}

			if ( ent->health <= 0 ) {
				if ( ent->client )
					ent->flags |= FL_NO_KNOCKBACK;

				if (ent->health < -999)
					ent->health = -999;

				ent->enemy = attacker;
				ent->die (ent, inflictor, attacker, damage, mod);
			} else if ( ent->pain ) {
				ent->pain (ent, attacker, damage);
			}
		}
	}
}

// ZTM: NOTE: I don't think this is used...
void VoodooPain(gentity_t *self, gentity_t *attacker, int damage)
{
	// Restore voodoo health
	self->health = 10000;

	VoodooDamage(self, NULL, attacker, damage, MOD_UNKNOWN);
}

void SP_func_voodoo( gentity_t *ent ) {

	if (!ent->target || !*ent->target) {
		G_Printf("func_voodoo: target not set!\n");
	}

	trap_SetBrushModel( ent, ent->model );
	ent->s.contents = CONTENTS_SOLID;		// replaces the -1 from trap_SetBrushModel
	ent->health = 10000; // Give health to force setup of breakable in InitMover
	InitMover( ent );
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

#ifdef TA_MISC // MATERIALS
	if (ent->damageMaterial == -1) {
		// Default to no damage debris
		ent->damageMaterial = 0;
	}
#endif

	ent->flags |= FL_NO_KNOCKBACK;

	// some movers spawn on the second frame, so delay thinking
	// until the third frame
	ent->nextthink = level.time + FRAMETIME * 2;
	ent->think = FuncVooodooThink;
	ent->count = 0;

	ent->reached = VoodooReached;
	ent->blocked = VoodooBlocked;
	ent->touch = VoodooTouch;
	ent->use = VoodooUse;
	ent->pain = VoodooPain;
	ent->die = VoodooDamage;
}
#endif
