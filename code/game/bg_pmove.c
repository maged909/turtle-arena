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
// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "../qcommon/q_shared.h"
#ifdef GAME // TA_NPCSYS
#include "g_local.h"
#else
//#include ../cgame/cg_local.h"
#include "bg_misc.h"
#endif
#include "bg_local.h"

pmove_t		*pm;
pml_t		pml;

// movement parameters
float	pm_stopspeed = 100.0f;
float	pm_duckScale = 0.25f;
float	pm_swimScale = 0.50f;
#ifdef IOQ3ZTM // LADDER
float	pm_ladderScale = 0.80f;
float	pm_ladderWaterScale = 0.50f;
#endif

float	pm_accelerate = 10.0f;
float	pm_airaccelerate = 1.0f;
float	pm_wateraccelerate = 4.0f;
float	pm_flyaccelerate = 8.0f;
#ifdef IOQ3ZTM // LADDER
float	pm_ladderAccelerate = 3000.0f;  // The acceleration to friction ratio is 1:1
#endif

float	pm_friction = 6.0f;
float	pm_waterfriction = 1.0f;
#ifdef IOQ3ZTM // WALK_UNDERWATER
float	pm_watergroundfriction = 7.0f;
#endif
float	pm_flightfriction = 3.0f;
float	pm_spectatorfriction = 5.0f;
#ifdef IOQ3ZTM // LADDER
float	pm_ladderfriction = 3000.0f;  // Friction is high enough so you don't slip down
#endif

int		c_pmove = 0;


/*
===============
PM_AddEvent

===============
*/
void PM_AddEvent( int newEvent ) {
#ifdef TA_NPCSYS // TDC_NPC
	if (pm->npc) {
		return;
	}
#endif
	BG_AddPredictableEventToPlayerstate( newEvent, 0, pm->ps );
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt( int entityNum ) {
	int		i;

	if ( entityNum == ENTITYNUM_WORLD ) {
		return;
	}
	if ( pm->numtouch == MAXTOUCH ) {
		return;
	}

	// see if it is already added
	for ( i = 0 ; i < pm->numtouch ; i++ ) {
		if ( pm->touchents[ i ] == entityNum ) {
			return;
		}
	}

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

/*
===================
PM_StartTorsoAnim
===================
*/
static void PM_StartTorsoAnim( int anim ) {
#ifdef IOQ3ZTM // Needed for TA_WEAPSYS
	if (anim < 0) {
		return;
	}
#endif
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}
	pm->ps->torsoAnim = ( ( pm->ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
		| anim;
}
static void PM_StartLegsAnim( int anim ) {
#ifdef IOQ3ZTM // Needed for TA_WEAPSYS
	if (anim < 0) {
		return;
	}
#endif
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}
	if ( pm->ps->legsTimer > 0 ) {
		return;		// a high priority animation is running
	}
	pm->ps->legsAnim = ( ( pm->ps->legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
		| anim;
}

static void PM_ContinueLegsAnim( int anim ) {
#ifdef IOQ3ZTM // Needed for TA_WEAPSYS
	if (anim < 0) {
		return;
	}
#endif
	if ( ( pm->ps->legsAnim & ~ANIM_TOGGLEBIT ) == anim ) {
		return;
	}
	if ( pm->ps->legsTimer > 0 ) {
		return;		// a high priority animation is running
	}
	PM_StartLegsAnim( anim );
}

static void PM_ContinueTorsoAnim( int anim ) {
#ifdef IOQ3ZTM // Needed for TA_WEAPSYS
	if (anim < 0) {
		return;
	}
#endif
	if ( ( pm->ps->torsoAnim & ~ANIM_TOGGLEBIT ) == anim ) {
		return;
	}
	if ( pm->ps->torsoTimer > 0 ) {
		return;		// a high priority animation is running
	}
	PM_StartTorsoAnim( anim );
}

static void PM_ForceLegsAnim( int anim ) {
	pm->ps->legsTimer = 0;
	PM_StartLegsAnim( anim );
}

#ifdef TA_PLAYERSYS
#if 0 // ZTM: To be used in the future
static void PM_StartAnim( int anim ) {
#ifdef IOQ3ZTM // Needed for TA_WEAPSYS
	if (anim < 0) {
		return;
	}
#endif
	if (!pm->playercfg || pm->playercfg->animations[anim].prefixType & AP_TORSO) {
		PM_StartTorsoAnim(anim);
	}
	if (!pm->playercfg || pm->playercfg->animations[anim].prefixType & AP_LEGS) {
		PM_StartLegsAnim(anim);
	}
}
#endif

#ifdef TURTLEARENA // LADDER
static void PM_ContinueAnim( int anim ) {
#ifdef IOQ3ZTM // Needed for TA_WEAPSYS
	if (anim < 0) {
		return;
	}
#endif
	if (!pm->playercfg || pm->playercfg->animations[anim].prefixType & AP_TORSO) {
		PM_ContinueTorsoAnim(anim);
	}
	if (!pm->playercfg || pm->playercfg->animations[anim].prefixType & AP_LEGS) {
		PM_ContinueLegsAnim(anim);
	}
}
#endif
#endif

/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce ) {
	float	backoff;
	float	change;
	int		i;
	
	backoff = DotProduct (in, normal);
	
	if ( backoff < 0 ) {
		backoff *= overbounce;
	} else {
		backoff /= overbounce;
	}

	for ( i=0 ; i<3 ; i++ ) {
		change = normal[i]*backoff;
		out[i] = in[i] - change;
	}
}


/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction( void ) {
	vec3_t	vec;
	float	*vel;
	float	speed, newspeed, control;
	float	drop;
	
	vel = pm->ps->velocity;
	
	VectorCopy( vel, vec );
	if ( pml.walking ) {
		vec[2] = 0;	// ignore slope movement
	}

	speed = VectorLength(vec);
	if (speed < 1) {
		vel[0] = 0;
		vel[1] = 0;		// allow sinking underwater
#ifdef IOQ3ZTM // WALK_UNDERWATER
		if (pm->waterlevel == 3 &&
				(pml.groundPlane || pm->ps->groundEntityNum != ENTITYNUM_NONE))
		{
			vel[2] = 0;
		}
#endif
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	// apply ground friction
	if ( pm->waterlevel <= 1 ) {
		if ( pml.walking && !(pml.groundTrace.surfaceFlags & SURF_SLICK) ) {
			// if getting knocked back, no friction
			if ( ! (pm->ps->pm_flags & PMF_TIME_KNOCKBACK) ) {
				control = speed < pm_stopspeed ? pm_stopspeed : speed;
				drop += control*pm_friction*pml.frametime;
			}
		}
	}

#ifdef TA_NPCSYS // TDC_NPC
	// JPL - bat flying has "ground" friction
	if (pm->npc && pm->ps->powerups[PW_FLIGHT])
	{
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*pm_friction*pml.frametime;
	}
#endif

	// apply water friction even if just wading
	if ( pm->waterlevel ) {
#ifdef IOQ3ZTM // WALK_UNDERWATER
		if (pm->waterlevel == 3 &&
				(pml.groundPlane || pm->ps->groundEntityNum != ENTITYNUM_NONE))
		{
#if 1
			if ( !(pml.groundTrace.surfaceFlags & SURF_SLICK) ) {
				// if getting knocked back, no friction
				if ( ! (pm->ps->pm_flags & PMF_TIME_KNOCKBACK) ) {
					control = speed < pm_stopspeed ? pm_stopspeed : speed;
					drop += control*pm_watergroundfriction*pml.frametime;
				}
			}
#else
			drop += speed*pm_watergroundfriction*pml.frametime;
#endif
		}
		else
#endif
		drop += speed*pm_waterfriction*pm->waterlevel*pml.frametime;
	}

#ifdef IOQ3ZTM // LADDER
	if ( pml.ladder ) {
		drop += speed*pm_ladderfriction*pml.frametime;  // Add ladder friction!
	}
	else
#endif
	// apply flying friction
	if ( pm->ps->powerups[PW_FLIGHT]) {
		drop += speed*pm_flightfriction*pml.frametime;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR) {
		drop += speed*pm_spectatorfriction*pml.frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0) {
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate( vec3_t wishdir, float wishspeed, float accel ) {
#ifndef TURTLEARENA // Use proper acceleration
	// q2 style
	int			i;
	float		addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct (pm->ps->velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return;
	}
	accelspeed = accel*pml.frametime*wishspeed;
	if (accelspeed > addspeed) {
		accelspeed = addspeed;
	}
	
	for (i=0 ; i<3 ; i++) {
		pm->ps->velocity[i] += accelspeed*wishdir[i];	
	}
#else
	// proper way (avoids strafe jump maxspeed bug), but feels bad
	vec3_t		wishVelocity;
	vec3_t		pushDir;
	float		pushLen;
	float		canPush;

	VectorScale( wishdir, wishspeed, wishVelocity );
	VectorSubtract( wishVelocity, pm->ps->velocity, pushDir );
	pushLen = VectorNormalize( pushDir );

	canPush = accel*pml.frametime*wishspeed;
	if (canPush > pushLen) {
		canPush = pushLen;
	}

	VectorMA( pm->ps->velocity, canPush, pushDir, pm->ps->velocity );
#endif
}



/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float PM_CmdScale( usercmd_t *cmd ) {
	int		max;
	float	total;
	float	scale;

	max = abs( cmd->forwardmove );
	if ( abs( cmd->rightmove ) > max ) {
		max = abs( cmd->rightmove );
	}
	if ( abs( cmd->upmove ) > max ) {
		max = abs( cmd->upmove );
	}
	if ( !max ) {
		return 0;
	}

	total = sqrt( cmd->forwardmove * cmd->forwardmove
		+ cmd->rightmove * cmd->rightmove + cmd->upmove * cmd->upmove );
	scale = (float)pm->ps->speed * max / ( 127.0 * total );

	return scale;
}


/*
================
PM_SetMovementDir

Determine the rotation of the legs relative
to the facing dir
================
*/
static void PM_SetMovementDir( void ) {
#ifdef TA_NPCSYS // TDC_NPC
	if (pm->npc)
	{
		pm->ps->movementDir = 0;
		return;
	}
#endif
	if ( pm->cmd.forwardmove || pm->cmd.rightmove ) {
		if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 0;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 2;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 3;
		} else if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 4;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 5;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 6;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 7;
		}
	} else {
		// if they aren't actively going directly sideways,
		// change the animation to the diagonal so they
		// don't stop too crooked
		if ( pm->ps->movementDir == 2 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->ps->movementDir == 6 ) {
			pm->ps->movementDir = 7;
		} 
	}
}

#ifdef TA_PATHSYS // 2DMODE
static void PM_PathMoveInital( void ) {
	//vec3_t angles;

	if (!pm->ps->pathMode) {
		return;
	}

	//VectorClear(angles);

	// if on axis, instead of a line {
	//   angle = angle between axis point and player point + 90;
	//   if going "backward"
	//      angles -= 180;
	// } else {
	//   angles = angle between point1 and point2
	// }

	// Controls
	if (pm->ps->pathMode == PATHMODE_TOP) {
		pm->cmd.upmove = 0;
	} else if (pm->ps->pathMode == PATHMODE_BACK) {
		pm->cmd.upmove = pm->cmd.forwardmove;
		pm->cmd.forwardmove = 0;
	} else { // PATHMODE_SIDE
		// If 2D side view mode
		pm->cmd.upmove = pm->cmd.forwardmove;
		pm->cmd.forwardmove = pm->cmd.rightmove;
		pm->cmd.rightmove = 0;
		//pm->cmd.angles[YAW] = angles[YAW];
		//self->client->ps.stats[STAT_DEAD_YAW] = vectoyaw ( dir );
	}
}

// Sets wishdir, based on PM_GrappleMove
void PM_SetupPathWishVel(vec3_t wishvel, const vec3_t wishdir) {
	vec3_t v, vel;
	int move;

	if (!pm->ps->pathMode) {
		return;
	}

	VectorScale(pml.forward, -16, v);
	if (pm->ps->eFlags & EF_TRAINBACKWARD) {
		move = -pm->cmd.forwardmove;
		VectorAdd(pm->ps->grapplePoint, v, v);
	} else {
		move = pm->cmd.forwardmove;
		VectorAdd(pm->ps->nextPoint, v, v);
	}
	v[2] = pm->ps->origin[2];
	VectorSubtract(v, pm->ps->origin, vel);
	VectorNormalize( vel );

	VectorScale(vel, move, vel);
	vel[2] = wishvel[2];

	// Okay now set output
	VectorCopy(vel, wishvel);
}
#endif


/*
=============
PM_CheckJump
=============
*/
static qboolean PM_CheckJump( void ) {
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return qfalse;		// don't allow jump until all buttons are up
	}

	if ( pm->cmd.upmove < 10 ) {
		// not holding jump
		return qfalse;
	}

	// must wait for jump to be released
	if ( pm->ps->pm_flags & PMF_JUMP_HELD ) {
		// clear upmove so cmdscale doesn't lower running speed
		pm->cmd.upmove = 0;
		return qfalse;
	}

	pml.groundPlane = qfalse;		// jumping away
	pml.walking = qfalse;
#ifdef IOQ3ZTM // WALK_UNDERWATER
	if (pm->waterlevel < 3)
#endif
	pm->ps->pm_flags |= PMF_JUMP_HELD;

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
#ifdef TURTLEARENA // LOCKON
	if (pm->ps->eFlags & EF_LOCKON)
	{
		float force;

		// Don't allow players to use multiple jumps to gain velocity
		VectorClear(pm->ps->velocity);

		// If going forward/backward and right/left, only use half jump force for each.
		if (pm->cmd.forwardmove != 0 && pm->cmd.rightmove != 0) {
			force = (JUMP_VELOCITY * pm->playercfg->jumpMult) * 0.6f;
		} else {
			force = (JUMP_VELOCITY * pm->playercfg->jumpMult) * 1.2f;
		}

		if (pm->cmd.forwardmove > 0) {
			// Jump forward
			VectorMA(pm->ps->velocity, force, pml.forward, pm->ps->velocity);
		} else if (pm->cmd.forwardmove < 0) {
			// Jump backward
			VectorMA(pm->ps->velocity, -force, pml.forward, pm->ps->velocity);
		}

		if (pm->cmd.rightmove < 0) {
			// Jump left
			VectorMA(pm->ps->velocity, -force, pml.right, pm->ps->velocity);
		} else if (pm->cmd.rightmove > 0) {
			// Jump right
			VectorMA(pm->ps->velocity, force, pml.right, pm->ps->velocity);
		}
	}
#endif
#ifdef TA_NPCSYS // TDC_NPC
	if (pm->npc) {
		pm->ps->velocity[2] = pm->cmd.upmove*8;
	} else
#endif
#ifdef TA_PLAYERSYS
	pm->ps->velocity[2] = JUMP_VELOCITY * pm->playercfg->jumpMult;
#else
	pm->ps->velocity[2] = JUMP_VELOCITY;
#endif
	PM_AddEvent( EV_JUMP );

	if ( pm->cmd.forwardmove >= 0
#ifdef TA_PATHSYS // 2DMODE
			|| pm->ps->pathMode == PATHMODE_SIDE
#endif
	 ) {
#ifdef TA_NPCSYS
		if (pm->npc) {
			PM_ForceLegsAnim( OBJECT_JUMP );
		} else
#endif
		PM_ForceLegsAnim( LEGS_JUMP );
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	} else {
#ifdef TA_NPCSYS
		if (pm->npc) {
			PM_ForceLegsAnim( OBJECT_JUMP );
		} else
#endif
#ifdef TURTLEARENA // PLAYERS
		if (pm->ps && pm->ps->eFlags & EF_LOCKON) {
			PM_ForceLegsAnim( LEGS_JUMPB_LOCKON );
		} else
#endif
		PM_ForceLegsAnim( LEGS_JUMPB );
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}

	return qtrue;
}

/*
=============
PM_CheckWaterJump
=============
*/
static qboolean	PM_CheckWaterJump( void ) {
	vec3_t	spot;
	int		cont;
	vec3_t	flatforward;

	if (pm->ps->pm_time) {
		return qfalse;
	}

	// check for water jump
	if ( pm->waterlevel != 2 ) {
		return qfalse;
	}

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

	VectorMA (pm->ps->origin, 30, flatforward, spot);
	spot[2] += 4;
	cont = pm->pointcontents (spot, pm->ps->clientNum );
	if ( !(cont & CONTENTS_SOLID) ) {
		return qfalse;
	}

	spot[2] += 16;
	cont = pm->pointcontents (spot, pm->ps->clientNum );
#ifdef TURTLEARENA // NO_BODY_TRACE
	if ( cont & pm->tracemask )
#else
	if ( cont & (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY) )
#endif
	{
		return qfalse;
	}

	// jump out of water
	VectorScale (pml.forward, 200, pm->ps->velocity);
#ifdef TA_PLAYERSYS
	pm->ps->velocity[2] = 350 * pm->playercfg->jumpMult;
#else
	pm->ps->velocity[2] = 350;
#endif

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return qtrue;
}

//============================================================================


/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
static void PM_WaterJumpMove( void ) {
	// waterjump has no control, but falls

	PM_StepSlideMove( qtrue );

	pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	if (pm->ps->velocity[2] < 0) {
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time = 0;
	}
}

/*
===================
PM_WaterMove

===================
*/
static void PM_WaterMove( void ) {
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;
	float	scale;
	float	vel;

	if ( PM_CheckWaterJump() ) {
		PM_WaterJumpMove();
		return;
	}
#if 0
	// jump = head for surface
	if ( pm->cmd.upmove >= 10 ) {
		if (pm->ps->velocity[2] > -300) {
			if ( pm->watertype == CONTENTS_WATER ) {
				pm->ps->velocity[2] = 100;
			} else if (pm->watertype == CONTENTS_SLIME) {
				pm->ps->velocity[2] = 80;
			} else {
				pm->ps->velocity[2] = 50;
			}
		}
	}
#endif
	PM_Friction ();

	scale = PM_CmdScale( &pm->cmd );
	//
	// user intentions
	//
	if ( !scale ) {
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = -60;		// sink towards bottom
	} else {
		for (i=0 ; i<3 ; i++)
			wishvel[i] = scale * pml.forward[i]*pm->cmd.forwardmove + scale * pml.right[i]*pm->cmd.rightmove;

		wishvel[2] += scale * pm->cmd.upmove;
	}

#ifdef TA_PATHSYS // 2DMODE
	PM_SetupPathWishVel(wishvel, wishdir);
#endif

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if ( wishspeed > pm->ps->speed * pm_swimScale ) {
		wishspeed = pm->ps->speed * pm_swimScale;
	}

	PM_Accelerate (wishdir, wishspeed, pm_wateraccelerate);

	// make sure we can go up slopes easily under water
	if ( pml.groundPlane && DotProduct( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0 ) {
		vel = VectorLength(pm->ps->velocity);
		// slide along the ground plane
		PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, 
			pm->ps->velocity, OVERCLIP );

		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, vel, pm->ps->velocity);
	}

	PM_SlideMove( qfalse );
}

#ifndef TURTLEARENA // POWERS
#ifdef MISSIONPACK
/*
===================
PM_InvulnerabilityMove

Only with the invulnerability powerup
===================
*/
static void PM_InvulnerabilityMove( void ) {
	pm->cmd.forwardmove = 0;
	pm->cmd.rightmove = 0;
	pm->cmd.upmove = 0;
	VectorClear(pm->ps->velocity);
}
#endif
#endif

/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void PM_FlyMove( void ) {
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;
	float	scale;

	// normal slowdown
	PM_Friction ();

	scale = PM_CmdScale( &pm->cmd );
	//
	// user intentions
	//
	if ( !scale ) {
		wishvel[0] = 0;
		wishvel[1] = 0;
#ifdef NIGHTSMODE
		if (pm->ps->powerups[PW_FLIGHT])
			wishvel[2] = -20;
		else
#endif
		wishvel[2] = 0;
	} else {
		for (i=0 ; i<3 ; i++) {
			wishvel[i] = scale * pml.forward[i]*pm->cmd.forwardmove + scale * pml.right[i]*pm->cmd.rightmove;
		}

		wishvel[2] += scale * pm->cmd.upmove;
	}

#ifdef TA_PATHSYS // 2DMODE
	PM_SetupPathWishVel(wishvel, wishdir);
#endif

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	PM_Accelerate (wishdir, wishspeed, pm_flyaccelerate);

	PM_StepSlideMove( qfalse );

#ifdef TA_NPCSYS
	if (pm->npc) {
		// Flying animation for flying NPCs
		PM_ContinueLegsAnim( OBJECT_WALK );
		return;
	}
#endif
#ifdef TA_WEAPSYS
	PM_ContinueLegsAnim( BG_LegsStandForPlayerState(pm->ps, pm->playercfg) );
#elif defined IOQ3ZTM // Don't use whatever random leg animation the player was in
	PM_ContinueLegsAnim( LEGS_IDLE );
#endif
}


/*
===================
PM_AirMove

===================
*/
static void PM_AirMove( void ) {
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;
	usercmd_t	cmd;

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	for ( i = 0 ; i < 2 ; i++ ) {
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	}
	wishvel[2] = 0;

#ifdef TA_PATHSYS // 2DMODE
	PM_SetupPathWishVel(wishvel, wishdir);
#endif

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	// not on ground, so little effect on velocity
#ifdef TA_NPCSYS // TDC_NPC
	if (pm->npc && cmd.upmove > 0)
		PM_Accelerate (wishdir, wishspeed, pm_accelerate); // just jumped, do normal acceleration
	else
#endif
	PM_Accelerate (wishdir, wishspeed, pm_airaccelerate);

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( pml.groundPlane ) {
		PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, 
			pm->ps->velocity, OVERCLIP );
	}

#if 0
	//ZOID:  If we are on the grapple, try stair-stepping
	//this allows a player to use the grapple to pull himself
	//over a ledge
	if (pm->ps->pm_flags & PMF_GRAPPLE_PULL)
		PM_StepSlideMove ( qtrue );
	else
		PM_SlideMove ( qtrue );
#endif

	PM_StepSlideMove ( qtrue );
}

/*
===================
PM_GrappleMove

===================
*/
static void PM_GrappleMove( void ) {
	vec3_t vel, v;
	float vlen;

	VectorScale(pml.forward, -16, v);
	VectorAdd(pm->ps->grapplePoint, v, v);
	VectorSubtract(v, pm->ps->origin, vel);
	vlen = VectorLength(vel);
	VectorNormalize( vel );

	if (vlen <= 100)
		VectorScale(vel, 10 * vlen, vel);
	else
		VectorScale(vel, 800, vel);

	VectorCopy(vel, pm->ps->velocity);

	pml.groundPlane = qfalse;
}

/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove( void ) {
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;
	usercmd_t	cmd;
	float		accelerate;
	float		vel;

	if ( pm->waterlevel > 2 && DotProduct( pml.forward, pml.groundTrace.plane.normal ) > 0
#ifdef IOQ3ZTM // WALK_UNDERWATER
		&& !(pml.groundPlane || pm->ps->groundEntityNum != ENTITYNUM_NONE)
#endif
		)
	{
		// begin swimming
		PM_WaterMove();
		return;
	}


	if ( PM_CheckJump () ) {
		// jumped away
		if ( pm->waterlevel > 1 ) {
			PM_WaterMove();
		} else {
			PM_AirMove();
		}
		return;
	}

	PM_Friction ();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity (pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP );
	PM_ClipVelocity (pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP );
	//
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	for ( i = 0 ; i < 3 ; i++ ) {
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	}
	// when going up or down slopes the wish velocity should Not be zero
//	wishvel[2] = 0;

#ifdef TA_PATHSYS // 2DMODE
	PM_SetupPathWishVel(wishvel, wishdir);
#endif

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	// clamp the speed lower if ducking
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		if ( wishspeed > pm->ps->speed * pm_duckScale ) {
			wishspeed = pm->ps->speed * pm_duckScale;
		}
	}

	// clamp the speed lower if wading or walking on the bottom
	if ( pm->waterlevel ) {
		float	waterScale;

		waterScale = pm->waterlevel / 3.0;
		waterScale = 1.0 - ( 1.0 - pm_swimScale ) * waterScale;
		if ( wishspeed > pm->ps->speed * waterScale ) {
			wishspeed = pm->ps->speed * waterScale;
		}
	}

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
		accelerate = pm_airaccelerate;
	} else {
#ifdef TA_WEAPSYS
		pm->xyspeed = sqrt( pm->ps->velocity[0] * pm->ps->velocity[0]
			+  pm->ps->velocity[1] * pm->ps->velocity[1] );

		// if not running, less movement while melee attacking, based on LoZ:TP
		if (pm->xyspeed < 200) {
			if (BG_MaxAttackIndex(pm->ps)-1 == BG_AttackIndexForPlayerState(pm->ps) && pm->ps->meleeDelay) {
				accelerate = 0;
			} else if (pm->ps->meleeTime) {
				accelerate = pm_accelerate/4;
			} else {
				accelerate = pm_accelerate;
			}
		}
		else
#endif
		accelerate = pm_accelerate;
	}

	PM_Accelerate (wishdir, wishspeed, accelerate);

	//Com_Printf("velocity = %1.1f %1.1f %1.1f\n", pm->ps->velocity[0], pm->ps->velocity[1], pm->ps->velocity[2]);
	//Com_Printf("velocity1 = %1.1f\n", VectorLength(pm->ps->velocity));

	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
		pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	} else {
		// don't reset the z velocity for slopes
//		pm->ps->velocity[2] = 0;
	}

	vel = VectorLength(pm->ps->velocity);

	// slide along the ground plane
	PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, 
		pm->ps->velocity, OVERCLIP );

	// don't decrease velocity when going up or down a slope
	VectorNormalize(pm->ps->velocity);
	VectorScale(pm->ps->velocity, vel, pm->ps->velocity);

	// don't do anything if standing still
	if (!pm->ps->velocity[0] && !pm->ps->velocity[1]) {
		return;
	}

	PM_StepSlideMove( qfalse );

	//Com_Printf("velocity2 = %1.1f\n", VectorLength(pm->ps->velocity));

}


/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove( void ) {
	float	forward;

	if ( !pml.walking ) {
		return;
	}

	// extra friction

	forward = VectorLength (pm->ps->velocity);
	forward -= 20;
	if ( forward <= 0 ) {
		VectorClear (pm->ps->velocity);
	} else {
		VectorNormalize (pm->ps->velocity);
		VectorScale (pm->ps->velocity, forward, pm->ps->velocity);
	}
}


/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove( void ) {
	float	speed, drop, friction, control, newspeed;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = VectorLength (pm->ps->velocity);
	if (speed < 1)
	{
		VectorCopy (vec3_origin, pm->ps->velocity);
	}
	else
	{
		drop = 0;

		friction = pm_friction*1.5;	// extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*friction*pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale (pm->ps->velocity, newspeed, pm->ps->velocity);
	}

	// accelerate
	scale = PM_CmdScale( &pm->cmd );

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;
	
	for (i=0 ; i<3 ; i++)
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	wishvel[2] += pm->cmd.upmove;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	// move
	VectorMA (pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);
}

//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number apropriate for the groundsurface
================
*/
static int PM_FootstepForSurface( void ) {
	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) {
		return 0;
	}
	if ( pml.groundTrace.surfaceFlags & SURF_METALSTEPS ) {
		return EV_FOOTSTEP_METAL;
	}
	return EV_FOOTSTEP;
}


/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand( void ) {
	float		delta;
	float		dist;
	float		vel, acc;
	float		t;
	float		a, b, c, den;

#ifdef IOQ3ZTM
	// ZTM: Don't land when swimming
	if ( pm->waterlevel == 3
#ifdef TA_NPCSYS // NPCs can't swim?
		&& !pm->npc
#endif
		)
	{
		return;
	}
#endif

	// decide which landing animation to use
#ifdef TA_NPCSYS
	if (pm->npc)
	{
		PM_ForceLegsAnim( OBJECT_LAND );
#if 0
		pm->ps->legsTimer = BG_AnimationTime(&pm->npc->info->animations[OBJECT_LAND]);
#else
		pm->ps->legsTimer = TIMER_LAND;
#endif
	} else {
#endif
	if ( pm->ps->pm_flags & PMF_BACKWARDS_JUMP ) {
		PM_ForceLegsAnim( LEGS_LANDB );
#if 0 // #ifdef TA_PLAYERSYS // PLAYERCFG_ANIMATION_TIMES // Doesn't work correctly?
		pm->ps->legsTimer = BG_AnimationTime(&pm->playercfg->animations[LEGS_LANDB]);
#endif
	} else {
		PM_ForceLegsAnim( LEGS_LAND );
#if 0 // #ifdef TA_PLAYERSYS // PLAYERCFG_ANIMATION_TIMES // Doesn't work correctly?
		pm->ps->legsTimer = BG_AnimationTime(&pm->playercfg->animations[LEGS_LAND]);
#endif
	}
#if 1 // #ifndef TA_PLAYERSYS // PLAYERCFG_ANIMATION_TIMES // Doesn't work correctly?
	pm->ps->legsTimer = TIMER_LAND;
#endif
#ifdef TA_NPCSYS
	}
#endif

	// calculate the exact velocity on landing
	dist = pm->ps->origin[2] - pml.previous_origin[2];
	vel = pml.previous_velocity[2];
	acc = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den =  b * b - 4 * a * c;
	if ( den < 0 ) {
		return;
	}
	t = (-b - sqrt( den ) ) / ( 2 * a );

	delta = vel + t * acc;
	delta = delta*delta * 0.0001;

	// ducking while falling doubles damage
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		delta *= 2;
	}

#ifndef IOQ3ZTM
	// never take falling damage if completely underwater
	if ( pm->waterlevel == 3 ) {
		return;
	}
#endif

	// reduce falling damage if there is standing water
	if ( pm->waterlevel == 2 ) {
		delta *= 0.25;
	}
	if ( pm->waterlevel == 1 ) {
		delta *= 0.5;
	}

	if ( delta < 1 ) {
		return;
	}

	// create a local entity event to play the sound

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	if ( !(pml.groundTrace.surfaceFlags & SURF_NODAMAGE) )  {
		if ( delta > 60 ) {
			PM_AddEvent( EV_FALL_FAR );
		} else if ( delta > 40 ) {
			// this is a pain grunt, so don't play it if dead
			if ( pm->ps->stats[STAT_HEALTH] > 0 ) {
				PM_AddEvent( EV_FALL_MEDIUM );
			}
		} else if ( delta > 7 ) {
			PM_AddEvent( EV_FALL_SHORT );
		} else {
			PM_AddEvent( PM_FootstepForSurface() );
		}
	}

	// start footstep cycle over
	pm->ps->bobCycle = 0;
}

/*
=============
PM_CheckStuck
=============
*/
/*
void PM_CheckStuck(void) {
	trace_t trace;

	pm->trace (&trace, pm->ps->origin, pm->ps->mins, pm->ps->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask);
	if (trace.allsolid) {
		//int shit = qtrue;
	}
}
*/

/*
=============
PM_CorrectAllSolid
=============
*/
static int PM_CorrectAllSolid( trace_t *trace ) {
	int			i, j, k;
	vec3_t		point;

	if ( pm->debugLevel ) {
		Com_Printf("%i:allsolid\n", c_pmove);
	}

	// jitter around
	for (i = -1; i <= 1; i++) {
		for (j = -1; j <= 1; j++) {
			for (k = -1; k <= 1; k++) {
				VectorCopy(pm->ps->origin, point);
				point[0] += (float) i;
				point[1] += (float) j;
				point[2] += (float) k;
				pm->trace (trace, point, pm->ps->mins, pm->ps->maxs, point, pm->ps->clientNum, pm->tracemask);
				if ( !trace->allsolid ) {
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25;

					pm->trace (trace, pm->ps->origin, pm->ps->mins, pm->ps->maxs, point, pm->ps->clientNum, pm->tracemask);
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;

	return qfalse;
}


/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed( void ) {
	trace_t		trace;
	vec3_t		point;

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {
		// we just transitioned into freefall
		if ( pm->debugLevel ) {
			Com_Printf("%i:lift\n", c_pmove);
		}

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy( pm->ps->origin, point );
		point[2] -= 64;

		pm->trace (&trace, pm->ps->origin, pm->ps->mins, pm->ps->maxs, point, pm->ps->clientNum, pm->tracemask);
		if ( trace.fraction == 1.0 ) {
			if ( pm->cmd.forwardmove >= 0 
#ifdef TA_PATHSYS // 2DMODE
			|| pm->ps->pathMode == PATHMODE_SIDE
#endif
			) {
#ifdef TA_NPCSYS
				if (pm->npc) {
					PM_ForceLegsAnim( OBJECT_JUMP );
				} else
#endif
				PM_ForceLegsAnim( LEGS_JUMP );
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			} else {
#ifdef TA_NPCSYS
				if (pm->npc) {
					PM_ForceLegsAnim( OBJECT_JUMP );
				} else
#endif
#ifdef TURTLEARENA // PLAYERS
				if (pm->ps && pm->ps->eFlags & EF_LOCKON) {
					PM_ForceLegsAnim( LEGS_JUMPB_LOCKON );
				} else
#endif
				PM_ForceLegsAnim( LEGS_JUMPB );
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
}


/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace( void ) {
	vec3_t		point;
	trace_t		trace;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] - 0.25;

	pm->trace (&trace, pm->ps->origin, pm->ps->mins, pm->ps->maxs, point, pm->ps->clientNum, pm->tracemask);
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if ( trace.allsolid ) {
		if ( !PM_CorrectAllSolid(&trace) )
			return;
	}

	// if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0 ) {
		PM_GroundTraceMissed();
#ifndef IOQ3ZTM // The same thing is done in PM_GroundTraceMissed
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
#endif
		return;
	}

	// check if getting thrown off the ground
	if ( pm->ps->velocity[2] > 0 && DotProduct( pm->ps->velocity, trace.plane.normal ) > 10 ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:kickoff\n", c_pmove);
		}
		// go into jump animation
		if ( pm->cmd.forwardmove >= 0 
#ifdef TA_PATHSYS // 2DMODE
			|| pm->ps->pathMode == PATHMODE_SIDE
#endif
		) {
#ifdef TA_NPCSYS
			if (pm->npc) {
				PM_ForceLegsAnim( OBJECT_JUMP );
			} else
#endif
			PM_ForceLegsAnim( LEGS_JUMP );
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		} else {
#ifdef TA_NPCSYS
			if (pm->npc) {
				PM_ForceLegsAnim( OBJECT_JUMP );
			} else
#endif
#ifdef TURTLEARENA // PLAYERS
			if (pm->ps && pm->ps->eFlags & EF_LOCKON) {
				PM_ForceLegsAnim( LEGS_JUMPB_LOCKON );
			} else
#endif
			PM_ForceLegsAnim( LEGS_JUMPB );
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}
	
	// slopes that are too steep will not be considered onground
	if ( trace.plane.normal[2] < MIN_WALK_NORMAL ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:steep\n", c_pmove);
		}
		// FIXME: if they can't slide down the slope, let them
		// walk (sharp crevices)
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qtrue;
		pml.walking = qfalse;
		return;
	}

	pml.groundPlane = qtrue;
	pml.walking = qtrue;

	// hitting solid ground will end a waterjump
	if (pm->ps->pm_flags & PMF_TIME_WATERJUMP)
	{
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time = 0;
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		// just hit the ground
		if ( pm->debugLevel ) {
			Com_Printf("%i:Land\n", c_pmove);
		}
		
#ifdef TA_NPCSYS // TDC_NPC
		// NPCs only land when jumping
		if (!pm->npc || (pm->npc && (pm->ps->legsAnim & ~ANIM_TOGGLEBIT ) == OBJECT_JUMP))
#endif
		PM_CrashLand();

		// don't do landing time if we were just going down a slope
		if ( pml.previous_velocity[2] < -200 ) {
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time = 250;
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;

	// don't reset the z velocity for slopes
//	pm->ps->velocity[2] = 0;

	PM_AddTouchEnt( trace.entityNum );
}


/*
=============
PM_SetWaterLevel	FIXME: avoid this twice?  certainly if not moving
=============
*/
static void PM_SetWaterLevel( void ) {
	vec3_t		point;
	int			cont;
	int			sample1;
	int			sample2;

	//
	// get waterlevel, accounting for ducking
	//
	pm->waterlevel = 0;
	pm->watertype = 0;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
#ifdef TA_NPCSYS
	if (pm->npc) {
		point[2] = pm->ps->origin[2] + pm->npc->info->mins[2] + 1;
	} else
#endif
#ifdef TA_PLAYERSYS // BOUNDINGBOX
		point[2] = pm->ps->origin[2] + pm->playercfg->bbmins[2] + 1;
#else
	point[2] = pm->ps->origin[2] + MINS_Z + 1;	
#endif

	cont = pm->pointcontents( point, pm->ps->clientNum );

	if ( cont & MASK_WATER ) {
#ifdef TA_NPCSYS
		if (pm->npc) {
			sample2 = pm->ps->viewheight - pm->npc->info->mins[2];
		} else
#endif
#ifdef TA_PLAYERSYS // BOUNDINGBOX
			sample2 = pm->ps->viewheight - pm->playercfg->bbmins[2];
#else
		sample2 = pm->ps->viewheight - MINS_Z;
#endif
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
#ifdef TA_NPCSYS
		if (pm->npc) {
			point[2] = pm->ps->origin[2] + pm->npc->info->mins[2] + sample1;
		} else
#endif
#ifdef TA_PLAYERSYS // BOUNDINGBOX
			point[2] = pm->ps->origin[2] + pm->playercfg->bbmins[2] + sample1;
#else
		point[2] = pm->ps->origin[2] + MINS_Z + sample1;
#endif
		cont = pm->pointcontents (point, pm->ps->clientNum );
		if ( cont & MASK_WATER ) {
			pm->waterlevel = 2;
#ifdef TA_NPCSYS
			if (pm->npc) {
				point[2] = pm->ps->origin[2] + pm->npc->info->mins[2] + sample2;
			} else
#endif
#ifdef TA_PLAYERSYS // BOUNDINGBOX
				point[2] = pm->ps->origin[2] + pm->playercfg->bbmins[2] + sample2;
#else
			point[2] = pm->ps->origin[2] + MINS_Z + sample2;
#endif
			cont = pm->pointcontents (point, pm->ps->clientNum );
			if ( cont & MASK_WATER ){
				pm->waterlevel = 3;
			}
		}
	}

}

/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static void PM_CheckDuck (void)
{
	trace_t	trace;

#ifdef TA_NPCSYS // TDC_NPC
	if (pm->npc)
	{
		return;
	}
#endif

#ifndef TURTLEARENA // POWERS
	if ( pm->ps->powerups[PW_INVULNERABILITY] ) {
		if ( pm->ps->pm_flags & PMF_INVULEXPAND ) {
			// invulnerability sphere has a 42 units radius
			VectorSet( pm->ps->mins, -42, -42, -42 );
			VectorSet( pm->ps->maxs, 42, 42, 42 );
		}
		else {
#ifdef TA_NPCSYS
			if (pm->npc) {
					VectorCopy( pm->npc->info->mins, pm->ps->mins );
					VectorCopy( pm->npc->info->maxs, pm->ps->maxs );
			} else {
#endif
#ifdef TA_PLAYERSYS // BOUNDINGBOX
				VectorCopy( pm->playercfg->bbmins, pm->ps->mins );
				VectorCopy( pm->playercfg->bbmaxs, pm->ps->maxs );
#else
			VectorSet( pm->ps->mins, -15, -15, MINS_Z );
			VectorSet( pm->ps->maxs, 15, 15, 16 );
#endif
#ifdef TA_NPCSYS
			}
#endif
		}
		pm->ps->pm_flags |= PMF_DUCKED;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
		return;
	}
	pm->ps->pm_flags &= ~PMF_INVULEXPAND;
#endif

#ifdef TA_NPCSYS
	if (pm->npc) {
		VectorCopy( pm->npc->info->mins, pm->ps->mins );
		pm->ps->maxs[0] = pm->npc->info->maxs[0];
		pm->ps->maxs[1] = pm->npc->info->maxs[1];
	} else {
#endif
#ifdef TA_PLAYERSYS // BOUNDINGBOX
		VectorCopy( pm->playercfg->bbmins, pm->ps->mins );

		pm->ps->maxs[0] = pm->playercfg->bbmaxs[0];
		pm->ps->maxs[1] = pm->playercfg->bbmaxs[1];
#else
	pm->ps->mins[0] = -15;
	pm->ps->mins[1] = -15;

	pm->ps->maxs[0] = 15;
	pm->ps->maxs[1] = 15;

	pm->ps->mins[2] = MINS_Z;
#endif
#ifdef TA_NPCSYS
	}
#endif

	if (pm->ps->pm_type == PM_DEAD)
	{
#ifdef TA_NPCSYS
		if (pm->npc) {
			pm->ps->maxs[2] = pm->npc->info->deadmax;
		} else
#endif
#ifdef TA_PLAYERSYS // BOUNDINGBOX
		pm->ps->maxs[2] = pm->playercfg->deadmax;
#else
		pm->ps->maxs[2] = -8;
#endif
		pm->ps->viewheight = DEAD_VIEWHEIGHT;
		return;
	}

	if (pm->cmd.upmove < 0)
	{	// duck
#ifdef IOQ3ZTM // ZTM: Only set ducked if on the ground
		if (pml.groundPlane)
#endif
		pm->ps->pm_flags |= PMF_DUCKED;
	}
	else
	{	// stand up if possible
		if (pm->ps->pm_flags & PMF_DUCKED)
		{
			// try to stand up
#ifdef TA_NPCSYS
			if (pm->npc) {
				pm->ps->maxs[2] = pm->npc->info->maxs[2];
			} else
#endif
#ifdef TA_PLAYERSYS // BOUNDINGBOX
				pm->ps->maxs[2] = pm->playercfg->bbmaxs[2];
#else
			pm->ps->maxs[2] = 32;
#endif
			pm->trace (&trace, pm->ps->origin, pm->ps->mins, pm->ps->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask );
			if (!trace.allsolid)
				pm->ps->pm_flags &= ~PMF_DUCKED;
		}
	}

	if (pm->ps->pm_flags & PMF_DUCKED)
	{
#ifdef TA_NPCSYS
		if (pm->npc) {
			pm->ps->maxs[2] = pm->npc->info->maxs[2] / 2;
		} else
#endif
#ifdef TA_PLAYERSYS // BOUNDINGBOX
			pm->ps->maxs[2] = pm->playercfg->bbmaxs[2] / 2;
#else
		pm->ps->maxs[2] = 16;
#endif
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
	}
	else
	{
#ifdef TA_NPCSYS
		if (pm->npc) {
			pm->ps->maxs[2] = pm->npc->info->maxs[2];
		} else
#endif
#ifdef TA_PLAYERSYS // BOUNDINGBOX
			pm->ps->maxs[2] = pm->playercfg->bbmaxs[2];
#else
		pm->ps->maxs[2] = 32;
#endif
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
	}
}



//===================================================================


/*
===============
PM_Footsteps
===============
*/
static void PM_Footsteps( void ) {
	float		bobmove;
	int			old;
	qboolean	footstep;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	pm->xyspeed = sqrt( pm->ps->velocity[0] * pm->ps->velocity[0]
		+  pm->ps->velocity[1] * pm->ps->velocity[1] );

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE
#ifdef IOQ3ZTM // Swimming and flying animation fix?
		&& !pml.groundPlane
#endif
#ifdef IOQ3ZTM // LADDER
		&& !pml.ladder
#endif
	) {

#ifndef TURTLEARENA // POWERS
		if ( pm->ps->powerups[PW_INVULNERABILITY] ) {
			PM_ContinueLegsAnim( LEGS_IDLECR );
		}
#endif
		// airborne leaves position in cycle intact, but doesn't advance
		if ( pm->waterlevel > 1 ) {
			PM_ContinueLegsAnim( LEGS_SWIM );
		}
		return;
	}

	// if not trying to move
#if defined TURTLEARENA && defined IOQ3ZTM // LADDER
	if( pml.ladder) {
		if ( !pm->cmd.forwardmove && !pm->cmd.upmove && !pm->cmd.rightmove ) {
			PM_ContinueAnim( BOTH_LADDER_STAND );
			return;
		}
	} else
#endif
	if ( !pm->cmd.forwardmove && !pm->cmd.rightmove ) {
		if (  pm->xyspeed < 5 ) {
			pm->ps->bobCycle = 0;	// start at beginning of cycle again
#ifdef TA_NPCSYS
			if ( pm->npc)
			{
				PM_ContinueLegsAnim( OBJECT_IDLE );
			}
			else
#endif
			if ( pm->ps->pm_flags & PMF_DUCKED ) {
				PM_ContinueLegsAnim( LEGS_IDLECR );
			} else {
#ifdef TA_WEAPSYS
				int anim = -1;

				// if melee attacking or firing gun
				if (pm->ps->weaponstate == WEAPON_FIRING) {
					anim = BG_LegsAttackForPlayerState(pm->ps, pm->playercfg);
				}

				// if not attacking or no attack animation
				if (anim < 0) {
					anim = BG_LegsStandForPlayerState(pm->ps, pm->playercfg);
				}

				PM_ContinueLegsAnim( anim );
#else
				PM_ContinueLegsAnim( LEGS_IDLE );
#endif
			}
#ifdef IOQ3ZTM // ZTM: Only return if started an animation
			return;
#endif
		}
#ifndef IOQ3ZTM
		return;
#endif
	}
	

	footstep = qfalse;

#if defined TURTLEARENA && defined IOQ3ZTM // LADDER
	if(pml.ladder) {
		bobmove = 0.3f;	// walking bobs slow
		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
			PM_ContinueAnim( BOTH_LADDER_DOWN );
		} else {
			PM_ContinueAnim( BOTH_LADDER_UP );
		}
	} else
#endif
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		bobmove = 0.5;	// ducked characters bob much faster
		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
			PM_ContinueLegsAnim( LEGS_BACKCR );
		}
		else {
			PM_ContinueLegsAnim( LEGS_WALKCR );
		}
		// ducked characters never play footsteps
	/*
	} else 	if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
		if ( !( pm->cmd.buttons & BUTTON_WALKING ) ) {
			bobmove = 0.4;	// faster speeds bob faster
			footstep = qtrue;
		} else {
			bobmove = 0.3;
		}
#ifdef TA_NPCSYS
		if (pm->npc)
			PM_ContinueLegsAnim( OBJECT_BACKPEDAL );
		else
#endif
		PM_ContinueLegsAnim( LEGS_BACK );
	*/
	} else {
		if ( !( pm->cmd.buttons & BUTTON_WALKING ) ) {
			bobmove = 0.4f;	// faster speeds bob faster
			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
#ifdef TA_NPCSYS
				if (pm->npc)
					PM_ContinueLegsAnim( OBJECT_BACKPEDAL );
				else
#endif
				PM_ContinueLegsAnim( LEGS_BACK );
			}
			else {
#ifdef TA_NPCSYS
				if (pm->npc)
					PM_ContinueLegsAnim( OBJECT_RUN );
				else
#endif
				PM_ContinueLegsAnim( LEGS_RUN );
			}
			footstep = qtrue;
		} else {
			bobmove = 0.3f;	// walking bobs slow
			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
#ifdef TA_NPCSYS
				if (pm->npc)
					PM_ContinueLegsAnim( OBJECT_BACKPEDAL );
				else
#endif
				PM_ContinueLegsAnim( LEGS_BACKWALK );
			}
			else {
#ifdef TA_NPCSYS
				if (pm->npc)
					PM_ContinueLegsAnim( OBJECT_WALK );
				else
#endif
				PM_ContinueLegsAnim( LEGS_WALK );
			}
		}
	}

	// check for footstep / splash sounds
	old = pm->ps->bobCycle;
	pm->ps->bobCycle = (int)( old + bobmove * pml.msec ) & 255;

	// if we just crossed a cycle boundary, play an apropriate footstep event
	if ( ( ( old + 64 ) ^ ( pm->ps->bobCycle + 64 ) ) & 128 ) {
		if ( pm->waterlevel == 0 ) {
			// on ground will only play sounds if running
			if ( footstep && !pm->noFootsteps ) {
				PM_AddEvent( PM_FootstepForSurface() );
			}
		} else if ( pm->waterlevel == 1 ) {
			// splashing
			PM_AddEvent( EV_FOOTSPLASH );
		} else if ( pm->waterlevel == 2 ) {
			// wading / swimming at surface
			PM_AddEvent( EV_SWIM );
		} else if ( pm->waterlevel == 3 ) {
			// no sound when completely underwater

		}
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents( void ) {		// FIXME?
	//
	// if just entered a water volume, play a sound
	//
	if (!pml.previous_waterlevel && pm->waterlevel) {
		PM_AddEvent( EV_WATER_TOUCH );
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (pml.previous_waterlevel && !pm->waterlevel) {
		PM_AddEvent( EV_WATER_LEAVE );
	}

	//
	// check for head just going under water
	//
	if (pml.previous_waterlevel != 3 && pm->waterlevel == 3) {
		PM_AddEvent( EV_WATER_UNDER );
	}

	//
	// check for head just coming out of water
	//
	if (pml.previous_waterlevel == 3 && pm->waterlevel != 3) {
		PM_AddEvent( EV_WATER_CLEAR );
	}
}


/*
===============
PM_BeginWeaponChange
===============
*/
static void PM_BeginWeaponChange( int weapon ) {
	if ( weapon <= WP_NONE ||
#ifdef TA_WEAPSYS
		weapon >= BG_NumWeaponGroups()
#else
		weapon >= WP_NUM_WEAPONS
#endif
		)
	{
		return;
	}

#ifndef TA_WEAPSYS_EX
	if ( !( pm->ps->stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
		return;
	}
#endif
	
	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		return;
	}

	PM_AddEvent( EV_CHANGE_WEAPON );
	pm->ps->weaponstate = WEAPON_DROPPING;
#ifdef TURTLEARENA // WEAPONS
	{
		animNumber_t anim = TORSO_DROP;

		if (pm->ps->stats[STAT_DEFAULTWEAPON] == pm->ps->weapon)
		{
			if (pm->ps->weaponHands & HB_BOTH)
			{
				anim = TORSO_PUTDEFAULT_BOTH;
			}
			else if (pm->ps->weaponHands & HB_PRIMARY)
			{
				anim = TORSO_PUTDEFAULT_PRIMARY;
			}
			else if (pm->ps->weaponHands & HB_SECONDARY)
			{
				anim = TORSO_PUTDEFAULT_SECONDARY;
			}
			pm->ps->weaponTime += BG_AnimationTime(&pm->playercfg->animations[anim])/2;
		}
		else
		{
			pm->ps->weaponTime += BG_AnimationTime(&pm->playercfg->animations[anim]);
		}

		PM_StartTorsoAnim( anim );
	}
#else
	pm->ps->weaponTime += 200;
	PM_StartTorsoAnim( TORSO_DROP );
#endif
}


/*
===============
PM_FinishWeaponChange
===============
*/
static void PM_FinishWeaponChange( void ) {
	int		weapon;
#ifdef TA_WEAPSYS_EX
	int oldWeapon;

	oldWeapon = pm->ps->weapon;
#endif

#ifdef TA_WEAPSYS_EX
	weapon = pm->ps->stats[STAT_PENDING_WEAPON];
#else
	weapon = pm->cmd.weapon;
#endif
	if ( weapon < WP_NONE ||
#ifdef TA_WEAPSYS
		weapon >= BG_NumWeaponGroups()
#else
		weapon >= WP_NUM_WEAPONS
#endif
		)
	{
		weapon = WP_NONE;
	}

#ifndef TA_WEAPSYS_EX
	if ( !( pm->ps->stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
		weapon = WP_NONE;
	}
#endif

	pm->ps->weapon = weapon;
	pm->ps->weaponstate = WEAPON_RAISING;
#ifdef TA_WEAPSYS
	pm->ps->weaponHands = BG_WeaponHandsForPlayerState(pm->ps);
#endif
#ifdef TURTLEARENA // WEAPONS // PLAYERCFG_ANIMATION_TIMES
	{
		animNumber_t anim = TORSO_RAISE;

		if (pm->ps->stats[STAT_DEFAULTWEAPON] == weapon)
		{
			if (pm->ps->weaponHands & HB_BOTH)
			{
				anim = TORSO_GETDEFAULT_BOTH;
			}
			else if (pm->ps->weaponHands & HB_PRIMARY)
			{
				anim = TORSO_GETDEFAULT_PRIMARY;
			}
			else if (pm->ps->weaponHands & HB_SECONDARY)
			{
				anim = TORSO_GETDEFAULT_SECONDARY;
			}
		}

		// If started a animation, continue it.
		if ( ( pm->ps->torsoAnim & ~ANIM_TOGGLEBIT ) >= TORSO_PUTDEFAULT_BOTH
			&& ( pm->ps->torsoAnim & ~ANIM_TOGGLEBIT ) <= TORSO_GETDEFAULT_SECONDARY)
		{
			pm->ps->weaponTime += BG_AnimationTime(&pm->playercfg->animations[anim]) / 2;
			PM_ContinueTorsoAnim( anim );
		}
		else
		{
			pm->ps->weaponTime += BG_AnimationTime(&pm->playercfg->animations[anim]);
			PM_StartTorsoAnim( anim );
		}
	}
#else
	pm->ps->weaponTime += 250;
	PM_StartTorsoAnim( TORSO_RAISE );
#endif
#ifdef TA_WEAPSYS_EX
	// Drop weapon if not default
	if (oldWeapon != pm->ps->stats[STAT_DEFAULTWEAPON])
	{
		// drop weapon
		pm->ps->stats[STAT_DROP_WEAPON] = oldWeapon;
		pm->ps->stats[STAT_DROP_AMMO] = pm->ps->stats[STAT_AMMO];
		PM_AddEvent( EV_DROP_WEAPON );
	}

	// Setup ammo
	if (pm->ps->stats[STAT_PENDING_WEAPON] == pm->ps->stats[STAT_DEFAULTWEAPON])
	{
		// Default weapons don't use ammo.
		pm->ps->stats[STAT_AMMO] = -1;
	}
	else
	{
		// Set ammo for new weapon.
		pm->ps->stats[STAT_AMMO] = pm->ps->stats[STAT_PENDING_AMMO];
		pm->ps->stats[STAT_PENDING_AMMO] = -1;
	}
#endif
}

#ifdef TURTLEARENA // WEAPONS
/*
===============
PM_BeginWeaponChange

hands = 0 = can't attack, caring a object over head?
hands = 1 = primary only
hands = 2 = secondary only
hands = 3 = primary and secondary.

===============
*/
static void PM_BeginWeaponHandsChange( int hands ) {
	int last_hands;
	animNumber_t anim;

	if ( hands < HB_NONE || hands >= HB_MAX ) {
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_DROPPING
		|| pm->ps->weaponstate == WEAPON_HAND_CHANGE ) {
		return;
	}

	if (hands == pm->ps->weaponHands) {
		// no change
		return;
	}

	//PM_AddEvent( EV_CHANGE_WEAPON ); // Play change sound here?
	pm->ps->weaponstate = WEAPON_HAND_CHANGE;

	last_hands = pm->ps->weaponHands;

	// Store hands to be set in PM_FinishWeaponHandsChange
	pm->ps->stats[STAT_NEW_WEAPON_HANDS] = hands;

	//Com_Printf("DEBUG: Changing hands (new=%d, old=%d)\n", hands, last_hands);

	anim = TORSO_DROP;

	if (pm->ps->stats[STAT_DEFAULTWEAPON] == pm->ps->weapon)
	{
		// both hands
		if (last_hands == HB_BOTH && hands == HB_NONE)
		{
			anim = TORSO_PUTDEFAULT_BOTH;
		}
		else if (last_hands == HB_NONE && hands == HB_BOTH)
		{
			anim = TORSO_GETDEFAULT_BOTH;
		}
		// primary hand
		else if ((last_hands == HB_BOTH && hands == HB_SECONDARY) || (last_hands == HB_PRIMARY && hands == HB_NONE))
		{
			anim = TORSO_PUTDEFAULT_PRIMARY;
		}
		else if ((last_hands == HB_SECONDARY && hands == HB_BOTH) || (last_hands == HB_NONE && hands == HB_PRIMARY))
		{
			anim = TORSO_GETDEFAULT_PRIMARY;
		}
		// secondary hand
		else if (last_hands == HB_BOTH && hands == HB_PRIMARY)
		{
			anim = TORSO_PUTDEFAULT_SECONDARY;
		}
		else if (last_hands == HB_PRIMARY && hands == HB_BOTH)
		{
			anim = TORSO_GETDEFAULT_SECONDARY;
		}
		else
		{
			// ZTM: Shouldn't happen, if I made it right...
			Com_Printf("PM_BeginDefaultWeaponChange: Bad hands; last_hands=%i, hands=%i\n", last_hands, hands);
		}

		// The animation is "drop" and "raise", so split time in half.
		pm->ps->weaponTime += BG_AnimationTime(&pm->playercfg->animations[anim]) / 2;
		// Don't override gesture when capturing the flag.
		if (!pm->ps->torsoTimer) {
			PM_StartTorsoAnim( anim );
		}
	}
	else
	{
		pm->ps->weaponTime += BG_AnimationTime(&pm->playercfg->animations[anim]);
		// Don't override gesture when capturing the flag.
		if (!pm->ps->torsoTimer) {
			PM_StartTorsoAnim( anim );
		}
	}
}


/*
===============
PM_FinishWeaponHandsChange
===============
*/
static void PM_FinishWeaponHandsChange( void ) {
	int last_hands, hands;
	animNumber_t anim;

	// Reusing WEAPON_RAISING should be okay here.
	pm->ps->weaponstate = WEAPON_RAISING;

	last_hands = pm->ps->weaponHands;
	hands = pm->ps->stats[STAT_NEW_WEAPON_HANDS];

	pm->ps->weaponHands = pm->ps->stats[STAT_NEW_WEAPON_HANDS];

	anim = TORSO_RAISE;

	if (pm->ps->stats[STAT_DEFAULTWEAPON] == pm->ps->weapon)
	{
		// both hands
		if (last_hands == HB_BOTH && hands == HB_NONE)
		{
			anim = TORSO_PUTDEFAULT_BOTH;
		}
		else if (last_hands == HB_NONE && hands == HB_BOTH)
		{
			anim = TORSO_GETDEFAULT_BOTH;
		}
		// primary hand
		else if (last_hands == HB_BOTH && hands == HB_SECONDARY)
		{
			anim = TORSO_PUTDEFAULT_PRIMARY;
		}
		else if (last_hands == HB_SECONDARY && hands == HB_BOTH)
		{
			anim = TORSO_GETDEFAULT_PRIMARY;
		}
		// secondary hand
		else if (last_hands == HB_BOTH && hands == HB_PRIMARY)
		{
			anim = TORSO_PUTDEFAULT_SECONDARY;
		}
		else if (last_hands == HB_PRIMARY && hands == HB_BOTH)
		{
			anim = TORSO_GETDEFAULT_SECONDARY;
		}

		// If started a animation, continue it.
		if ( ( pm->ps->torsoAnim & ~ANIM_TOGGLEBIT ) >= TORSO_PUTDEFAULT_BOTH
			&& ( pm->ps->torsoAnim & ~ANIM_TOGGLEBIT ) <= TORSO_GETDEFAULT_SECONDARY)
		{
			pm->ps->weaponTime += BG_AnimationTime(&pm->playercfg->animations[anim]) / 2;
			PM_ContinueTorsoAnim( anim );
			return;
		}
	}

	pm->ps->weaponTime += BG_AnimationTime(&pm->playercfg->animations[anim]);
	// Don't override gesture when capturing the flag.
	if (!pm->ps->torsoTimer) {
		PM_StartTorsoAnim( anim );
	}
}
#endif

/*
==============
PM_TorsoAnimation

==============
*/
static void PM_TorsoAnimation( void ) {
	if ( pm->ps->weaponstate == WEAPON_READY ) {
#ifdef TA_WEAPSYS // ZTM: Weapon type code.
		PM_ContinueTorsoAnim( BG_TorsoStandForPlayerState(pm->ps, pm->playercfg) );
#else
		if ( pm->ps->weapon == WP_GAUNTLET ) {
			PM_ContinueTorsoAnim( TORSO_STAND2 );
		} else {
			PM_ContinueTorsoAnim( TORSO_STAND );
		}
#endif
		return;
	}
}

#ifdef TA_HOLDSYS
static void PM_NextHoldable(void)
{
	int i;

	// Change to the next valid holdable item.
	for ( i = 1 ; i < BG_NumHoldableItems() ; i++ ) {
		pm->ps->holdableIndex++;
		if ( pm->ps->holdableIndex == BG_NumHoldableItems() ) {
			pm->ps->holdableIndex = 1;
		}

#ifndef MISSIONPACK // if not MP skip its holdables.
		if (
#ifndef TURTLEARENA // NO_KAMIKAZE_ITEM
		pm->ps->holdableIndex == HI_KAMIKAZE ||
#endif
		pm->ps->holdableIndex == HI_PORTAL
#ifndef TURTLEARENA // POWERS
			|| pm->ps->holdableIndex == HI_INVULNERABILITY
#endif
			) {
			continue;
		}
#endif

		if ( pm->ps->holdable[pm->ps->holdableIndex] != 0 ) {
			break;
		}
	}
	if ( i == BG_NumHoldableItems() ) {
		// None are valid so use none.
		pm->ps->holdableIndex = HI_NONE;
	}
}
#endif

/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter
==============
*/
static void PM_Weapon( void ) {
	int		addTime;

	// don't allow attack until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return;
	}

	// ignore if spectator
	if ( pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	// check for dead player
	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
#ifndef TA_WEAPSYS // Game says if player has a weapon. Set to WP_NONE in TossClientItems
		pm->ps->weapon = WP_NONE;
#endif
		return;
	}

#ifdef TA_HOLDSYS
	// Check if valid, in cgame we have to pass HI_NO_SELECT (255)
	//   so bg can change to the next holdable.
	if (pm->cmd.holdable != HI_NONE && pm->cmd.holdable < BG_NumHoldableItems() && pm->ps->holdable[pm->cmd.holdable] != 0) {
		pm->ps->holdableIndex = pm->cmd.holdable;
	}
#endif

#ifdef TURTLEARENA // HOLD_SHURIKEN
	// make holdable function
	if ( pm->ps->holdableTime > 0 ) {
		pm->ps->holdableTime -= pml.msec;
	}
#endif

	// check for item using
	if ( pm->cmd.buttons & BUTTON_USE_HOLDABLE ) {
		if (
#ifdef TURTLEARENA // HOLD_SHURIKEN
		pm->ps->holdableTime <= 0
#else
		! ( pm->ps->pm_flags & PMF_USE_ITEM_HELD )
#endif
#ifdef TA_HOLDSYS
			&& (pm->ps->holdable[pm->ps->holdableIndex] != 0 || !pm->ps->holdableIndex)
#endif
#ifdef TA_ENTSYS // FUNC_USE
			&& !(pm->ps->eFlags & EF_USE_ENT)
#endif
#ifdef TA_PLAYERSYS // LADDER // Don't allow haldable items to be used while on a ladder (mainly don't want shurikens to be used)
			&& !(pm->ps->eFlags & EF_LADDER)
#endif
		) {
			if (
#ifdef TA_HOLDSYS
			pm->ps->holdableIndex == HI_MEDKIT
#elif defined TA_ITEMSYS
			BG_ItemForItemNum(pm->ps->stats[STAT_HOLDABLE_ITEM])->giTag == HI_MEDKIT
#else
			bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag == HI_MEDKIT
#endif
				&& pm->ps->stats[STAT_HEALTH] >= (pm->ps->stats[STAT_MAX_HEALTH] + 25) ) {
				// don't use medkit if at max health
			} else {
#ifdef TURTLEARENA // HOLD_SHURIKEN
				pm->ps->holdableTime = 500;
#else
				pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
#endif
#ifdef TA_HOLDSYS
				PM_AddEvent( EV_USE_ITEM0 + pm->ps->holdableIndex );
#elif defined TA_ITEMSYS
				PM_AddEvent( EV_USE_ITEM0 + BG_ItemForItemNum(pm->ps->stats[STAT_HOLDABLE_ITEM])->giTag );
#else
				PM_AddEvent( EV_USE_ITEM0 + bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag );
#endif
#ifdef TA_HOLDSYS
				if (pm->ps->holdable[pm->ps->holdableIndex] > 0
#ifdef TURTLEARENA // HOLD_SHURIKEN // Grappling shurikens don't use ammo
					&& !bg_projectileinfo[BG_ProjectileIndexForHoldable(pm->ps->holdableIndex)].grappling
#endif
				) {
					pm->ps->holdable[pm->ps->holdableIndex]--;

					// It holdable item can no longer be used,
					// auto select next valid holdable item, if none selects HI_NONE.
					if (pm->ps->holdable[pm->ps->holdableIndex] == 0) {
						PM_NextHoldable();
					}
				}
#else
				pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
#endif
			}
			return;
		}
	}
	else {
		pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;
	}


	// make weapon function
	if ( pm->ps->weaponTime > 0 ) {
		pm->ps->weaponTime -= pml.msec;
	}

#ifdef TA_WEAPSYS_EX
	// Drop pickup weapon (User pressed key)
	// and check for out of ammo for pickup weapons
	if ( pm->ps->weaponTime <= 0 && pm->ps->weaponstate != WEAPON_FIRING
		&& ((pm->cmd.buttons & BUTTON_DROP_WEAPON)
		|| (pm->ps->stats[STAT_AMMO] == 0
#ifdef MISSIONPACK // ZTM: Don't auto drop if have ammo regen!
#ifdef TA_WEAPSYS
		&& BG_ItemForItemNum(pm->ps->stats[STAT_PERSISTANT_POWERUP])->giTag != PW_AMMOREGEN
#else
		&& bg_itemlist[pm->ps->stats[STAT_PERSISTANT_POWERUP]].giTag != PW_AMMOREGEN
#endif
#endif
		))
		&& pm->ps->weapon != pm->ps->stats[STAT_DEFAULTWEAPON])
	{
		pm->ps->stats[STAT_PENDING_WEAPON] = pm->ps->stats[STAT_DEFAULTWEAPON];
	}
#endif

	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising
	if ( pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING ) {
#ifdef TA_WEAPSYS_EX
		if ( pm->ps->weapon != pm->ps->stats[STAT_PENDING_WEAPON] ) {
			PM_BeginWeaponChange( pm->ps->stats[STAT_PENDING_WEAPON] );
		}
#else
		if ( pm->ps->weapon != pm->cmd.weapon ) {
			PM_BeginWeaponChange( pm->cmd.weapon );
		}
#endif
#ifdef TURTLEARENA // WEAPONS
		// Just check the ones we can do it in?
		if (pm->ps->weaponstate != WEAPON_DROPPING
			&& pm->ps->weaponstate != WEAPON_HAND_CHANGE)
		{
			int hands = BG_WeaponHandsForPlayerState(pm->ps);
			if (pm->ps->weaponHands != hands)
			{
				PM_BeginWeaponHandsChange(hands);
			}
		}
#endif
	}

	if ( pm->ps->weaponTime > 0 ) {
		return;
	}

	// change weapon if time
	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		PM_FinishWeaponChange();
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_RAISING ) {
#ifdef TA_WEAPSYS
		pm->ps->weaponstate = WEAPON_READY;
		PM_StartTorsoAnim( BG_TorsoStandForPlayerState(pm->ps, pm->playercfg) );
		PM_StartLegsAnim( BG_LegsStandForPlayerState(pm->ps, pm->playercfg) );
#else
		pm->ps->weaponstate = WEAPON_READY;
		if ( pm->ps->weapon == WP_GAUNTLET ) {
			PM_StartTorsoAnim( TORSO_STAND2 );
		} else {
			PM_StartTorsoAnim( TORSO_STAND );
		}
#endif
		return;
	}

#ifdef TURTLEARENA // WEAPONS
	if ( pm->ps->weaponstate == WEAPON_HAND_CHANGE ) {
		PM_FinishWeaponHandsChange();
		return;
	}
#endif


#ifdef IOQ3ZTM
	// Handle grapple
#ifdef TA_WEAPSYS
	if (bg_weapongroupinfo[pm->ps->weapon].weapon[0]->proj->grappling)
#else
	if (pm->ps->weapon == WP_GRAPPLING_HOOK)
#endif
	{
		// If player has a shot grapple don't play attack animation.
		if (pm->ps->pm_flags & PMF_FIRE_HELD)
		{
#ifdef TA_WEAPSYS
			PM_ContinueTorsoAnim( BG_TorsoStandForPlayerState(pm->ps, pm->playercfg) );
#else
			PM_ContinueTorsoAnim( TORSO_STAND );
#endif

			// No weapon changing.
			pm->ps->weaponstate = WEAPON_FIRING;
			pm->ps->weaponTime += 500;
			return;
		}
	}
#endif

#ifdef TA_WEAPSYS // ZTM: Weapon type code.
	if ( BG_WeaponHasMelee(pm->ps->weapon) )
	{
		// check for fire (Melee weapons can do damage while not holding attack)
		if ( (!pm->ps->meleeTime && !pm->ps->meleeDelay)
#ifdef TA_PLAYERSYS // LADDER // Don't allow weapons to be used while on ladders
			|| (pm->ps->eFlags & EF_LADDER)
#endif
			)
		{
			pm->ps->weaponTime = 0;
			pm->ps->weaponstate = WEAPON_READY;
			return;
		}
	}
	else
	{
		// check for fire
		if ( !(pm->cmd.buttons & BUTTON_ATTACK) || pm->ps->weaponHands == HB_NONE
#ifdef TA_PLAYERSYS // LADDER // Don't allow weapons to be used while on ladders
			|| (pm->ps->eFlags & EF_LADDER)
#endif
			)
		{
			pm->ps->weaponTime = 0;
			pm->ps->weaponstate = WEAPON_READY;
			return;
		}
	}

	// Gauntlet-type
	if ( BG_WeaponHasType(pm->ps->weapon, WT_GAUNTLET) ) {
		// the guantlet only "fires" when it actually hits something
		if ( !pm->gauntletHit ) {
			pm->ps->weaponTime = 0;
			pm->ps->weaponstate = WEAPON_READY;
			return;
		}
	}
#else
	// check for fire
	if ( ! (pm->cmd.buttons & BUTTON_ATTACK) ) {
		pm->ps->weaponTime = 0;
		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	// start the animation even if out of ammo
	if ( pm->ps->weapon == WP_GAUNTLET ) {
		// the guantlet only "fires" when it actually hits something
		if ( !pm->gauntletHit ) {
			pm->ps->weaponTime = 0;
			pm->ps->weaponstate = WEAPON_READY;
			return;
		}
		PM_StartTorsoAnim( TORSO_ATTACK2 );
	} else {
		PM_StartTorsoAnim( TORSO_ATTACK );
	}
#endif

#ifndef TA_WEAPSYS
	pm->ps->weaponstate = WEAPON_FIRING;
#endif

#ifdef TA_WEAPSYS_EX
	// check for out of ammo,
	//  only happens for default weapon.
	//  pickup weapons have already been handled.
	if ( ! pm->ps->stats[STAT_AMMO] ) {
		//PM_AddEvent( EV_NOAMMO );
		pm->ps->weaponTime += 500;
		return;
	}
#else
	// check for out of ammo
	if ( ! pm->ps->ammo[ pm->ps->weapon ] ) {
		PM_AddEvent( EV_NOAMMO );
		pm->ps->weaponTime += 500;
		return;
	}
#endif

	// take an ammo away if not infinite
#ifdef TA_WEAPSYS_EX
	if (pm->ps->stats[STAT_AMMO] != -1) {
		pm->ps->stats[STAT_AMMO]--;
	}
#else
	if ( pm->ps->ammo[ pm->ps->weapon ] != -1 ) {
		pm->ps->ammo[ pm->ps->weapon ]--;
	}
#endif

#ifdef TA_WEAPSYS
	// Make sure we have ammo before attacking.
	pm->ps->weaponstate = WEAPON_FIRING;

	// MELEEATTACK
	if ( BG_WeaponHasMelee(pm->ps->weapon) )
	{
		int anim;

		anim = BG_TorsoAttackForPlayerState(pm->ps);

		// If this isn't the current animation, start it
		if ((pm->ps->torsoAnim & ~ANIM_TOGGLEBIT ) != anim)
		{
			PM_StartTorsoAnim( anim );
			PM_StartLegsAnim( BG_LegsAttackForPlayerState(pm->ps, pm->playercfg) );

			// fire weapon
			PM_AddEvent( EV_FIRE_WEAPON );
		}
	}
	else
	{
		PM_StartTorsoAnim( BG_TorsoAttackForPlayerState(pm->ps) );
		PM_StartLegsAnim( BG_LegsAttackForPlayerState(pm->ps, pm->playercfg) );

		// fire weapon
		PM_AddEvent( EV_FIRE_WEAPON );
	}
#else
	// fire weapon
	PM_AddEvent( EV_FIRE_WEAPON );
#endif

#ifdef TA_WEAPSYS
	if (BG_WeaponHasType(pm->ps->weapon, WT_MELEE))
	{
		if (pm->ps->meleeTime)
			pm->ps->weaponTime = pm->ps->meleeTime;
		else
			pm->ps->weaponTime = pm->ps->meleeDelay;
		return;
	}

	addTime = bg_weapongroupinfo[pm->ps->weapon].weapon[0]->attackDelay;
#else
	switch( pm->ps->weapon ) {
	default:
	case WP_GAUNTLET:
		addTime = 400;
		break;
	case WP_LIGHTNING:
		addTime = 50;
		break;
	case WP_SHOTGUN:
		addTime = 1000;
		break;
	case WP_MACHINEGUN:
		addTime = 100;
		break;
	case WP_GRENADE_LAUNCHER:
		addTime = 800;
		break;
	case WP_ROCKET_LAUNCHER:
		addTime = 800;
		break;
	case WP_PLASMAGUN:
		addTime = 100;
		break;
	case WP_RAILGUN:
		addTime = 1500;
		break;
	case WP_BFG:
		addTime = 200;
		break;
	case WP_GRAPPLING_HOOK:
		addTime = 400;
		break;
#ifdef MISSIONPACK
	case WP_NAILGUN:
		addTime = 1000;
		break;
	case WP_PROX_LAUNCHER:
		addTime = 800;
		break;
	case WP_CHAINGUN:
		addTime = 30;
		break;
#endif
	}
#endif

#ifdef MISSIONPACK
#ifdef TA_ITEMSYS
	if( BG_ItemForItemNum(pm->ps->stats[STAT_PERSISTANT_POWERUP])->giTag == PW_SCOUT )
#else
	if( bg_itemlist[pm->ps->stats[STAT_PERSISTANT_POWERUP]].giTag == PW_SCOUT )
#endif
	{
		addTime /= 1.5;
	}
	else
#ifdef TA_ITEMSYS
	if( BG_ItemForItemNum(pm->ps->stats[STAT_PERSISTANT_POWERUP])->giTag == PW_AMMOREGEN )
#else
	if( bg_itemlist[pm->ps->stats[STAT_PERSISTANT_POWERUP]].giTag == PW_AMMOREGEN )
#endif
	{
		addTime /= 1.3;
  }
  else
#endif
	if ( pm->ps->powerups[PW_HASTE] ) {
		addTime /= 1.3;
	}

	pm->ps->weaponTime += addTime;
}

/*
================
PM_Animate
================
*/

static void PM_Animate( void ) {
	if ( pm->cmd.buttons & BUTTON_GESTURE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GESTURE );
#ifdef TA_PLAYERSYS // PLAYERCFG_ANIMATION_TIMES
			pm->ps->torsoTimer = BG_AnimationTime(&pm->playercfg->animations[TORSO_GESTURE]);
#else
			pm->ps->torsoTimer = TIMER_GESTURE;
#endif
			PM_AddEvent( EV_TAUNT );
		}
#ifdef MISSIONPACK
	} else if ( pm->cmd.buttons & BUTTON_GETFLAG ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GETFLAG );
#ifdef TA_PLAYERSYS // PLAYERCFG_ANIMATION_TIMES
			pm->ps->torsoTimer = BG_AnimationTime(&pm->playercfg->animations[TORSO_GETFLAG]);
#else
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
#endif
		}
	} else if ( pm->cmd.buttons & BUTTON_GUARDBASE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GUARDBASE );
#ifdef TA_PLAYERSYS // PLAYERCFG_ANIMATION_TIMES
			pm->ps->torsoTimer = BG_AnimationTime(&pm->playercfg->animations[TORSO_GUARDBASE]);
#else
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
#endif
		}
	} else if ( pm->cmd.buttons & BUTTON_PATROL ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_PATROL );
#ifdef TA_PLAYERSYS // PLAYERCFG_ANIMATION_TIMES
			pm->ps->torsoTimer = BG_AnimationTime(&pm->playercfg->animations[TORSO_PATROL]);
#else
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
#endif
		}
	} else if ( pm->cmd.buttons & BUTTON_FOLLOWME ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_FOLLOWME );
#ifdef TA_PLAYERSYS // PLAYERCFG_ANIMATION_TIMES
			pm->ps->torsoTimer = BG_AnimationTime(&pm->playercfg->animations[TORSO_FOLLOWME]);
#else
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
#endif
		}
	} else if ( pm->cmd.buttons & BUTTON_AFFIRMATIVE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_AFFIRMATIVE );
#ifdef TA_PLAYERSYS // PLAYERCFG_ANIMATION_TIMES
			pm->ps->torsoTimer = BG_AnimationTime(&pm->playercfg->animations[TORSO_AFFIRMATIVE]);
#else
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
#endif
		}
	} else if ( pm->cmd.buttons & BUTTON_NEGATIVE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_NEGATIVE );
#ifdef TA_PLAYERSYS // PLAYERCFG_ANIMATION_TIMES
			pm->ps->torsoTimer = BG_AnimationTime(&pm->playercfg->animations[TORSO_NEGATIVE]);
#else
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
#endif
		}
#endif
	}
}


/*
================
PM_DropTimers
================
*/
static void PM_DropTimers( void ) {
	// drop misc timing counter
	if ( pm->ps->pm_time ) {
		if ( pml.msec >= pm->ps->pm_time ) {
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		} else {
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
	if ( pm->ps->legsTimer > 0 ) {
		pm->ps->legsTimer -= pml.msec;
		if ( pm->ps->legsTimer < 0 ) {
			pm->ps->legsTimer = 0;
		}
	}

	if ( pm->ps->torsoTimer > 0 ) {
		pm->ps->torsoTimer -= pml.msec;
		if ( pm->ps->torsoTimer < 0 ) {
			pm->ps->torsoTimer = 0;
		}
	}
}

/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated isntead of a full move
================
*/
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd ) {
	short		temp;
	int		i;

	if ( ps->pm_type == PM_INTERMISSION
#ifndef TA_SP
		|| ps->pm_type == PM_SPINTERMISSION
#endif
		)
	{
		return;		// no view changes at all
	}

	if ( ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0 ) {
		return;		// no view changes at all
	}

	// circularly clamp the angles with deltas
	for (i=0 ; i<3 ; i++) {
#ifdef TA_PATHSYS // 2DMODE
		if (ps->pathMode == PATHMODE_SIDE && i == YAW)
			continue;
#endif
		temp = cmd->angles[i] + ps->delta_angles[i];
		if ( i == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) {
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp = 16000;
			} else if ( temp < -16000 ) {
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp = -16000;
			}
		}
		ps->viewangles[i] = SHORT2ANGLE(temp);
	}

}


#ifdef IOQ3ZTM // LADDER
/*
===================
PM_LadderMove()
by: Calrathan [Arthur Tomlin]

Right now all I know is that this works for VERTICAL ladders.
Ladders with angles on them (urban2 for AQ2) haven't been tested.
===================
*/
static void PM_LadderMove( void ) {
	int i;
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;
	float speedScale;
	float scale;
	float vel;

	//
	// FIRST CHECK IF THE PLAYER HAS TO BE ABLE TO WALK BACKWARDS
	//
	trace_t trace;
	vec3_t origin;
	qboolean backwards =  qfalse;

	VectorCopy(pm->ps->origin, origin);
	origin[2] -= 20;

	pm->trace (&trace, pm->ps->origin, pm->ps->mins, pm->ps->maxs, origin,
		pm->ps->clientNum, pm->tracemask);

	if(trace.fraction < 1)
		backwards = qtrue;
	// CHECK END

	PM_Friction ();

	scale = PM_CmdScale( &pm->cmd );

	// user intentions [what the user is attempting to do]
	if ( !scale ) {
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = 0;

		// Snap to 8 unit grid, so player always holds onto ladder correctly!
		//  (But only if not moving and if on a vertical ladder)
		if (!VectorLength(pm->ps->velocity) && pm->ps->origin2[2] == 0)
		{
			float baseZ = (pm->ps->origin[2] + pm->ps->mins[2]);
			int baseZint = (int)baseZ;
			int offset = baseZint % 8;

			if (offset) {
				// Snap in the direction the player was moving
				if (pm->ps->pm_flags & PMF_BACKWARDS_RUN) {
					// Do nothing
				} else {
					offset = 8 - abs(offset);
				}

				//Com_Printf("DEBUG: Snap ladder origin %f (%d) %s to %d (ofs=%d)\n", baseZ, baseZint, (pm->ps->pm_flags & PMF_BACKWARDS_RUN) ? "down" : "up", baseZint + offset - (int)pm->ps->mins[2], offset);
				pm->ps->origin[2] = baseZint + offset - (int)pm->ps->mins[2];
			}
		}
	} else {   // if they're trying to move... lets calculate it
		for (i=0 ; i<3 ; i++){
			float fw = pm->cmd.forwardmove, rt = pm->cmd.rightmove;
			if(fw < 0 && !backwards)
				fw = 0;

			wishvel[i] = scale * pml.forward[i]*fw +
				     scale * pml.right[i]*rt;
		}
		wishvel[2] = scale * pm->cmd.forwardmove;

		if (wishvel[2] < 0) {
			pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
		} else {
			pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
		}
	}

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if ( pm->waterlevel > 1 ) {
		speedScale = pm_ladderWaterScale;
	} else {
		speedScale = pm_ladderScale;
	}

	if ( wishspeed > pm->ps->speed * speedScale ) {
		wishspeed = pm->ps->speed * speedScale;
	}

	PM_Accelerate (wishdir, wishspeed, pm_ladderAccelerate);

	// This SHOULD help us with sloped ladders, but it remains untested.
	if ( pml.groundPlane && DotProduct( pm->ps->velocity,
		pml.groundTrace.plane.normal ) < 0 ) {
		vel = VectorLength(pm->ps->velocity);
		// slide along the ground plane [the ladder section under our feet]
		PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal,
			pm->ps->velocity, OVERCLIP );

		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, vel, pm->ps->velocity);

#ifdef TA_PLAYERSYS // LADDER
		// Save ladder dir, inv of trace.plane.normal
		VectorCopy(pml.groundTrace.plane.normal, pm->ps->origin2);
		VectorInverse(pm->ps->origin2);
#endif
	}

	PM_SlideMove( qfalse ); // move without gravity
}


/*
=============
CheckLadder [ ARTHUR TOMLIN ]
=============
*/
void CheckLadder( void )
{
	vec3_t flatforward,spot;
	vec3_t origin;
	trace_t trace;
	qboolean backwards;

	// Don't climb ladder while ducking, if on ladder player will fall down
	if (pm->cmd.upmove < 0) {
		if (pm->ps->eFlags & EF_LADDER) {
			// Jump away from ladder
			VectorMA(pm->ps->velocity, (JUMP_VELOCITY * pm->playercfg->jumpMult) * -0.2f, pml.forward, pm->ps->velocity);

			pml.groundPlane = qfalse;		// jumping away
			pml.walking = qfalse;

			pm->ps->groundEntityNum = ENTITYNUM_NONE;
#ifdef TA_NPCSYS // TDC_NPC
			if (pm->npc) {
				pm->ps->velocity[2] = pm->cmd.upmove*8;
			} else
#endif
#ifdef TA_PLAYERSYS
			pm->ps->velocity[2] = (JUMP_VELOCITY * pm->playercfg->jumpMult) * 0.6f;
#else
			pm->ps->velocity[2] = JUMP_VELOCITY;
#endif
			PM_AddEvent( EV_JUMP );

#ifdef TA_NPCSYS
			if (pm->npc) {
				PM_ForceLegsAnim( OBJECT_JUMP );
			} else
#endif
			PM_ForceLegsAnim( LEGS_JUMP );
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		}
		goto notOnLadder;
	} else if (pm->cmd.upmove > 0) {
		// must wait for jump to be released
		if ( pm->ps->pm_flags & PMF_JUMP_HELD ) {
			// clear upmove so cmdscale doesn't lower running speed
			pm->cmd.upmove = 0;
		}
		// Jump off ladder
		else if (!(pm->ps->pm_flags & PMF_RESPAWNED) && (pm->cmd.upmove >= 10) && (pm->ps->eFlags & EF_LADDER)) {
			// Turn around
			pm->ps->delta_angles[YAW] += ANGLE2SHORT(180);

			// Jump away from ladder
			VectorMA(pm->ps->velocity, (JUMP_VELOCITY * pm->playercfg->jumpMult) * -0.4f, pml.forward, pm->ps->velocity);

			pml.groundPlane = qfalse;		// jumping away
			pml.walking = qfalse;
			pm->ps->pm_flags |= PMF_JUMP_HELD;

			pm->ps->groundEntityNum = ENTITYNUM_NONE;
#ifdef TA_NPCSYS // TDC_NPC
			if (pm->npc) {
				pm->ps->velocity[2] = pm->cmd.upmove*8;
			} else
#endif
#ifdef TA_PLAYERSYS
			pm->ps->velocity[2] = (JUMP_VELOCITY * pm->playercfg->jumpMult) * 0.6f;
#else
			pm->ps->velocity[2] = JUMP_VELOCITY;
#endif
			PM_AddEvent( EV_JUMP );

#ifdef TA_NPCSYS
			if (pm->npc) {
				PM_ForceLegsAnim( OBJECT_JUMP );
			} else
#endif
			PM_ForceLegsAnim( LEGS_JUMP );
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;

			goto notOnLadder;
		} else {
			goto notOnLadder;
		}
	}

	// check if we should be backwards walking down the ladder
	backwards = qfalse;

	VectorCopy(pm->ps->origin, origin);
	origin[2] -= 30;

	pm->trace (&trace, pm->ps->origin, pm->ps->mins, pm->ps->maxs, origin,
		pm->ps->clientNum, pm->tracemask);

	if(trace.fraction == 1)
		backwards = qtrue;
	// check end

	VectorCopy(pm->ps->origin, origin);
	if(backwards)
		origin[2] -= 20;

	// check for ladder
	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;

	VectorNormalize (flatforward);
	VectorMA (origin, 2, flatforward, spot);

	pm->trace (&trace, origin, pm->ps->mins, pm->ps->maxs, spot,
		pm->ps->clientNum, pm->tracemask);

	if ((trace.fraction < 1) && (trace.surfaceFlags & SURF_LADDER)) {
		pml.ladder = qtrue;

#ifdef TA_PLAYERSYS // LADDER
		pm->ps->eFlags |= EF_LADDER;
		// Save ladder dir, inv of trace.plane.normal
		VectorCopy(trace.plane.normal, pm->ps->origin2);
		VectorInverse(pm->ps->origin2);
#endif
	} else {
notOnLadder:
		pml.ladder = qfalse;

#ifdef TA_PLAYERSYS // LADDER
		if (pm->ps->eFlags & EF_LADDER) {
			pm->ps->eFlags &= ~EF_LADDER;
			// Clear ladder dir
			VectorClear(pm->ps->origin2);
		}
#endif
	}
}
#endif

/*
================
PmoveSingle

================
*/
void trap_SnapVector( float *v );

void PmoveSingle (pmove_t *pmove) {
	pm = pmove;

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
	c_pmove++;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

#ifndef TURTLEARENA // NO_BODY_TRACE
	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->tracemask &= ~CONTENTS_BODY;	// corpses can fly through bodies
	}
#endif

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if ( abs( pm->cmd.forwardmove ) > 64 || abs( pm->cmd.rightmove ) > 64 ) {
		pm->cmd.buttons &= ~BUTTON_WALKING;
	}

#ifdef TURTLEARENA // LOCKON
	// set the lock-on flag
	if ( pm->ps->stats[STAT_HEALTH] > 0 && (pm->cmd.buttons & BUTTON_WALKING) ) {
		pm->ps->eFlags |= EF_LOCKON;
	} else {
		pm->ps->eFlags &= ~EF_LOCKON;
	}
#endif

	// set the talk balloon flag
	if ( pm->cmd.buttons & BUTTON_TALK ) {
		pm->ps->eFlags |= EF_TALK;
	} else {
		pm->ps->eFlags &= ~EF_TALK;
	}

	// set the firing flag for continuous beam weapons
	if ( !(pm->ps->pm_flags & PMF_RESPAWNED) && pm->ps->pm_type != PM_INTERMISSION && pm->ps->pm_type != PM_NOCLIP
#ifdef TA_WEAPSYS
		&& ( pm->ps->meleeTime ||
			(( pm->cmd.buttons & BUTTON_ATTACK )
#ifdef TA_WEAPSYS_EX
			&& pm->ps->stats[STAT_AMMO] != 0
#else
			&& pm->ps->ammo[ pm->ps->weapon ] != 0
#endif
			) )
#else
		&& ( pm->cmd.buttons & BUTTON_ATTACK ) && pm->ps->ammo[ pm->ps->weapon ]
#endif
		) {
		pm->ps->eFlags |= EF_FIRING;
	} else {
		pm->ps->eFlags &= ~EF_FIRING;
	}

	// clear the respawned flag if attack and use are cleared
	if ( pm->ps->stats[STAT_HEALTH] > 0 && 
		!( pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE) ) ) {
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if ( pmove->cmd.buttons & BUTTON_TALK ) {
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}
#ifdef TA_SP
	// Don't move after you have finished the level
	else if ( pm->ps->eFlags & EF_FINISHED ) {
		pmove->cmd.buttons = 0;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}
#endif

	// clear all pmove local vars
	memset (&pml, 0, sizeof(pml));

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if ( pml.msec < 1 ) {
		pml.msec = 1;
	} else if ( pml.msec > 200 ) {
		pml.msec = 200;
	}
	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	VectorCopy (pm->ps->origin, pml.previous_origin);

	// save old velocity for crashlanding
	VectorCopy (pm->ps->velocity, pml.previous_velocity);

	pml.frametime = pml.msec * 0.001;

#ifdef TA_NPCSYS // TDC_NPC
	if (!pm->npc)
	{
#endif
	// update the viewangles
	PM_UpdateViewAngles( pm->ps, &pm->cmd );
#ifdef TA_NPCSYS
	}
#endif

	AngleVectors (pm->ps->viewangles, pml.forward, pml.right, pml.up);

	if ( pm->cmd.upmove < 10 ) {
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
	}

	// decide if backpedaling animations should be used
#ifdef TA_PATHSYS // 2DMODE
	if (pm->ps->pathMode == PATHMODE_SIDE) {
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	} else
#endif
	if ( pm->cmd.forwardmove < 0 ) {
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	} else if ( pm->cmd.forwardmove > 0 || ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove ) ) {
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if ( pm->ps->pm_type >= PM_DEAD ) {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR ) {
#ifdef TA_PATHSYS
		PM_PathMoveInital();
#endif
#ifdef IOQ3ZTM // set pml.groundPlane before PM_CheckDuck
		// set pml.groundPlane
		PM_GroundTrace();
#endif
		PM_CheckDuck ();
		PM_FlyMove ();
		PM_DropTimers ();
		return;
	}

	if ( pm->ps->pm_type == PM_NOCLIP ) {
		PM_NoclipMove ();
		PM_DropTimers ();
		return;
	}

	if (pm->ps->pm_type == PM_FREEZE) {
		return;		// no movement at all
	}

	if ( pm->ps->pm_type == PM_INTERMISSION
#ifndef TA_SP
		|| pm->ps->pm_type == PM_SPINTERMISSION
#endif
		)
	{
		return;		// no movement at all
	}

#ifdef TA_PATHSYS
	// Inital path movement
	PM_PathMoveInital();
#endif

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	// set mins, maxs, and viewheight
	PM_CheckDuck ();

	// set groundentity
	PM_GroundTrace();

#ifdef IOQ3ZTM // Must set bbox before GroundTrace, and must call GroundTrace to duck
	PM_CheckDuck ();
#endif

	if ( pm->ps->pm_type == PM_DEAD ) {
		PM_DeadMove ();
	}

	PM_DropTimers();
#ifdef IOQ3ZTM // LADDER
	CheckLadder();  // check and see if we're on a ladder
#endif

#ifdef TA_PLAYERSYS
    // Setup accelerates based on the per-player one.
	if (pm->playercfg)
	{
		pm_accelerate = pm->playercfg->accelerate_speed;
	}
	else // NPCs
	{
		pm_accelerate = 10;
	}
	pm_airaccelerate = pm_accelerate * 0.1f;
	pm_wateraccelerate = pm_accelerate * 0.4f;
	pm_flyaccelerate = pm_accelerate * 0.8f;
#endif

#ifndef TURTLEARENA // POWERS
#ifdef MISSIONPACK
	if ( pm->ps->powerups[PW_INVULNERABILITY] ) {
		PM_InvulnerabilityMove();
	} else
#endif
#endif
	if ( pm->ps->powerups[PW_FLIGHT] ) {
#ifdef IOQ3ZTM // Use grapple while flying
		if (pm->ps->pm_flags & PMF_GRAPPLE_PULL) {
			PM_GrappleMove();
		}
		else
#endif
#ifdef IOQ3ZTM // LADDER
		if (pml.ladder) {
			PM_LadderMove();
		}
		else
#endif
		// flight powerup doesn't allow jump and has different friction
		PM_FlyMove();
	} else if (pm->ps->pm_flags & PMF_GRAPPLE_PULL) {
		PM_GrappleMove();
		// We can wiggle a bit
		PM_AirMove();
	} else if (pm->ps->pm_flags & PMF_TIME_WATERJUMP) {
		PM_WaterJumpMove();
#ifdef IOQ3ZTM // LADDER
	} else if (pml.ladder) {
		PM_LadderMove();
#endif
	} else if ( pm->waterlevel > 1 ) {
#ifdef IOQ3ZTM // WALK_UNDERWATER
		if (pm->waterlevel == 3 &&
				(pml.groundPlane || pm->ps->groundEntityNum != ENTITYNUM_NONE))
		{
			// walking on ground
			PM_WalkMove();
		}
		else
#endif
		// swimming
		PM_WaterMove();
	} else if ( pml.walking ) {
		// walking on ground
		PM_WalkMove();
	} else {
		// airborne
		PM_AirMove();
	}

#ifdef TA_NPCSYS // TDC_NPC
	if (!pm->npc)
	{
#endif
	PM_Animate();
#ifdef TA_NPCSYS
	}
#endif

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();
	PM_SetWaterLevel();

#ifdef TA_NPCSYS // TDC_NPC
	if (!pm->npc)
	{
#endif
	// weapons
	PM_Weapon();

	// torso animation
	PM_TorsoAnimation();
#ifdef TA_NPCSYS
	}
#endif

	// footstep events / legs animations
	PM_Footsteps();

	// entering / leaving water splashes
	PM_WaterEvents();

	// snap some parts of playerstate to save network bandwidth
	trap_SnapVector( pm->ps->velocity );
}


/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove (pmove_t *pmove) {
	int			finalTime;

#ifdef TA_PLAYERSYS
	if (!pmove->playercfg)
	{
		// Spectators were not passing playercfg...
		Com_Error(ERR_DROP, "Pmove: playercfg is NULL!\n");
		return;
	}
#endif

	finalTime = pmove->cmd.serverTime;

	if ( finalTime < pmove->ps->commandTime ) {
		return;	// should not happen
	}

	if ( finalTime > pmove->ps->commandTime + 1000 ) {
		pmove->ps->commandTime = finalTime - 1000;
	}

	pmove->ps->pmove_framecount = (pmove->ps->pmove_framecount+1) & ((1<<PS_PMOVEFRAMECOUNTBITS)-1);

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while ( pmove->ps->commandTime != finalTime ) {
		int		msec;

		msec = finalTime - pmove->ps->commandTime;

		if ( pmove->pmove_fixed ) {
			if ( msec > pmove->pmove_msec ) {
				msec = pmove->pmove_msec;
			}
		}
		else {
			if ( msec > 66 ) {
				msec = 66;
			}
		}
		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle( pmove );

		if ( pmove->ps->pm_flags & PMF_JUMP_HELD ) {
			pmove->cmd.upmove = 20;
		}
	}

	//PM_CheckStuck();

}

