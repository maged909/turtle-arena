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
// g_weapon.c 
// perform the server side effects of a weapon firing

#include "g_local.h"

static	float	s_quadFactor;
static	vec3_t	forward, right, up;
static	vec3_t	muzzle;

#ifndef TA_WEAPSYS
#define NUM_NAILSHOTS 15
#endif

/*
================
G_BounceProjectile
================
*/
void G_BounceProjectile( vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout ) {
	vec3_t v, newv;
	float dot;

	VectorSubtract( impact, start, v );
	dot = DotProduct( v, dir );
	VectorMA( v, -2*dot, dir, newv );

	VectorNormalize(newv);
	VectorMA(impact, 8192, newv, endout);
}


#ifdef TURTLEARENA // LOCKON
/*
================
G_AutoAim

In third person it's hard to aim, so give them some help.
================
*/
void G_AutoAim(gentity_t *ent, int projnum, vec3_t start, vec3_t forward, vec3_t right, vec3_t up)
{
	gentity_t *target;
	vec3_t targetOrigin;
	vec3_t dir;
	int i;
	float maxRand;
	float range;
	float angle;

	angle = 120.0f;
	range = 768.0f;
	if (bg_projectileinfo[projnum].instantDamage) {
		range = bg_projectileinfo[projnum].speed;
	}

	// Clients only use auto aim if locked on to a entity
	if (ent->player && !(ent->player->ps.eFlags & EF_LOCKON)) {
		target = NULL;
	} else {
		// Client locked-on, misc_shooter, ...
		target = ent->enemy;
	}

	if (target && (target == ent || !target->takedamage))
	{
		target = NULL;
	}

	// Clients update target (ent->enemy) each frame.
	if (
#ifdef TURTLEARENA // LOCKON
		!ent->player &&
#endif
		!G_ValidTarget(ent, target, start, forward, range, angle, 2))
	{
		// Search for a target
		target = G_FindTarget(ent, start, forward, range, angle);
	}

	if (!target) {
		return;
	}

	// Aim higher then origin
	VectorCopy(target->r.currentOrigin, targetOrigin);
	for (i = 0; i < 3; i++)
	{
		targetOrigin[i] += (target->s.mins[i] + target->s.maxs[i]) * 0.5;
		// Move around a bit so that shurikens don't all stick in the same spot.
		maxRand = target->s.maxs[i] * 0.5;
		targetOrigin[i] += maxRand - (random() * (maxRand * 2));
	}

	// Get direction to aim
	VectorSubtract( targetOrigin, start, dir );
	VectorNormalize( dir );

	// Get up and right from dir (which is "forward")
	VectorCopy( dir, forward );
	PerpendicularVector( up, dir );
	CrossProduct( up, dir, right );
}
#endif

#ifdef TURTLEARENA // HOLD_SHURIKEN
/*
================
G_ThrowShuriken

Spawns shuriken missile based on holdable number.

ZTM: TODO: Player animation for throw shuriken?
================
*/
void G_ThrowShuriken(gentity_t *ent, holdable_t holdable)
{
	if (!BG_ProjectileIndexForHoldable(holdable))
		return;

	// set aiming directions
	AngleVectors (ent->player->ps.viewangles, forward, right, up);
	CalcMuzzlePoint ( ent, forward, right, up, muzzle );

	fire_shuriken (ent, muzzle, forward, right, up, holdable);
}
#endif

#ifdef TA_WEAPSYS // MELEEATTACK
#define MELEE_CHAINTIME 1500
void G_StartMeleeAttack(gentity_t *ent)
{
	gplayer_t *player = ent->player;

	// Make sure it is a melee weapon.
	if ( !BG_WeaponHasMelee(player->ps.weapon)
		|| player->ps.weaponHands == HB_NONE
#ifdef TA_PLAYERSYS // LADDER
		|| (player->ps.eFlags & EF_LADDER)
#endif
		)
	{
		return;
	}

	// Next attack animation
	player->ps.meleeAttack++;

	// ZTM: Use the animation time for the attack time!
	player->ps.meleeTime = BG_AnimationTime(&player->pers.playercfg.animations[BG_TorsoAttackForPlayerState(&player->ps)]);
	player->ps.meleeLinkTime = 1.5f * player->ps.meleeTime; // MELEE_CHAINTIME

	//G_Printf("DEBUG: player %i started new melee attack (%i)\n", ent - g_entities, player->ps.meleeAttack);

	// Play sound and stuff.
	G_AddEvent( ent, EV_FIRE_WEAPON, 0 );
}

#ifdef TA_GAME_MODELS
/*
======================
G_PositionEntityOnTag

Modifies the child's position and axis by the given
tag location
======================
*/
static qboolean G_PositionEntityOnTag( orientation_t *child, lerpFrame_t *parentLF,
	orientation_t parent, qhandle_t parentModel, char *tagName )
{
	int				i;
	orientation_t	lerped;
	qboolean		returnValue;

	// lerp the tag
	returnValue = trap_R_LerpTag( &lerped, parentModel, parentLF->oldFrame, parentLF->frame,
		1.0 - parentLF->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent.origin, child->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( child->origin, lerped.origin[i], parent.axis[i], child->origin );
	}

	MatrixMultiply( lerped.axis, parent.axis, child->axis );

	return returnValue;
}


/*
======================
G_PositionRotatedEntityOnTag

Modifies the child's position and axis by the given
tag location
======================
*/
static qboolean G_PositionRotatedEntityOnTag( orientation_t *child, lerpFrame_t *parentLF,
	orientation_t parent, qhandle_t parentModel, char *tagName )
{
	int				i;
	orientation_t	lerped;
	vec3_t			tempAxis[3];
	qboolean		returnValue;

//AxisClear( entity->axis );
	// lerp the tag
	returnValue = trap_R_LerpTag( &lerped, parentModel, parentLF->oldFrame, parentLF->frame,
		1.0 - parentLF->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent.origin, child->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( child->origin, lerped.origin[i], parent.axis[i], child->origin );
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply( child->axis, lerped.axis, tempAxis );
	MatrixMultiply( tempAxis, parent.axis, child->axis );

	return returnValue;
}

/*
======================
G_SetupPlayerTagOrientation

Sets up the orientation to use G_PositionEntityOnTag and G_PositionRotatedEntityOnTag.
======================
*/
qboolean G_SetupPlayerTagOrientation(gentity_t *ent, orientation_t *legsOrientation, orientation_t *torsoOrientation)
{
	qhandle_t model;

	if (!ent->player->pers.legsModel || !ent->player->pers.torsoModel) {
		//G_Printf("DEBUG: G_SetupPlayerTagOrientation: Failed to load model.\n");
		return qfalse;
	}

	// Set to player's origin
	VectorCopy(ent->r.currentOrigin, legsOrientation->origin);

	// Use pre-calculated axes
	AxisCopy(ent->player->pers.legsAxis, legsOrientation->axis);
	AxisCopy(ent->player->pers.torsoAxis, torsoOrientation->axis);

	model = ent->player->pers.legsModel;

	// Find torso origin
	if (!G_PositionRotatedEntityOnTag(torsoOrientation, &ent->player->pers.legs, *legsOrientation, model, "tag_torso")) {
		//G_Printf("DEBUG: G_SetupPlayerTagOrientation: Failed to find tag_torso\n");
		return qfalse;
	}

	return qtrue;
}
#endif

// Based on CheckGauntletHit
qboolean G_MeleeDamageSingle(gentity_t *ent, qboolean checkTeamHit, int hand, weapontype_t wt)
{
	trace_t			tr;
	vec3_t			start;
	vec3_t			end;
	vec3_t			pushDir; // dir for knockback
	gentity_t		*tent;
	gentity_t		*traceEnt;
	int				damage;
	int				dflags;
	weapon_t		weaponGroupNum;
	int				mod; // Means of death
	bg_weaponinfo_t	*weapon;
	int				i;
	qboolean		traceHit;
	qhandle_t		bodyModel;
	lerpFrame_t		*bodyLerp;
	orientation_t	weaponOrientation;
#ifdef TA_GAME_MODELS
	orientation_t	legsOrientation;
	orientation_t	torsoOrientation;
#endif

	if (!BG_WeapTypeIsMelee(wt)) {
		return qfalse;
	}

#ifdef TA_GAME_MODELS // Use the weapon tag angles and pos!
	// Setup the orientations
	if (!G_SetupPlayerTagOrientation(ent, &legsOrientation, &torsoOrientation)) {
		return qfalse;
	}

	memset(&weaponOrientation, 0, sizeof (weaponOrientation));
#endif

	dflags = 0;
	weaponGroupNum = ent->player->ps.weapon;

	bodyModel = ent->player->pers.torsoModel;
	bodyLerp = &ent->player->pers.torso;

	// Use hand to select the weapon tag.
	if (hand == HAND_PRIMARY)
	{
		if (ent->player->melee_debounce &&
			ent->player->melee_debounce > level.time)
		{
			return qfalse;
		}
#ifdef TA_GAME_MODELS // TA_WEAPSYS
		// put weapon on torso
#ifdef TURTLEARENA // PLAYERS
		if (!G_PositionEntityOnTag(&weaponOrientation, bodyLerp,
			torsoOrientation, bodyModel, "tag_hand_primary"))
#endif
		{
#if !defined TURTLEARENA || defined TA_SUPPORTQ3 // PLAYERS
			if (!G_PositionEntityOnTag(&weaponOrientation, bodyLerp,
				torsoOrientation, bodyModel, "tag_weapon"))
#endif
			{
				// Failed to put weapon on torso!
				return qfalse;
			}
		}
#else
		// set aiming directions
		AngleVectors (ent->player->ps.viewangles, forward, right, up);

		CalcMuzzlePoint ( ent, forward, right, up, muzzle );
#endif
	}
	else
	{
		if (ent->player->melee_debounce2 &&
			ent->player->melee_debounce2 > level.time)
		{
			return qfalse;
		}
#ifdef TA_GAME_MODELS // TA_WEAPSYS
		// put weapon on torso
#ifdef TURTLEARENA // PLAYERS
		if (!G_PositionEntityOnTag(&weaponOrientation, bodyLerp,
			torsoOrientation, bodyModel, "tag_hand_secondary"))
#endif
		{
#if !defined TURTLEARENA || defined TA_SUPPORTQ3 // PLAYERS
			if (!G_PositionEntityOnTag(&weaponOrientation, bodyLerp,
				torsoOrientation, bodyModel, "tag_flag"))
#endif
			{
				// Failed to put weapon on torso!
				return qfalse;
			}
		}
#else
		return qfalse;
#endif
	}

#ifdef TA_GAME_MODELS // TA_WEAPSYS
	// Setup "start" and "end" using weaponOrientation
	VectorCopy(weaponOrientation.axis[0], forward);
	VectorCopy(weaponOrientation.axis[1], right);
	VectorCopy(weaponOrientation.axis[2], up);
	VectorCopy(weaponOrientation.origin, muzzle);
#else
	// Setup weaponOrientation, so that the old code works with new code.
	VectorCopy(forward, weaponOrientation.axis[0]);
	VectorCopy(right, weaponOrientation.axis[1]);
	VectorCopy(up, weaponOrientation.axis[2]);
	VectorCopy(muzzle, weaponOrientation.origin);
#endif

	// Using "up" makes players fly (really high) into the air when hit.
	//VectorCopy(up, pushDir);
	VectorCopy(forward, pushDir);

	traceHit = qfalse;

	weapon = bg_weapongroupinfo[weaponGroupNum].weapon[hand];
	mod = weapon->mod;

	// Use default kill message
	if (mod == MOD_UNKNOWN) {
		if (hand == HAND_PRIMARY) {
			mod = MOD_WEAPON_PRIMARY;
		} else {
			mod = MOD_WEAPON_SECONDARY;
		}
	}

	if (weapon->flags & WIF_CUTS)
	{
		dflags |= DAMAGE_CUTS;
	}

	if (ent->player->ps.powerups[PW_QUAD] ) {
		s_quadFactor = g_quadfactor.value;
	} else {
		s_quadFactor = 1;
	}
#ifdef MISSIONPACK
	if( ent->player->persistantPowerup && ent->player->persistantPowerup->item && ent->player->persistantPowerup->item->giTag == PW_DOUBLER ) {
		s_quadFactor *= 2;
	}
#endif
#ifdef TA_PLAYERSYS
	if (ent->player->pers.playercfg.ability == ABILITY_STRENGTH) {
		s_quadFactor *= 1.3f;
	}
#endif

	for (i = 0; i < MAX_WEAPON_BLADES; i++)
	{
		if (weapon->blades[i].damage == 0)
			continue;

		// Move forward, right, up
		VectorMA (weaponOrientation.origin, weapon->blades[i].origin[2], up, start);
		VectorMA (weaponOrientation.origin, weapon->blades[i].tip[2], up, end);

		VectorMA (start, weapon->blades[i].origin[1], right, start);
		VectorMA (end, weapon->blades[i].tip[1], right, end);

		VectorMA (start, weapon->blades[i].origin[0], forward, start);
		VectorMA (end, weapon->blades[i].tip[0], forward, end);

		trap_Trace (&tr, start, NULL, NULL, end, ent->s.number, MASK_SHOT);

		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			continue;
		}

		damage = weapon->blades[i].damage * s_quadFactor;

		if ( !checkTeamHit && (tr.startsolid || (tr.contents & CONTENTS_SOLID)) && !(weapon->flags & WIF_ALWAYS_DAMAGE) ) {
			// Push player away from trace dir!
			// Based on code in G_Damage
			vec3_t	kvel;
			float	mass;
			int knockback;

			knockback = 5+damage*1.5f;

			mass = 200;

			VectorScale (tr.plane.normal, g_knockback.value * (float)knockback / mass, kvel);
			VectorAdd (ent->player->ps.velocity, kvel, ent->player->ps.velocity);

			ent->player->ps.meleeDelay = 1000;

			// set the timer so that the other player can't cancel
			// out the movement immediately
			if ( !ent->player->ps.pm_time ) {
				int		t;

				t = knockback * 2;
				if ( t < 50 ) {
					t = 50;
				}
				if ( t > 200 ) {
					t = 200;
				}

				ent->player->ps.pm_time = t;
				ent->player->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			}
		}

		traceEnt = &g_entities[ tr.entityNum ];

#ifdef MISSIONPACK
		// Can't damage your own obelisk
		if (traceEnt->pain == ObeliskPain
			&& traceEnt->spawnflags == ent->player->sess.sessionTeam)
		{
			continue;
		}
#endif

		if (traceEnt->mustcut && !(dflags & DAMAGE_CUTS))
		{
			continue;
		}

		if (checkTeamHit)
		{
			if (traceEnt->takedamage && OnSameTeam(ent, traceEnt))
			{
				return qtrue;
			}
			continue;
		}

#ifdef TURTLEARENA // POWERS
		// Don't show hit effects on clients who can't be damaged
		if (traceEnt->player && (traceEnt->player->ps.powerups[PW_FLASHING]
			|| traceEnt->player->ps.powerups[PW_INVUL])) {
			continue;
		}
#endif

		// Client hit an entity
		if (!traceEnt->player || (traceEnt->player &&
			(!OnSameTeam(ent, traceEnt) || g_friendlyFire.integer != 0)))
		{
			// pain_debounce_time?

			// ZTM: Do a effect when hit anything!
			//   based on G_MissileImpact code
			if ( traceEnt->takedamage && traceEnt->player ) {
				tent = G_TempEntity( tr.endpos, EV_WEAPON_HIT );
				tent->s.otherEntityNum = traceEnt->s.number;
			} else if( tr.surfaceFlags & SURF_METALSTEPS ) {
				tent = G_TempEntity( tr.endpos, EV_WEAPON_MISS_METAL );
			}
			// ZTM: Don't show melee effect when hitting the air...
			else if (tr.fraction != 1.0) {
				tent = G_TempEntity( tr.endpos, EV_WEAPON_MISS );
			}
			else {
				// hit nothing.
				tent = NULL;
			}

			if (tent)
			{
				tent->s.eventParm = DirToByte( tr.plane.normal );
				tent->s.weapon = weaponGroupNum;
				tent->s.weaponHands = hand;
				tent->s.playerNum = ent->s.number;
			}
		}

		if (!traceEnt->takedamage) {
			continue;
		}

		if (ent->player->ps.powerups[PW_QUAD] ) {
			// First time only
			if (!traceHit) {
				G_AddEvent( ent, EV_POWERUP_QUAD, 0 );
			}
		}

		G_Damage( traceEnt, ent, ent, pushDir, tr.endpos, damage, dflags, mod );

		traceHit = qtrue;
	}

	if (checkTeamHit) {
		return qfalse;
	}

	if (traceHit)
	{
		//int mult;
		//int score;

		// Increase score chain
		if (ent->player->ps.chain < 999) {
			ent->player->ps.chain++;
		}
		ent->player->ps.chainTime = MELEE_CHAINTIME;

#if 0
		// Max multipler of 10
		mult = ent->player->ps.chain;
		if (mult > 10) {
			mult = 10;
		}

		score = 1;

		// ZTM: TODO: Give points (based on chain) for melee damage?
		AddScore(ent, tr.endpos, score * mult);
#endif

		if (hand == HAND_PRIMARY)
			ent->player->melee_debounce = level.time + 200;
		else
			ent->player->melee_debounce2 = level.time + 200;
	}
	return traceHit;
}

qboolean G_MeleeDamage(gentity_t *ent, qboolean attacking)
{
	qboolean rtn;
	int i;

	rtn = qfalse;

	for (i = 0; i < MAX_HANDS; i++)
	{
		if (ent->player->ps.weaponHands & HAND_TO_HB(i))
		{
			if (attacking || (bg_weapongroupinfo[ent->player->ps.weapon].weapon[i]->flags & WIF_ALWAYS_DAMAGE))
			{
				if (G_MeleeDamageSingle(ent, qfalse, i, bg_weapongroupinfo[ent->player->ps.weapon].weapon[i]->weapontype))
				{
					rtn = qtrue;
				}
			}
		}
	}

	return rtn;
}
#endif // TA_WEAPSYS
#ifndef TA_WEAPSYS
/*
======================================================================

GAUNTLET

======================================================================
*/

void Weapon_Gauntlet( gentity_t *ent ) {

}

/*
===============
CheckGauntletAttack
===============
*/
qboolean CheckGauntletAttack( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			damage;

	// set aiming directions
	AngleVectors (ent->player->ps.viewangles, forward, right, up);

	CalcMuzzlePoint ( ent, forward, right, up, muzzle );

	VectorMA (muzzle, 32, forward, end);

	trap_Trace (&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);
	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return qfalse;
	}

	if ( ent->player->noclip ) {
		return qfalse;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	// send blood impact
	if ( traceEnt->takedamage && traceEnt->player ) {
		tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
		tent->s.otherEntityNum = traceEnt->s.number;
		tent->s.eventParm = DirToByte( tr.plane.normal );
		tent->s.weapon = ent->s.weapon;
	}

	if ( !traceEnt->takedamage) {
		return qfalse;
	}

	if ( ent->player->ps.powerups[PW_QUAD] ) {
		G_AddEvent( ent, EV_POWERUP_QUAD, 0 );
	}

#ifndef TA_WEAPSYS
	if ( g_instagib.integer ) {
		s_quadFactor = 100;
	} else
#endif
	if ( ent->player->ps.powerups[PW_QUAD] ) {
		s_quadFactor = g_quadfactor.value;
	} else {
		s_quadFactor = 1;
	}
#ifdef MISSIONPACK
	if( ent->player->persistantPowerup && ent->player->persistantPowerup->item && ent->player->persistantPowerup->item->giTag == PW_DOUBLER ) {
		s_quadFactor *= 2;
	}
#endif

	damage = 50 * s_quadFactor;
	G_Damage( traceEnt, ent, ent, forward, tr.endpos,
		damage, 0, MOD_GAUNTLET );

	return qtrue;
}
#endif // TA_WEAPSYS


/*
======================================================================

MACHINEGUN

======================================================================
*/

#ifndef TA_WEAPSYS // ZTM: I replaced all of these, see fire_weapon
#ifdef MISSIONPACK
#define CHAINGUN_SPREAD		600
#define CHAINGUN_DAMAGE		7
#endif
#define MACHINEGUN_SPREAD	200
#define	MACHINEGUN_DAMAGE	7
#define	MACHINEGUN_TEAM_DAMAGE	5		// wimpier MG in teamplay

void Bullet_Fire (gentity_t *ent, float spread, int damage, int mod ) {
	trace_t		tr;
	vec3_t		end;
#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
	vec3_t		impactpoint, bouncedir;
#endif
	float		r;
	float		u;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			i, passent;

	damage *= s_quadFactor;

	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * spread * 16;
	r = cos(r) * crandom() * spread * 16;
	VectorMA (muzzle, 8192*16, forward, end);
	VectorMA (end, r, right, end);
	VectorMA (end, u, up, end);

	passent = ent->s.number;
	for (i = 0; i < 10; i++) {

		// backward-reconcile the other clients
		G_DoTimeShiftFor( ent );

		trap_Trace (&tr, muzzle, NULL, NULL, end, passent, MASK_SHOT);

		// put them back
		G_UndoTimeShiftFor( ent );

		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			return;
		}

		traceEnt = &g_entities[ tr.entityNum ];

		// snap the endpos to integers, but nudged towards the line
		SnapVectorTowards( tr.endpos, muzzle );

		// send bullet impact
		if ( traceEnt->takedamage && traceEnt->player ) {
			tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_FLESH );
			tent->s.eventParm = traceEnt->s.number;
			if( LogAccuracyHit( traceEnt, ent ) ) {
				ent->player->accuracy_hits++;
			}
		} else {
			tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_WALL );
			tent->s.eventParm = DirToByte( tr.plane.normal );
		}
		tent->s.otherEntityNum = ent->s.number;

		if ( traceEnt->takedamage) {
#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
			if ( traceEnt->player && traceEnt->player->invulnerabilityTime > level.time ) {
				if (G_InvulnerabilityEffect( traceEnt, forward, tr.endpos, impactpoint, bouncedir )) {
					G_BounceProjectile( muzzle, impactpoint, bouncedir, end );
					VectorCopy( impactpoint, muzzle );
					// the player can hit him/herself with the bounced rail
					passent = ENTITYNUM_NONE;
				}
				else {
					VectorCopy( tr.endpos, muzzle );
					passent = traceEnt->s.number;
				}
				continue;
			}
			else {
#endif
				G_Damage( traceEnt, ent, ent, forward, tr.endpos,
					damage, 0, mod);
#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
			}
#endif
		}
		break;
	}
}


/*
======================================================================

BFG

======================================================================
*/

void BFG_Fire ( gentity_t *ent ) {
	gentity_t	*m;

	m = fire_bfg (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->player->ps.velocity, m->s.pos.trDelta );	// "real" physics
}


/*
======================================================================

SHOTGUN

======================================================================
*/

// DEFAULT_SHOTGUN_SPREAD and DEFAULT_SHOTGUN_COUNT	are in bg_public.h, because
// client predicts same spreads
#define	DEFAULT_SHOTGUN_DAMAGE	10

qboolean ShotgunPellet( vec3_t start, vec3_t end, gentity_t *ent ) {
	trace_t		tr;
	int			damage, i, passent;
	gentity_t	*traceEnt;
#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
	vec3_t		impactpoint, bouncedir;
#endif
	vec3_t		tr_start, tr_end;
	qboolean	hitPlayer = qfalse;

	passent = ent->s.number;
	VectorCopy( start, tr_start );
	VectorCopy( end, tr_end );
	for (i = 0; i < 10; i++) {
		trap_Trace (&tr, tr_start, NULL, NULL, tr_end, passent, MASK_SHOT);
		traceEnt = &g_entities[ tr.entityNum ];

		// send bullet impact
		if (  tr.surfaceFlags & SURF_NOIMPACT ) {
			return qfalse;
		}

		if ( traceEnt->takedamage) {
			damage = DEFAULT_SHOTGUN_DAMAGE * s_quadFactor;
#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
			if ( traceEnt->player && traceEnt->player->invulnerabilityTime > level.time ) {
				if (G_InvulnerabilityEffect( traceEnt, forward, tr.endpos, impactpoint, bouncedir )) {
					G_BounceProjectile( tr_start, impactpoint, bouncedir, tr_end );
					VectorCopy( impactpoint, tr_start );
					// the player can hit him/herself with the bounced rail
					passent = ENTITYNUM_NONE;
				}
				else {
					VectorCopy( tr.endpos, tr_start );
					passent = traceEnt->s.number;
				}
				continue;
			}
#endif
			if( LogAccuracyHit( traceEnt, ent ) ) {
				hitPlayer = qtrue;
			}
			G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_SHOTGUN);
			return hitPlayer;
		}
		return qfalse;
	}
	return qfalse;
}

// this should match CG_ShotgunPattern
void ShotgunPattern( vec3_t origin, vec3_t origin2, int seed, gentity_t *ent ) {
	int			i;
	float		r, u;
	vec3_t		end;
	vec3_t		forward, right, up;
	qboolean	hitPlayer = qfalse;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2( origin2, forward );
	PerpendicularVector( right, forward );
	CrossProduct( forward, right, up );

	// backward-reconcile the other clients
	G_DoTimeShiftFor( ent );

	// generate the "random" spread pattern
	for ( i = 0 ; i < DEFAULT_SHOTGUN_COUNT ; i++ ) {
		r = Q_crandom( &seed ) * DEFAULT_SHOTGUN_SPREAD * 16;
		u = Q_crandom( &seed ) * DEFAULT_SHOTGUN_SPREAD * 16;
		VectorMA( origin, 8192 * 16, forward, end);
		VectorMA (end, r, right, end);
		VectorMA (end, u, up, end);
		if( ShotgunPellet( origin, end, ent ) && !hitPlayer ) {
			hitPlayer = qtrue;
			ent->player->accuracy_hits++;
		}
	}

	// put them back
	G_UndoTimeShiftFor( ent );
}


void weapon_supershotgun_fire (gentity_t *ent) {
	gentity_t		*tent;

	// send shotgun blast
	tent = G_TempEntity( muzzle, EV_SHOTGUN );
	VectorScale( forward, 4096, tent->s.origin2 );
	SnapVector( tent->s.origin2 );
	tent->s.eventParm = rand() & 255;		// seed for spread pattern
	tent->s.otherEntityNum = ent->s.number;

	ShotgunPattern( tent->s.pos.trBase, tent->s.origin2, tent->s.eventParm, ent );
}


/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void weapon_grenadelauncher_fire (gentity_t *ent) {
	gentity_t	*m;

	// extra vertical velocity
	forward[2] += 0.2f;
	VectorNormalize( forward );

	m = fire_grenade (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->player->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

/*
======================================================================

ROCKET

======================================================================
*/

void Weapon_RocketLauncher_Fire (gentity_t *ent) {
	gentity_t	*m;

	m = fire_rocket (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->player->ps.velocity, m->s.pos.trDelta );	// "real" physics
}


/*
======================================================================

PLASMA GUN

======================================================================
*/

void Weapon_Plasmagun_Fire (gentity_t *ent) {
	gentity_t	*m;

	m = fire_plasma (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->player->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

/*
======================================================================

RAILGUN

======================================================================
*/


/*
=================
weapon_railgun_fire
=================
*/
#define	MAX_RAIL_HITS	4
void weapon_railgun_fire (gentity_t *ent) {
	vec3_t		end;
#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
	vec3_t impactpoint, bouncedir;
#endif
	trace_t		trace;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			damage;
	int			i;
	int			hits;
	int			unlinked;
	int			passent;
	gentity_t	*unlinkedEntities[MAX_RAIL_HITS];

	damage = 100 * s_quadFactor;

	VectorMA (muzzle, 8192, forward, end);

	// backward-reconcile the other clients
	G_DoTimeShiftFor( ent );

	// trace only against the solids, so the railgun will go through people
	unlinked = 0;
	hits = 0;
	passent = ent->s.number;
	do {
		trap_Trace (&trace, muzzle, NULL, NULL, end, passent, MASK_SHOT );
		if ( trace.entityNum >= ENTITYNUM_MAX_NORMAL ) {
			break;
		}
		traceEnt = &g_entities[ trace.entityNum ];
		if ( traceEnt->takedamage ) {
#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
			if ( traceEnt->player && traceEnt->player->invulnerabilityTime > level.time ) {
				if ( G_InvulnerabilityEffect( traceEnt, forward, trace.endpos, impactpoint, bouncedir ) ) {
					G_BounceProjectile( muzzle, impactpoint, bouncedir, end );
					// snap the endpos to integers to save net bandwidth, but nudged towards the line
					SnapVectorTowards( trace.endpos, muzzle );
					// send railgun beam effect
					tent = G_TempEntity( trace.endpos, EV_RAILTRAIL );
					// set player number for custom colors on the railtrail
					tent->s.playerNum = ent->s.playerNum;
					VectorCopy( muzzle, tent->s.origin2 );
					// move origin a bit to come closer to the drawn gun muzzle
					VectorMA( tent->s.origin2, 4, right, tent->s.origin2 );
					VectorMA( tent->s.origin2, -1, up, tent->s.origin2 );
					tent->s.eventParm = 255;	// don't make the explosion at the end
					//
					VectorCopy( impactpoint, muzzle );
					// the player can hit him/herself with the bounced rail
					passent = ENTITYNUM_NONE;
				}
			}
			else {
				if( LogAccuracyHit( traceEnt, ent ) ) {
					hits++;
				}
				G_Damage (traceEnt, ent, ent, forward, trace.endpos, damage, 0, MOD_RAILGUN);
			}
#else
				if( LogAccuracyHit( traceEnt, ent ) ) {
					hits++;
				}
				G_Damage (traceEnt, ent, ent, forward, trace.endpos, damage, 0, MOD_RAILGUN);
#endif
		}
		if ( trace.contents & CONTENTS_SOLID ) {
			break;		// we hit something solid enough to stop the beam
		}
		// unlink this entity, so the next trace will go past it
		trap_UnlinkEntity( traceEnt );
		unlinkedEntities[unlinked] = traceEnt;
		unlinked++;
	} while ( unlinked < MAX_RAIL_HITS );

	// put them back
	G_UndoTimeShiftFor( ent );

	// link back in any entities we unlinked
	for ( i = 0 ; i < unlinked ; i++ ) {
		trap_LinkEntity( unlinkedEntities[i] );
	}

	// the final trace endpos will be the terminal point of the rail trail

	// snap the endpos to integers to save net bandwidth, but nudged towards the line
	SnapVectorTowards( trace.endpos, muzzle );

	// send railgun beam effect
	tent = G_TempEntity( trace.endpos, EV_RAILTRAIL );

	// set player number for custom colors on the railtrail
	tent->s.playerNum = ent->s.playerNum;
#ifdef IOQ3ZTM // ATTACH_RAIL_TO_FLASH
	tent->s.generic1 = 1;
#endif

	VectorCopy( muzzle, tent->s.origin2 );
	// move origin a bit to come closer to the drawn gun muzzle
	VectorMA( tent->s.origin2, 4, right, tent->s.origin2 );
	VectorMA( tent->s.origin2, -1, up, tent->s.origin2 );

	// no explosion at end if SURF_NOIMPACT, but still make the trail
	if ( trace.surfaceFlags & SURF_NOIMPACT ) {
		tent->s.eventParm = 255;	// don't make the explosion at the end
	} else {
		tent->s.eventParm = DirToByte( trace.plane.normal );
	}
	tent->s.playerNum = ent->s.playerNum;

	// give the shooter a reward sound if they have made two railgun hits in a row
	if ( hits == 0 ) {
		// complete miss
		ent->player->accurateCount = 0;
	} else {
		// check for "impressive" reward sound
		ent->player->accurateCount += hits;
		if ( ent->player->accurateCount >= 2 ) {
			ent->player->accurateCount -= 2;
			ent->player->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
			// add the sprite over the player's head
#ifdef IOQ3ZTM
			ent->player->ps.eFlags &= ~EF_AWARD_BITS;
#else
			ent->player->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
#endif
			ent->player->ps.eFlags |= EF_AWARD_IMPRESSIVE;
			ent->player->rewardTime = level.time + REWARD_SPRITE_TIME;
		}
		ent->player->accuracy_hits++;
	}

}
#endif // #ifndef TA_WEAPSYS

/*
======================================================================

GRAPPLING HOOK

======================================================================
*/

#ifndef TA_WEAPSYS
void Weapon_GrapplingHook_Fire (gentity_t *ent)
{
#ifdef IOQ3ZTM
	if (!(ent->player->ps.pm_flags & PMF_FIRE_HELD) && !ent->player->hook)
		fire_grapple (ent, muzzle, forward);

	ent->player->ps.pm_flags |= PMF_FIRE_HELD;
#else
	if (!ent->player->fireHeld && !ent->player->hook)
		fire_grapple (ent, muzzle, forward);

	ent->player->fireHeld = qtrue;
#endif
}
#endif

#ifdef IOQ3ZTM // GRAPPLE_RETURN
void Weapon_ForceHookFree (gentity_t *ent)
{
	ent->parent->player->hook = NULL;
	ent->parent->player->ps.pm_flags &= ~PMF_GRAPPLE_PULL;
	G_FreeEntity( ent );
}

void G_SetMissileVelocity(gentity_t *bolt, vec3_t dir, int projnum);

#define HOOK_RETURN_THINK_TIME 100
// Grappling hook returning to parent
void G_GrapplingHookReturnThink (gentity_t *ent)
{
	vec3_t dir;
	vec3_t target;

	if (!ent->parent) {
		Weapon_ForceHookFree(ent);
		return;
	}

	if (ent->parent->player) {
		vec3_t muzzle;
		vec3_t forward, right, up;

		AngleVectors (ent->parent->player->ps.viewangles, forward, right, up);
		CalcMuzzlePoint ( ent->parent, forward, right, up, muzzle );
		// Offset using handSide in ent->s.weaponHands
		if (ent->s.weaponHands == HS_RIGHT)
			VectorMA (muzzle, 4, right, muzzle);
		else if (ent->s.weaponHands == HS_LEFT)
			VectorMA (muzzle, -4, right, muzzle);
		VectorCopy(muzzle, target);
	} else {
		VectorCopy(ent->parent->r.currentOrigin, target);
	}

	// get dir to ent->parent
	VectorSubtract(target, ent->r.currentOrigin, dir);

	if (VectorLength(dir) < 64)
	{
		Weapon_ForceHookFree(ent);
		return;
	}

	if (ent->parent->player) {
		// Pervent player from firing, it doesn't effect game's grappling hook but does cgame does fire effects.
		ent->parent->player->ps.weaponTime = HOOK_RETURN_THINK_TIME + 50;
	}

	// for exact trajectory calculation, set current point to base.
	VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);

	VectorNormalize(dir);
#if 0
	// 0.5 is swing rate.
	//VectorScale(dir, 0.5, dir);
	VectorAdd(dir, ent->r.currentAngles, dir);

	// turn nozzle to target angle
	VectorNormalize(dir);
	VectorCopy(dir, ent->r.currentAngles);
#endif

	ent->s.pos.trTime = level.time;
	VectorScale(dir, 1.5f, dir); // go back to player faster
	G_SetMissileVelocity(ent, dir, ent->s.weapon);

	ent->nextthink = level.time + HOOK_RETURN_THINK_TIME;	// decrease this value also makes fast swing
	ent->think = G_GrapplingHookReturnThink;
}
#endif

void Weapon_HookFree (gentity_t *ent)
{
#ifdef IOQ3ZTM // GRAPPLE_RETURN
	if (ent->parent->player->ps.pm_type != PM_DEAD
#ifdef TA_PLAYERSYS // LADDER
		&& !(ent->parent->player->ps.eFlags & EF_LADDER)
#endif
		)
	{
		// Pull grapple back to player before removing entity, like LoZ: TP
		ent->parent->player->ps.pm_flags &= ~PMF_GRAPPLE_PULL;
		ent->s.eType = ET_MISSILE; // If ET_GRAPPLE change to missile

#ifdef TA_WEAPSYS
		// Don't damage parent
		ent->count &= ~1;
		ent->flags |= FL_MISSILE_NO_DAMAGE_PARENT;
#endif

		ent->s.eFlags |= EF_TRAINBACKWARD;
		G_GrapplingHookReturnThink(ent);
		return;
	}
#endif
#ifdef IOQ3ZTM // GRAPPLE_RETURN
	Weapon_ForceHookFree ( ent );
#else
	ent->parent->player->hook = NULL;
	ent->parent->player->ps.pm_flags &= ~PMF_GRAPPLE_PULL;
	G_FreeEntity( ent );
#endif
}

void Weapon_HookThink (gentity_t *ent)
{
	ent->nextthink = level.time + FRAMETIME;

	if (ent->enemy && ent->enemy->player) {
		vec3_t v, oldorigin;

		if ( ent->enemy->player->ps.pm_type == PM_DEAD ) {
			Weapon_HookFree( ent );
			return;
		}

		VectorCopy(ent->r.currentOrigin, oldorigin);
		v[0] = ent->enemy->r.currentOrigin[0] + (ent->enemy->s.mins[0] + ent->enemy->s.maxs[0]) * 0.5;
		v[1] = ent->enemy->r.currentOrigin[1] + (ent->enemy->s.mins[1] + ent->enemy->s.maxs[1]) * 0.5;
		v[2] = ent->enemy->r.currentOrigin[2] + (ent->enemy->s.mins[2] + ent->enemy->s.maxs[2]) * 0.5;
		SnapVectorTowards( v, oldorigin );	// save net bandwidth

		G_SetOrigin( ent, v );
	}

	VectorCopy( ent->r.currentOrigin, ent->parent->player->ps.grapplePoint);
}

#ifndef TA_WEAPSYS
/*
======================================================================

LIGHTNING GUN

======================================================================
*/

void Weapon_LightningFire( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end;
#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
	vec3_t impactpoint, bouncedir;
#endif
	gentity_t	*traceEnt, *tent;
	int			damage, i, passent;

	damage = 8 * s_quadFactor;

	passent = ent->s.number;
	for (i = 0; i < 10; i++) {
		VectorMA( muzzle, LIGHTNING_RANGE, forward, end );

		// backward-reconcile the other clients
		G_DoTimeShiftFor( ent );

		trap_Trace( &tr, muzzle, NULL, NULL, end, passent, MASK_SHOT );

		// put them back
		G_UndoTimeShiftFor( ent );

#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
		// if not the first trace (the lightning bounced of an invulnerability sphere)
		if (i) {
			// add bounced off lightning bolt temp entity
			// the first lightning bolt is a cgame only visual
			//
			tent = G_TempEntity( muzzle, EV_LIGHTNINGBOLT );
			VectorCopy( tr.endpos, end );
			SnapVector( end );
			VectorCopy( end, tent->s.origin2 );
		}
#endif
		if ( tr.entityNum == ENTITYNUM_NONE ) {
			return;
		}

		traceEnt = &g_entities[ tr.entityNum ];

		if ( traceEnt->takedamage) {
#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
			if ( traceEnt->player && traceEnt->player->invulnerabilityTime > level.time ) {
				if (G_InvulnerabilityEffect( traceEnt, forward, tr.endpos, impactpoint, bouncedir )) {
					G_BounceProjectile( muzzle, impactpoint, bouncedir, end );
					VectorCopy( impactpoint, muzzle );
					VectorSubtract( end, impactpoint, forward );
					VectorNormalize(forward);
					// the player can hit him/herself with the bounced lightning
					passent = ENTITYNUM_NONE;
				}
				else {
					VectorCopy( tr.endpos, muzzle );
					passent = traceEnt->s.number;
				}
				continue;
			}
#endif
			if( LogAccuracyHit( traceEnt, ent ) ) {
				ent->player->accuracy_hits++;
			}
			G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_LIGHTNING);
		}

		if ( traceEnt->takedamage && traceEnt->player ) {
			tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
			tent->s.otherEntityNum = traceEnt->s.number;
			tent->s.eventParm = DirToByte( tr.plane.normal );
			tent->s.weapon = ent->s.weapon;
		} else if ( !( tr.surfaceFlags & SURF_NOIMPACT ) ) {
			tent = G_TempEntity( tr.endpos, EV_MISSILE_MISS );
			tent->s.eventParm = DirToByte( tr.plane.normal );
		}

		break;
	}
}

#ifdef MISSIONPACK
/*
======================================================================

NAILGUN

======================================================================
*/

void Weapon_Nailgun_Fire (gentity_t *ent) {
	gentity_t	*m;
	int			count;

	for( count = 0; count < NUM_NAILSHOTS; count++ ) {
		m = fire_nail (ent, muzzle, forward, right, up );
		m->damage *= s_quadFactor;
		m->splashDamage *= s_quadFactor;
	}

//	VectorAdd( m->s.pos.trDelta, ent->player->ps.velocity, m->s.pos.trDelta );	// "real" physics
}


/*
======================================================================

PROXIMITY MINE LAUNCHER

======================================================================
*/

void weapon_proxlauncher_fire (gentity_t *ent) {
	gentity_t	*m;

	// extra vertical velocity
	forward[2] += 0.2f;
	VectorNormalize( forward );

	m = fire_prox (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->player->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

#endif
#endif // #ifndef TA_WEAPSYS

//======================================================================


/*
===============
LogAccuracyHit
===============
*/
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker ) {
	if( !target->takedamage ) {
		return qfalse;
	}

	if ( target == attacker ) {
		return qfalse;
	}

	if( !target->player ) {
		return qfalse;
	}

	if( !attacker->player ) {
		return qfalse;
	}

	if( target->player->ps.stats[STAT_HEALTH] <= 0 ) {
		return qfalse;
	}

	if ( OnSameTeam( target, attacker ) ) {
		return qfalse;
	}

	return qtrue;
}


/*
===============
CalcMuzzlePoint

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePoint ( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {
	VectorCopy( ent->s.pos.trBase, muzzlePoint );
	muzzlePoint[2] += ent->player->ps.viewheight;
	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector( muzzlePoint );
}

/*
===============
CalcMuzzlePointOrigin

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePointOrigin ( gentity_t *ent, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {
	VectorCopy( ent->s.pos.trBase, muzzlePoint );
	muzzlePoint[2] += ent->player->ps.viewheight;
	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector( muzzlePoint );
}



/*
===============
FireWeapon
===============
*/
void FireWeapon( gentity_t *ent ) {
#ifdef TA_WEAPSYS
	int i;
#endif

#ifndef TA_WEAPSYS
	if ( g_instagib.integer ) {
		s_quadFactor = 100;
	} else
#endif
	if ( ent->player->ps.powerups[PW_QUAD] ) {
		s_quadFactor = g_quadfactor.value;
	} else {
		s_quadFactor = 1;
	}
#ifdef MISSIONPACK
	if( ent->player->persistantPowerup && ent->player->persistantPowerup->item && ent->player->persistantPowerup->item->giTag == PW_DOUBLER ) {
		s_quadFactor *= 2;
	}
#endif

#ifndef TA_WEAPSYS
	// track shots taken for accuracy tracking.  Grapple is not a weapon and gauntet is just not tracked
	if( ent->s.weapon != WP_GRAPPLING_HOOK && ent->s.weapon != WP_GAUNTLET )
	{
#ifdef MISSIONPACK
		if( ent->s.weapon == WP_NAILGUN ) {
			ent->player->accuracy_shots += NUM_NAILSHOTS;
		} else {
			ent->player->accuracy_shots++;
		}
#else
		ent->player->accuracy_shots++;
#endif
	}
#endif

#ifdef TA_WEAPSYS
	for (i = 0; i < MAX_HANDS; i++)
	{
		if (!(ent->player->ps.weaponHands & HAND_TO_HB(i))) {
			continue;
		}

		if (bg_weapongroupinfo[ent->s.weapon].weapon[i]->weapontype == WT_GUN)
		{
			// track shots taken for accuracy tracking.
			if (!bg_weapongroupinfo[ent->s.weapon].weapon[i]->proj->grappling) {
				ent->player->accuracy_shots += bg_weapongroupinfo[ent->s.weapon].weapon[i]->proj->numProjectiles;
			}

			// set aiming directions
			AngleVectors (ent->player->ps.viewangles, forward, right, up);
			CalcMuzzlePointOrigin ( ent, ent->player->oldOrigin, forward, right, up, muzzle );
			if (ent->player->pers.playercfg.handSide[i] == HS_RIGHT)
				VectorMA (muzzle, 4, right, muzzle);
			else if (ent->player->pers.playercfg.handSide[i] == HS_LEFT)
				VectorMA (muzzle, -4, right, muzzle);

#ifdef TURTLEARENA // LOCKON
			G_AutoAim(ent, bg_weapongroupinfo[ent->s.weapon].weapon[i]->projnum,
					muzzle, forward, right, up);
#endif
			fire_weapon(ent, muzzle, forward, right, up,
					bg_weapongroupinfo[ent->s.weapon].weaponnum[i], s_quadFactor, ent->player->pers.playercfg.handSide[i]);
		}

	}
#else
	// set aiming directions
	AngleVectors (ent->player->ps.viewangles, forward, right, up);

	CalcMuzzlePointOrigin ( ent, ent->player->oldOrigin, forward, right, up, muzzle );

#ifdef TURTLEARENA // LOCKON
	G_AutoAim(ent, 0, muzzle, forward, right, up);
#endif
	// fire the specific weapon
	switch( ent->s.weapon ) {
	case WP_GAUNTLET:
		Weapon_Gauntlet( ent );
		break;
	case WP_LIGHTNING:
		Weapon_LightningFire( ent );
		break;
	case WP_SHOTGUN:
		weapon_supershotgun_fire( ent );
		break;
	case WP_MACHINEGUN:
		if ( g_gametype.integer != GT_TEAM ) {
			Bullet_Fire( ent, MACHINEGUN_SPREAD, MACHINEGUN_DAMAGE, MOD_MACHINEGUN );
		} else {
			Bullet_Fire( ent, MACHINEGUN_SPREAD, MACHINEGUN_TEAM_DAMAGE, MOD_MACHINEGUN );
		}
		break;
	case WP_GRENADE_LAUNCHER:
		weapon_grenadelauncher_fire( ent );
		break;
	case WP_ROCKET_LAUNCHER:
		Weapon_RocketLauncher_Fire( ent );
		break;
	case WP_PLASMAGUN:
		Weapon_Plasmagun_Fire( ent );
		break;
	case WP_RAILGUN:
		weapon_railgun_fire( ent );
		break;
	case WP_BFG:
		BFG_Fire( ent );
		break;
	case WP_GRAPPLING_HOOK:
		Weapon_GrapplingHook_Fire( ent );
		break;
#ifdef MISSIONPACK
	case WP_NAILGUN:
		Weapon_Nailgun_Fire( ent );
		break;
	case WP_PROX_LAUNCHER:
		weapon_proxlauncher_fire( ent );
		break;
	case WP_CHAINGUN:
		Bullet_Fire( ent, CHAINGUN_SPREAD, CHAINGUN_DAMAGE, MOD_CHAINGUN );
		break;
#endif
	default:
// FIXME		G_Error( "Bad ent->s.weapon" );
		break;
	}
#endif
}


#if defined MISSIONPACK && !defined TURTLEARENA // NO_KAMIKAZE_ITEM

/*
===============
KamikazeRadiusDamage
===============
*/
static void KamikazeRadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float radius ) {
	float		dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;

	if ( radius < 1 ) {
		radius = 1;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		if (!ent->takedamage) {
			continue;
		}

		// don't hit things we have already hit
		if( ent->kamikazeTime > level.time ) {
			continue;
		}

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}

		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

//		if( CanDamage (ent, origin) ) {
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;
			G_Damage( ent, NULL, attacker, dir, origin, damage, DAMAGE_RADIUS|DAMAGE_NO_TEAM_PROTECTION, MOD_KAMIKAZE );
			ent->kamikazeTime = level.time + 3000;
//		}
	}
}

/*
===============
KamikazeShockWave
===============
*/
static void KamikazeShockWave( vec3_t origin, gentity_t *attacker, float damage, float push, float radius ) {
	float		dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;

	if ( radius < 1 )
		radius = 1;

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		// don't hit things we have already hit
		if( ent->kamikazeShockTime > level.time ) {
			continue;
		}

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}

		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

//		if( CanDamage (ent, origin) ) {
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			dir[2] += 24;
			G_Damage( ent, NULL, attacker, dir, origin, damage, DAMAGE_RADIUS|DAMAGE_NO_TEAM_PROTECTION, MOD_KAMIKAZE );
			//
			dir[2] = 0;
			VectorNormalize(dir);
			if ( ent->player ) {
				ent->player->ps.velocity[0] = dir[0] * push;
				ent->player->ps.velocity[1] = dir[1] * push;
				ent->player->ps.velocity[2] = 100;
			}
			ent->kamikazeShockTime = level.time + 3000;
//		}
	}
}

/*
===============
KamikazeDamage
===============
*/
static void KamikazeDamage( gentity_t *self ) {
	int i;
	float t;
	gentity_t *ent;
	vec3_t newangles;

	self->count += 100;

	if (self->count >= KAMI_SHOCKWAVE_STARTTIME) {
		// shockwave push back
		t = self->count - KAMI_SHOCKWAVE_STARTTIME;
		KamikazeShockWave(self->s.pos.trBase, self->activator, 25, 400,	(int) (float) t * KAMI_SHOCKWAVE_MAXRADIUS / (KAMI_SHOCKWAVE_ENDTIME - KAMI_SHOCKWAVE_STARTTIME) );
	}
	//
	if (self->count >= KAMI_EXPLODE_STARTTIME) {
		// do our damage
		t = self->count - KAMI_EXPLODE_STARTTIME;
		KamikazeRadiusDamage( self->s.pos.trBase, self->activator, 400,	(int) (float) t * KAMI_BOOMSPHERE_MAXRADIUS / (KAMI_IMPLODE_STARTTIME - KAMI_EXPLODE_STARTTIME) );
	}

	// either cycle or kill self
	if( self->count >= KAMI_SHOCKWAVE_ENDTIME ) {
		G_FreeEntity( self );
		return;
	}
	self->nextthink = level.time + 100;

	// add earth quake effect
	newangles[0] = crandom() * 2;
	newangles[1] = crandom() * 2;
	newangles[2] = 0;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		ent = &g_entities[i];
		if (!ent->inuse)
			continue;
		if (!ent->player)
			continue;

		if (ent->player->ps.groundEntityNum != ENTITYNUM_NONE) {
			ent->player->ps.velocity[0] += crandom() * 120;
			ent->player->ps.velocity[1] += crandom() * 120;
			ent->player->ps.velocity[2] = 30 + random() * 25;
		}

		ent->player->ps.delta_angles[0] += ANGLE2SHORT(newangles[0] - self->movedir[0]);
		ent->player->ps.delta_angles[1] += ANGLE2SHORT(newangles[1] - self->movedir[1]);
		ent->player->ps.delta_angles[2] += ANGLE2SHORT(newangles[2] - self->movedir[2]);
	}
	VectorCopy(newangles, self->movedir);
}

/*
===============
G_StartKamikaze
===============
*/
void G_StartKamikaze( gentity_t *ent ) {
	gentity_t	*explosion;
	gentity_t	*te;
	vec3_t		snapped;

	// start up the explosion logic
	explosion = G_Spawn();

	explosion->s.eType = ET_EVENTS + EV_KAMIKAZE;
	explosion->eventTime = level.time;

	if ( ent->player ) {
		VectorCopy( ent->s.pos.trBase, snapped );
	}
	else {
		VectorCopy( ent->activator->s.pos.trBase, snapped );
	}
	SnapVector( snapped );		// save network bandwidth
	G_SetOrigin( explosion, snapped );

	explosion->classname = "kamikaze";
	explosion->s.pos.trType = TR_STATIONARY;

	explosion->kamikazeTime = level.time;

	explosion->think = KamikazeDamage;
	explosion->nextthink = level.time + 100;
	explosion->count = 0;
	VectorClear(explosion->movedir);

	trap_LinkEntity( explosion );

	if (ent->player) {
		//
		explosion->activator = ent;
		//
		ent->s.eFlags &= ~EF_KAMIKAZE;
		ent->player->ps.eFlags &= ~EF_KAMIKAZE;
		// nuke the guy that used it
		G_Damage( ent, ent, ent, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_KAMIKAZE );
	}
	else {
		if ( !strcmp(ent->activator->classname, "bodyque") ) {
			explosion->activator = &g_entities[ent->activator->r.ownerNum];
		}
		else {
			explosion->activator = ent->activator;
		}
	}

	// play global sound at all clients
	te = G_TempEntity(snapped, EV_GLOBAL_TEAM_SOUND );
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = GTS_KAMIKAZE;
}
#endif
