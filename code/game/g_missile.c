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

#define	MISSILE_PRESTEP_TIME	50

/*
================
G_BounceMissile

================
*/
void G_BounceMissile( gentity_t *ent, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );

	if ( ent->s.eFlags & EF_BOUNCE_HALF ) {
		VectorScale( ent->s.pos.trDelta, 0.65, ent->s.pos.trDelta );
		// check for stop
		if ( trace->plane.normal[2] > 0.2 && VectorLength( ent->s.pos.trDelta ) < 40 ) {
			G_SetOrigin( ent, trace->endpos );
			ent->s.time = level.time / 4;
			return;
		}
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}


/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile( gentity_t *ent ) {
	vec3_t		dir;
	vec3_t		origin;

#ifdef TA_WEAPSYS
	if (bg_projectileinfo[ent->s.weapon].grappling)
	{
		Weapon_HookFree(ent);
		return;
	}

	if (bg_projectileinfo[ent->s.weapon].explosionType == PE_NONE)
	{
		G_FreeEntity(ent);
		return;
	}
#endif

	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	SnapVector( origin );
	G_SetOrigin( ent, origin );

#ifdef TA_WEAPSYS
	// Missile impacted a surface
	if (ent->count & 2)
	{
		VectorCopy(ent->s.angles2, dir);
	}
	else
	{
#endif
	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;
#ifdef TA_WEAPSYS
	}
#endif

#ifndef TA_WEAPSYS // Must be after G_RadiusDamage
	ent->s.eType = ET_GENERAL;
#endif
	G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );
#ifdef TA_WEAPSYS
	if (ent->parent)
		ent->s.clientNum = ent->parent->s.number;
	else
		ent->s.clientNum = ENTITYNUM_NONE;
#endif

	ent->freeAfterEvent = qtrue;

	// splash damage
	if ( ent->splashDamage ) {
#ifdef TA_WEAPSYS
		if( G_RadiusDamage( ent->r.currentOrigin, ent, ent->parent, ent->splashDamage, ent->splashRadius, ent
			, ent->splashMethodOfDeath ) )
#else
		if( G_RadiusDamage( ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent
			, ent->splashMethodOfDeath ) )
#endif
		{
			g_entities[ent->r.ownerNum].client->accuracy_hits++;
		}
	}

#ifdef TA_WEAPSYS
	ent->s.eType = ET_GENERAL;
#endif

	trap_LinkEntity( ent );
}

#ifdef TA_WEAPSYS
/*
================
G_Missile_Die

From XREAL r3036
================
*/
static void G_Missile_Die(gentity_t * ent, gentity_t * inflictor, gentity_t * attacker, int damage, int mod)
{
	ent->nextthink = level.time + 1;
	ent->think = G_ExplodeMissile;
}

// ZTM: XREAL allows for missile to use accel, but they added TR_ACCELERATION
//              so it makes sense...
void G_SetMissileVelocity(gentity_t *bolt, vec3_t dir, int projnum)
{
	float velocity;

	velocity = bg_projectileinfo[projnum].speed;
	velocity += random() * bg_projectileinfo[projnum].spdRndAdd;

#if 0 // ZTM: Ug, Q3 doesn't have TR_ACCELERATION ...
	//XREAL: if (g_rocketAcceleration.integer)
	if (acceleration)
	{
		// use acceleration instead of linear velocity
		bolt->s.pos.trType = TR_ACCELERATION;
		bolt->s.pos.trAcceleration = acceleration;
		VectorScale(dir, velocity * 4, bolt->s.pos.trDelta);
		// XREAL
		//bolt->s.pos.trAcceleration = g_rocketAcceleration.value;
		//VectorScale(dir, g_rocketVelocity.value, bolt->s.pos.trDelta);
	}
	else
#endif
	{
		// scale direction, put into trDelta
		bolt->s.pos.trType = TR_LINEAR;
		VectorScale(dir, velocity, bolt->s.pos.trDelta);
		// XREAL
		//VectorScale(dir, g_rocketVelocity.value * 0.25, bolt->s.pos.trDelta);
	}

#if 0
	if (bolt->parent && bolt->parent->client) {
		// "real" physics
		VectorAdd( bolt->s.pos.trDelta, bolt->parent->client->ps.velocity, bolt->s.pos.trDelta );
	}
#endif

	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
}

/*
================
G_HomingMissile

From XREAL r3036
================
*/
void G_HomingMissile(gentity_t * ent)
{
	gentity_t      *target = NULL;
	gentity_t      *blip = NULL;
	vec3_t          dir, blipdir;
	vec_t           angle;
	const int		HOMING_THINK_TIME = 60;

#ifdef TA_WEAPSYS // XREAL: spawnTime
	// explode after 15 seconds without a hit
	if (bg_projectileinfo[ent->s.weapon].timetolive != -1
		&& ent->spawnTime + bg_projectileinfo[ent->s.weapon].timetolive <= level.time)
	{
		G_ExplodeMissile(ent);
		return;
	}
#endif

	/*
	if(ent->parent->health <= 0)
	{
		ent->nextthink = level.time + 15000;
		ent->think = G_ExplodeMissile;
		return;
	}
	*/

	while((blip = G_FindRadius(blip, ent->r.currentOrigin, 2000)) != NULL)
	{
		if(blip->client == NULL)
			continue;

		if(blip == ent->parent)
			continue;

		if(blip->health <= 0)
			continue;

		if(blip->flags & FL_NOTARGET)
			continue;

		if(blip->client->sess.sessionTeam >= TEAM_SPECTATOR)
			continue;

		if(OnSameTeam(blip, ent->parent))
			continue;

		if(!G_IsVisible(ent->s.number, ent->r.currentOrigin, blip->r.currentOrigin))
			continue;

		VectorSubtract(blip->r.currentOrigin, ent->r.currentOrigin, blipdir);

		if((target == NULL) || (VectorLength(blipdir) < VectorLength(dir)))
		{
			angle = AngleBetweenVectors(ent->r.currentAngles, blipdir);

			if(angle < 120.0f)
			{
				// We add it as our target
				target = blip;
				VectorCopy(blipdir, dir);
			}
		}
	}

	if (target == NULL)
	{
		ent->nextthink = level.time + HOMING_THINK_TIME;	// + 10000;
		ent->think = G_HomingMissile;
	}
	else
	{
		// for exact trajectory calculation, set current point to base.
		VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);

		VectorNormalize(dir);
		// 0.5 is swing rate.
		VectorScale(dir, 0.5, dir);
		VectorAdd(dir, ent->r.currentAngles, dir);

		// turn nozzle to target angle
		VectorNormalize(dir);
		VectorCopy(dir, ent->r.currentAngles);

		ent->s.pos.trTime = level.time;
		G_SetMissileVelocity(ent, dir, ent->s.weapon);

		ent->nextthink = level.time + HOMING_THINK_TIME;	// decrease this value also makes fast swing
		ent->think = G_HomingMissile;

		//G_Printf("targeting %s\n", target->classname);
	}
}

/*
================
G_MissileFall

Drop missile to ground
================
*/
void G_MissileFall( gentity_t *ent ) {
	// Fall!
	ent->s.pos.trType = TR_GRAVITY;
}

/*
================
G_MissileFall_Die
================
*/
static void G_MissileFall_Die(gentity_t * ent, gentity_t * inflictor, gentity_t * attacker, int damage, int mod)
{
	ent->nextthink = level.time + 1;
	ent->think = G_MissileFall;
}

#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
void G_BounceProjectile( vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout );
#endif

// self is the "parent", the entity that owns the missile (like a player or shooter_*)
qboolean fire_projectile(gentity_t *self, vec3_t start, vec3_t forward,
		vec3_t right, vec3_t up, int projnum, float quadFactor,
		int mod, int splashMod, int handSide)
{
	vec3_t		mins = { -8, -8, -8 };
	vec3_t		maxs = {  8,  8,  8 };
	gentity_t	*bolt;
	vec3_t		dir;
	vec3_t		end;
	int count;
	int spread;
	int damage;
	int splashDamage;
	float splashRadius;
	int range;
	qboolean hitClient;

	if (projnum < 0 || projnum >= BG_NumProjectiles())
	{
		return qfalse;
	}

	// Check if can fire grappling projectile
	if (self && self->client && bg_projectileinfo[projnum].grappling)
	{
#ifdef IOQ3ZTM
#ifdef TURTLEARENA // HOLD_SHURIKEN
		if (handSide == HS_CENTER) // Shuriken holdable item
		{
			if ((self->client->ps.pm_flags & PMF_USE_ITEM_HELD) || self->client->hook) {
				return qfalse;
			}
			self->client->ps.pm_flags |= PMF_USE_ITEM_HELD;
		}
		else
#endif
		{
			if ((self->client->ps.pm_flags & PMF_FIRE_HELD) || self->client->hook) {
				return qfalse;
			}
			self->client->ps.pm_flags |= PMF_FIRE_HELD;
		}
#else
		if (self->client->fireHeld || self->client->hook) {
			return qfalse;
		}
		self->client->fireHeld = qtrue;
#endif
	}

	spread = bg_projectileinfo[projnum].spread;
#if 0 // ZTM: WONTFIX: Only for machinegun
	if (g_gametype.integer == GT_TEAM)
	{
		damage = bg_projectileinfo[projnum].damage * 0.7;
		splashDamage = bg_projectileinfo[projnum].splashDamage * 0.7;
	}
	else
#endif
	{
		damage = bg_projectileinfo[projnum].damage;
		splashDamage = bg_projectileinfo[projnum].splashDamage;
	}
	splashRadius = bg_projectileinfo[projnum].splashRadius;

	if (quadFactor > 1) {
		damage *= quadFactor;
		splashDamage *= quadFactor;
	}

	// Use default kill messages
	// Missile is spawned
	if (mod == MOD_UNKNOWN) {
		mod = MOD_PROJECTILE;
	}
	if (splashMod == MOD_UNKNOWN) {
		splashMod = MOD_PROJECTILE_EXPLOSION;
	}

	range = 0;
	if (bg_projectileinfo[projnum].instantDamage)
	{
		range = bg_projectileinfo[projnum].speed;
	}
	if (!range) {
		// 8192 is used by bullets, railgun, and nails
		// Lightning uses 768
		range = 8192;
	}

	for (count = 0; count < bg_projectileinfo[projnum].numProjectiles; count++ )
	{
		if (spread) // if spread make your own dir.
		{
			float		r, u;

			r = random() * M_PI * 2.0f;
			u = sin(r) * crandom() * spread * 16;
			r = cos(r) * crandom() * spread * 16;
			VectorMA(start, (range * 16), forward, end);
			VectorMA(end, r, right, end);
			VectorMA(end, u, up, end);
			VectorSubtract(end, start, dir);
		}
		else
		{
			VectorMA(start, range, forward, end);
			VectorCopy(forward, dir);
		}

		if (bg_projectileinfo[projnum].flags & PF_USE_GRAVITY)
		{
			// extra vertical velocity
			dir[2] += 0.2f;
		}

		VectorNormalize (dir);

		if (bg_projectileinfo[projnum].instantDamage)
		{
			// Based on Q3's Bullet_Fire
			//    (with extra code from Weapon_LightningFire and weapon_railgun_fire)
			trace_t		tr;
#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
			vec3_t		impactpoint, bouncedir;
#endif
			gentity_t	*tent;
			gentity_t	*traceEnt;
			int			i, passent;
			int			unlinked;
			gentity_t	*unlinkedEntities[10];
			int			hits;

			unlinked = hits = 0;

			// Do a bullet trace instead of spawning a missile.
			passent = self->s.number;
			for (i = 0; i < 10; i++) // 10 for lightning
			{
				trap_Trace (&tr, start, NULL, NULL, end, passent, MASK_SHOT);

#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
				// if not the first trace (the lightning bounced off an invulnerability sphere)
				if (i && bg_projectileinfo[projnum].trailType == PT_LIGHTNING) {
					// add bounced off lightning bolt temp entity
					// the first lightning bolt is a cgame only visual
					//
					tent = G_TempEntity( start, EV_LIGHTNINGBOLT );
					tent->s.weapon = projnum;
					VectorCopy( tr.endpos, end );
					SnapVector( end );
					VectorCopy( end, tent->s.origin2 );
				}
#endif
#if 0 // RAIL
				if ( tr.entityNum >= ENTITYNUM_MAX_NORMAL ) {
					break;
				}
#else // LIGHTNING
				if ( tr.entityNum == ENTITYNUM_NONE ) {
					break;
				}
#endif

				traceEnt = &g_entities[ tr.entityNum ];

				// snap the endpos to integers, but nudged towards the line
				SnapVectorTowards( tr.endpos, start );

				if ( !(tr.surfaceFlags & SURF_NOIMPACT) )
				{
					// send bullet impact
					if ( traceEnt->takedamage && traceEnt->client )
					{
#if 1 // lightning
						tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
						tent->s.otherEntityNum = traceEnt->s.number;
						tent->s.eventParm = DirToByte( tr.plane.normal );
						tent->s.clientNum = self->s.number;
#else
						tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_FLESH );
						tent->s.eventParm = traceEnt->s.number;
						tent->s.otherEntityNum = self->s.number;
#endif
						tent->s.weapon = projnum;
					} else if (tr.surfaceFlags & SURF_METALSTEPS) {
						tent = G_TempEntity( tr.endpos, EV_MISSILE_MISS_METAL );
						tent->s.clientNum = self->s.number;
						tent->s.weapon = projnum;
						tent->s.eventParm = DirToByte( tr.plane.normal );
					} else {
#if 1 // lightning
						tent = G_TempEntity( tr.endpos, EV_MISSILE_MISS );
						tent->s.clientNum = self->s.number;
#else
						tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_WALL );
						tent->s.otherEntityNum = self->s.number;
#endif
						tent->s.weapon = projnum;
						tent->s.eventParm = DirToByte( tr.plane.normal );
					}
				}

				if ( traceEnt->takedamage)
				{
#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
					if ( traceEnt->client && traceEnt->client->invulnerabilityTime > level.time )
					{
						if (G_InvulnerabilityEffect( traceEnt, dir, tr.endpos, impactpoint, bouncedir ))
						{
							G_BounceProjectile( start, impactpoint, bouncedir, end );
							if (bg_projectileinfo[projnum].trailType == PT_RAIL)
							{
								// snap the endpos to integers to save net bandwidth, but nudged towards the line
								SnapVectorTowards( tr.endpos, start );
								// send railgun beam effect
								tent = G_TempEntity( tr.endpos, EV_RAILTRAIL );
								// set player number for custom colors on the railtrail
								tent->s.clientNum = self->s.clientNum;
								tent->s.weapon = projnum;
								tent->s.weaponHands = MAX_HANDS; // Don't attach to client's gun
								VectorCopy( start, tent->s.origin2 );
								// move origin a bit to come closer to the drawn gun muzzle
								VectorMA( tent->s.origin2, 4, right, tent->s.origin2 );
								VectorMA( tent->s.origin2, -1, up, tent->s.origin2 );
								tent->s.eventParm = 255;	// don't make the explosion at the end
							}
							VectorCopy( impactpoint, start );
#if 1 // lightning
							VectorSubtract( end, impactpoint, dir );
							VectorNormalize(dir);
#endif
							// the player can hit him/herself with the bounced rail
							passent = ENTITYNUM_NONE;
						}
						else
						{
							VectorCopy( tr.endpos, start );
							passent = traceEnt->s.number;
						}
						if (bg_projectileinfo[projnum].trailType != PT_RAIL) {
							continue;
						}
					}
					else
#endif
					{
						G_Damage( traceEnt, self, self,
#if 1 // ZTM: Knockback in direction projectile was moving
							dir,
#else
							forward,
#endif
							tr.endpos,
							damage,
							0, mod);

						hitClient = LogAccuracyHit( traceEnt, self );

						// Splash damage!
						if (G_RadiusDamage(tr.endpos, self, self, damage, splashRadius, traceEnt, splashMod)) {
							hitClient = qtrue;
						}

						if( hitClient ) {
							hits++;
						}
					}
				}

				// weapon_railgun_fire
				if (bg_projectileinfo[projnum].maxHits > 1) {
					if ( tr.contents & CONTENTS_SOLID ) {
						break;		// we hit something solid enough to stop the beam
					}

					// unlink this entity, so the next trace will go past it
					trap_UnlinkEntity( traceEnt );
					unlinkedEntities[unlinked] = traceEnt;
					unlinked++;

					if (i+1 >= bg_projectileinfo[projnum].maxHits) {
						break;
					}
				} else {
					break;
				}
			}

			// link back in any entities we unlinked
			for ( i = 0 ; i < unlinked ; i++ ) {
				trap_LinkEntity( unlinkedEntities[i] );
			}

			if (self && self->client) {
#ifndef TURTLEARENA // AWARDS
				if (bg_projectileinfo[projnum].maxHits > 1) {
					// give the shooter a reward sound if they have made two railgun hits in a row
					if ( hits == 0 ) {
						// complete miss
						self->client->accurateCount = 0;
					} else {
						// check for "impressive" reward sound
						self->client->accurateCount += hits;
						if ( self->client->accurateCount >= 2 ) {
							self->client->accurateCount -= 2;
							self->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
							// add the sprite over the player's head
#ifdef IOQ3ZTM
							self->client->ps.eFlags &= ~EF_AWARD_BITS;
#else
							self->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
#endif
							self->client->ps.eFlags |= EF_AWARD_IMPRESSIVE;
							self->client->rewardTime = level.time + REWARD_SPRITE_TIME;
						}
						self->client->accuracy_hits++;
					}
				}
				else
#endif
				if (hits) {
					self->client->accuracy_hits++;
				}
			}

			// From weapon_railgun_fire
			if (bg_projectileinfo[projnum].trailType == PT_RAIL)
			{
				gentity_t	*tent;

				// the final trace endpos will be the terminal point of the rail trail

				// snap the endpos to integers to save net bandwidth, but nudged towards the line
				SnapVectorTowards( tr.endpos, start );

				// send railgun beam effect
				tent = G_TempEntity( tr.endpos, EV_RAILTRAIL );

				// set player number for custom colors on the railtrail
				tent->s.clientNum = self->s.clientNum;

				// Set projectile number
				tent->s.weapon = projnum;

				// ZTM: NOTE: This could be a problem if multiple hands use the same handSide.
				tent->s.weaponHands = MAX_HANDS;
				if (self->client) {
					for (i = 0; i < MAX_HANDS; i++)
					{
						if (self->client->pers.playercfg.handSide[i] == handSide) {
							tent->s.weaponHands = i;
							break;
						}
					}
				}

				VectorCopy( start, tent->s.origin2 );
				// move origin a bit to come closer to the drawn gun muzzle
				if (handSide == HS_RIGHT)
					VectorMA( tent->s.origin2, 4, right, tent->s.origin2 );
				else if (handSide == HS_LEFT)
					VectorMA( tent->s.origin2, -4, right, tent->s.origin2 );
				VectorMA( tent->s.origin2, -1, up, tent->s.origin2 );
			}
			continue;
		}

		bolt = G_Spawn();
		bolt->classname = bg_projectileinfo[projnum].name;
		if (self && self->client && bg_projectileinfo[projnum].grappling)
		{
			if (bg_projectileinfo[projnum].timetolive == -1)
				bolt->nextthink = -1;
			else
				bolt->nextthink = level.time + bg_projectileinfo[projnum].timetolive;

			bolt->think = Weapon_HookFree;

			self->client->hook = bolt;
		}
		else if (bg_projectileinfo[projnum].homing)
		{
			bolt->nextthink = level.time + bg_projectileinfo[projnum].homing;
			bolt->think = G_HomingMissile;
		}
		else
		{
			if (bg_projectileinfo[projnum].timetolive == -1)
				bolt->nextthink = -1;
			else
				bolt->nextthink = level.time + bg_projectileinfo[projnum].timetolive;

			if (bg_projectileinfo[projnum].fallToGround)
				bolt->think = G_MissileFall; // Just fall out of air.
			else
				bolt->think = G_ExplodeMissile;
		}
		bolt->s.eType = ET_MISSILE;
		bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
		bolt->s.weapon = projnum;
		if (self) {
			bolt->r.ownerNum = self->s.number;
		} else {
			bolt->r.ownerNum = ENTITYNUM_WORLD;
		}
		bolt->parent = self;
		// grapple
		bolt->s.otherEntityNum = bolt->r.ownerNum; // use to match beam in client

		if (!bg_projectileinfo[projnum].damageAttacker) {
			bolt->flags |= FL_MISSILE_NO_DAMAGE_PARENT;
		}

		bolt->damage = damage;
		bolt->splashDamage = splashDamage;
		bolt->splashRadius = splashRadius;

		bolt->methodOfDeath = mod;
		bolt->splashMethodOfDeath = splashMod;

		bolt->clipmask = MASK_SHOT;
		bolt->target_ent = NULL;

		VectorCopy( start, bolt->s.pos.trBase );

		bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME; // move a bit on the very first frame
		G_SetMissileVelocity(bolt, dir, projnum);

		VectorCopy (start, bolt->r.currentOrigin);

		if (bg_projectileinfo[projnum].bounceType == PB_FULL)
			bolt->s.eFlags = EF_BOUNCE;
		else if (bg_projectileinfo[projnum].bounceType == PB_HALF)
			bolt->s.eFlags = EF_BOUNCE_HALF;

		if (bg_projectileinfo[projnum].flags & PF_USE_GRAVITY)
			bolt->s.pos.trType = TR_GRAVITY;

		// Limit bounces
		bolt->s.modelindex2 = bg_projectileinfo[projnum].maxBounces;

		// ZTM: Shootable missiles, taken from XREAL
		if (bg_projectileinfo[projnum].shootable)
		{
			// Make the projectile shootable
			bolt->s.contents = CONTENTS_SHOOTABLE;
			VectorCopy(mins, bolt->s.mins);
			VectorCopy(maxs, bolt->s.maxs);
			bolt->takedamage = qtrue;
			bolt->health = bg_projectileinfo[projnum].damage;

			if (bg_projectileinfo[projnum].fallToGround)
				bolt->die = G_MissileFall_Die;
			else
				bolt->die = G_Missile_Die;
		}

		// Save handSide in missile
		bolt->s.weaponHands = handSide;

		if (self && self->client)
		{
			// Taken from Q3's fire_prox;
			// ZTM: Used by prox mines so that if that player changes teams the mines
			//        don't "change" teams as well (or something...).
			bolt->s.team = self->client->sess.sessionTeam;

			// Needed for stickOnImpact projectiles
			bolt->s.angles[0] = self->client->ps.viewangles[0];
			bolt->s.angles[1] = self->client->ps.viewangles[1];
			bolt->s.angles[2] = self->client->ps.viewangles[2];
		}
	}

	return qtrue;
}

// NOTE: weaponnum is *not* weapon group
qboolean fire_weapon(gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up, int weaponnum, float quadFactor, int handSide)
{
	if (weaponnum <= 0 || weaponnum >= BG_NumWeapons()) {
		return qfalse;
	}

	return fire_projectile(self, start, forward, right, up, bg_weaponinfo[weaponnum].projnum,
		quadFactor, bg_weaponinfo[weaponnum].mod, bg_weaponinfo[weaponnum].splashMod, handSide);
}

qboolean fire_weaponDir(gentity_t *self, vec3_t start, vec3_t dir, int weaponnum, float quadFactor, int handSide)
{
	vec3_t right, up;

	if (weaponnum <= 0 || weaponnum >= BG_NumWeapons()) {
		return qfalse;
	}

	// Get up and right from dir (which is "forward")
	PerpendicularVector( up, dir );
	CrossProduct( up, dir, right );

	return fire_projectile(self, start, dir, right, up, bg_weaponinfo[weaponnum].projnum,
		quadFactor, bg_weaponinfo[weaponnum].mod, bg_weaponinfo[weaponnum].splashMod, handSide);
}
#endif

#ifdef TURTLEARENA // HOLD_SHURIKEN
/*
=================
fire_shuriken
=================
*/
qboolean fire_shuriken (gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up, holdable_t holdable)
{
	int projnum;
	float s_quadFactor;

	if (self->client->ps.powerups[PW_QUAD] ) {
		s_quadFactor = g_quadfactor.value;
	} else {
		s_quadFactor = 1;
	}
#ifdef MISSIONPACK
	if( self->client->persistantPowerup && self->client->persistantPowerup->item && self->client->persistantPowerup->item->giTag == PW_DOUBLER ) {
		s_quadFactor *= 2;
	}
#endif

	projnum = BG_ProjectileIndexForHoldable(holdable);

	G_AutoAim(self, projnum, start, forward, right, up);

	return fire_projectile(self, start, forward, right, up, projnum,
				s_quadFactor, MOD_UNKNOWN, MOD_UNKNOWN, HS_CENTER);
}
#endif

#if defined MISSIONPACK || defined TA_WEAPSYS
/*
================
ProximityMine_Explode
================
*/
static void ProximityMine_Explode( gentity_t *mine ) {
	G_ExplodeMissile( mine );
	// if the prox mine has a trigger free it
	if (mine->activator) {
		G_FreeEntity(mine->activator);
		mine->activator = NULL;
	}
}

/*
================
ProximityMine_Die
================
*/
static void ProximityMine_Die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + 1;
}

/*
================
ProximityMine_Trigger
================
*/
void ProximityMine_Trigger( gentity_t *trigger, gentity_t *other, trace_t *trace ) {
	vec3_t		v;
	gentity_t	*mine;

	if( !other->client ) {
		return;
	}

	// trigger is a cube, do a distance test now to act as if it's a sphere
	VectorSubtract( trigger->s.pos.trBase, other->s.pos.trBase, v );
	if( VectorLength( v ) > trigger->parent->splashRadius ) {
		return;
	}


	if ( g_gametype.integer >= GT_TEAM ) {
		// don't trigger same team mines
		if (trigger->parent->s.team == other->client->sess.sessionTeam) {
			return;
		}
	}

	// ok, now check for ability to damage so we don't get triggered thru walls, closed doors, etc...
	if( !CanDamage( other, trigger->s.pos.trBase ) ) {
		return;
	}

	// trigger the mine!
	mine = trigger->parent;
	mine->s.loopSound = 0;
#ifdef TA_WEAPSYS
	G_AddEvent( mine, EV_PROJECTILE_TRIGGER, 0 );
#else
	G_AddEvent( mine, EV_PROXIMITY_MINE_TRIGGER, 0 );
#endif
	mine->nextthink = level.time + 500;

	G_FreeEntity( trigger );
}

/*
================
ProximityMine_Activate
================
*/
static void ProximityMine_Activate( gentity_t *ent ) {
	gentity_t	*trigger;
	float		r;

	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + g_proxMineTimeout.integer;

	ent->takedamage = qtrue;
	ent->health = 1;
	ent->die = ProximityMine_Die;

#ifdef TA_WEAPSYS // Per-projectile tick sound name
	if (bg_projectileinfo[ent->s.weapon].tickSoundName[0]) {
		ent->s.loopSound = G_SoundIndex(bg_projectileinfo[ent->s.weapon].tickSoundName);
	}
#else
	ent->s.loopSound = G_SoundIndex( "sound/weapons/proxmine/wstbtick.wav" );
#endif

	// build the proximity trigger
	trigger = G_Spawn ();

	trigger->classname = "proxmine_trigger";

	r = ent->splashRadius;
	VectorSet( trigger->s.mins, -r, -r, -r );
	VectorSet( trigger->s.maxs, r, r, r );

	G_SetOrigin( trigger, ent->s.pos.trBase );

	trigger->parent = ent;
	trigger->s.contents = CONTENTS_TRIGGER;
	trigger->touch = ProximityMine_Trigger;

	trap_LinkEntity (trigger);

	// set pointer to trigger so the entity can be freed when the mine explodes
	ent->activator = trigger;
}

#ifndef TURTLEARENA // WEAPONS
/*
================
ProximityMine_ExplodeOnPlayer
================
*/
static void ProximityMine_ExplodeOnPlayer( gentity_t *mine ) {
	gentity_t	*player;

	player = mine->enemy;
	player->client->ps.eFlags &= ~EF_TICKING;

#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
	if ( player->client->invulnerabilityTime > level.time ) {
		G_Damage( player, mine->parent, mine->parent, vec3_origin, mine->s.origin, 1000, DAMAGE_NO_KNOCKBACK, MOD_JUICED );
		player->client->invulnerabilityTime = 0;
		G_TempEntity( player->client->ps.origin, EV_JUICED );
	}
	else
#endif
	{
		G_SetOrigin( mine, player->s.pos.trBase );
		// make sure the explosion gets to the client
		mine->r.svFlags &= ~SVF_NOCLIENT;
#ifndef TA_WEAPSYS
		mine->splashMethodOfDeath = MOD_PROXIMITY_MINE;
#endif
		G_ExplodeMissile( mine );
	}
}

/*
================
ProximityMine_Player
================
*/
static void ProximityMine_Player( gentity_t *mine, gentity_t *player ) {
	if( mine->s.eFlags & EF_NODRAW ) {
		return;
	}

#ifdef TA_WEAPSYS
	G_AddEvent( mine, EV_PROJECTILE_STICK, 255 );
	mine->s.time2 = SURF_FLESH; // surfaceflags
#else
	G_AddEvent( mine, EV_PROXIMITY_MINE_STICK, 0 );
#endif

	if( player->s.eFlags & EF_TICKING ) {
		player->activator->splashDamage += mine->splashDamage;
		player->activator->splashRadius *= 1.50;
		mine->think = G_FreeEntity;
		mine->nextthink = level.time;
		return;
	}

	player->client->ps.eFlags |= EF_TICKING;
	player->activator = mine;

	mine->s.eFlags |= EF_NODRAW;
	mine->r.svFlags |= SVF_NOCLIENT;
	mine->s.pos.trType = TR_LINEAR;
	VectorClear( mine->s.pos.trDelta );

	mine->enemy = player;
	mine->think = ProximityMine_ExplodeOnPlayer;
#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
	if ( player->client->invulnerabilityTime > level.time ) {
		mine->nextthink = level.time + 2 * 1000;
	}
	else
#endif
	{
		mine->nextthink = level.time + 10 * 1000;
	}
}
#endif
#endif

/*
================
G_MissileImpact
================
*/
void G_MissileImpact( gentity_t *ent, trace_t *trace ) {
	gentity_t		*other;
	qboolean		hitClient = qfalse;
#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
	vec3_t			forward, impactpoint, bouncedir;
	int				eFlags;
#endif
#ifdef TA_WEAPSYS
	qboolean damagedOther = qfalse;
#endif
	other = &g_entities[trace->entityNum];

#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
	if ( other->takedamage ) {
#ifdef TA_WEAPSYS
		if ( !bg_projectileinfo[ent->s.weapon].stickOnImpact )
#else
		if ( ent->s.weapon != WP_PROX_LAUNCHER )
#endif
		{
			if ( other->client && other->client->invulnerabilityTime > level.time ) {
				//
				VectorCopy( ent->s.pos.trDelta, forward );
				VectorNormalize( forward );
				if (G_InvulnerabilityEffect( other, forward, ent->s.pos.trBase, impactpoint, bouncedir )) {
					VectorCopy( bouncedir, trace->plane.normal );
					eFlags = ent->s.eFlags & EF_BOUNCE_HALF;
					ent->s.eFlags &= ~EF_BOUNCE_HALF;
					G_BounceMissile( ent, trace );
					ent->s.eFlags |= eFlags;
				}
				ent->target_ent = other;
				return;
			}
		}
	}
#endif
	// impact damage
	if (other->takedamage
#ifdef TA_WEAPSYS // stickOnImpact only damages once
		&& !(ent->count & 2)
#endif
		)
	{
		// FIXME: wrong damage direction?
		if ( ent->damage ) {
			vec3_t	velocity;

			if( LogAccuracyHit( other, &g_entities[ent->r.ownerNum] ) ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
				hitClient = qtrue;
			}
			BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );
			if ( VectorLength( velocity ) == 0 ) {
#ifdef IOQ3ZTM
				VectorCopy(trace->plane.normal, velocity);
#else
				velocity[2] = 1;	// stepped on a grenade
#endif
			}
#ifdef TA_WEAPSYS
			damagedOther = G_Damage (other, ent, &g_entities[ent->r.ownerNum], velocity,
				ent->s.origin, ent->damage,
				0, ent->methodOfDeath);
#else
			G_Damage (other, ent, &g_entities[ent->r.ownerNum], velocity,
				ent->s.origin, ent->damage, 
				0, ent->methodOfDeath);
#endif
		}
	}

	// check for bounce
	if (
#ifdef TA_WEAPSYS
		!damagedOther &&
#else
		!other->takedamage &&
#endif
		( ent->s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) ) ) {
		G_BounceMissile( ent, trace );
#ifdef TA_WEAPSYS // Bounce missiles
		// Die on Nth bounce
		if (ent->s.modelindex2 > 0)
		{
			ent->s.modelindex2--;
			if (ent->s.modelindex2 == 0)
			{
				// Kill missile
				G_ExplodeMissile( ent );
				return;
			}
		}
		G_AddEvent( ent, EV_PROJECTILE_BOUNCE, DirToByte( trace->plane.normal ) );
		ent->s.time2 = trace->surfaceFlags; // surface
#else
		G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
#endif
		return;
	}

#ifdef TA_WEAPSYS
	if (bg_projectileinfo[ent->s.weapon].stickOnImpact != PSOI_NONE) {
		vec3_t dir;

#ifndef TURTLEARENA
		// if it's a player, stick it on to them (flag them and remove this entity)
		if( bg_projectileinfo[ent->s.weapon].explosionType == PE_PROX &&
			other->s.eType == ET_PLAYER && other->health > 0 )
		{
			ProximityMine_Player( ent, other );
			return;
		}
#endif

		if (ent->count & 2) {
			// Already stuck to wall
			return;
		}
		ent->count |= 2;

		// Don't stick to players or obelisk
		if (other->s.eType == ET_PLAYER
#ifdef MISSIONPACK
			|| (other->pain == ObeliskPain)
#endif
			)
		{
			goto missileExplode;
		}

		// Don't stick to the entity that this missile just killed.
		if (damagedOther && other->health <= 0)
		{
			// Don't remove porjectile if it doesn't explode.
			if (bg_projectileinfo[ent->s.weapon].explosionType == PE_NONE)
			{
				ent->s.pos.trType = TR_GRAVITY;
				ent->count &= ~2;
				return;
			}
			else
			{
				goto missileExplode;
			}
		}

		if (bg_projectileinfo[ent->s.weapon].shootable)
			VectorMA(trace->endpos, -8, trace->plane.normal, trace->endpos);

		SnapVectorTowards( trace->endpos, ent->s.pos.trBase );
		G_SetOrigin( ent, trace->endpos );

		if (bg_projectileinfo[ent->s.weapon].stickOnImpact == PSOI_KEEP_ANGLES) {
#if 0
			// convert direction of travel into axis
			if ( VectorNormalize2( ent->s.pos.trDelta, dir ) == 0 ) {
				dir[2] = 1;
			}

			// Set the angles
			vectoangles( dir, ent->s.angles );
#else
			VectorCopy(trace->plane.normal, dir);
#endif
		} else {
			VectorCopy(trace->plane.normal, dir);
			vectoangles( dir, ent->s.angles );

			switch (bg_projectileinfo[ent->s.weapon].stickOnImpact)
			{
				case PSOI_ANGLE_270:
					ent->s.angles[0] += 270;
					break;
				case PSOI_ANGLE_180:
					ent->s.angles[0] += 180;
					break;
				case PSOI_ANGLE_90:
					// Maybe this is good for prox mines, but doesn't look good on my
					//   rocket or shuirkens...
					ent->s.angles[0] += 90;
					break;
				case PSOI_ANGLE_0:
					break;
			}
		}

		// Save direction
		VectorCopy(dir, ent->s.angles2);

		ent->s.pos.trType = TR_STATIONARY;
		VectorClear( ent->s.pos.trDelta );

		G_AddEvent( ent, EV_PROJECTILE_STICK, DirToByte(trace->plane.normal) );
		ent->s.time2 = trace->surfaceFlags; // surface

		if (bg_projectileinfo[ent->s.weapon].explosionType == PE_PROX)
		{
			// When a BREAKABLE ET_MOVER is killed it drops the projectiles stuck to it,
			//   so don't setup the prox mine when it impact a surface if it already hit been setup.
			if (ent->die != ProximityMine_Die)
			{
				ent->think = ProximityMine_Activate;
				ent->nextthink = level.time + 2000;
				ent->die = ProximityMine_Die;
			}
		}
		else
		{
			ent->die = G_Missile_Die;
		}

		// link the prox mine to the other entity
		ent->enemy = other;
		VectorCopy(dir, ent->movedir);
		VectorSet(ent->s.mins, -4, -4, -4);
		VectorSet(ent->s.maxs, 4, 4, 4);
		trap_LinkEntity(ent);

		return;
	}
#elif defined MISSIONPACK
	if( ent->s.weapon == WP_PROX_LAUNCHER ) {
		if( ent->s.pos.trType != TR_GRAVITY ) {
			return;
		}

		// if it's a player, stick it on to them (flag them and remove this entity)
		if( other->s.eType == ET_PLAYER && other->health > 0 ) {
			ProximityMine_Player( ent, other );
			return;
		}

		SnapVectorTowards( trace->endpos, ent->s.pos.trBase );
		G_SetOrigin( ent, trace->endpos );
		ent->s.pos.trType = TR_STATIONARY;
		VectorClear( ent->s.pos.trDelta );

		G_AddEvent( ent, EV_PROXIMITY_MINE_STICK, trace->surfaceFlags );

		ent->think = ProximityMine_Activate;
		ent->nextthink = level.time + 2000;

		vectoangles( trace->plane.normal, ent->s.angles );
		ent->s.angles[0] += 90;

		// link the prox mine to the other entity
		ent->enemy = other;
		ent->die = ProximityMine_Die;
		VectorCopy(trace->plane.normal, ent->movedir);
		VectorSet(ent->s.mins, -4, -4, -4);
		VectorSet(ent->s.maxs, 4, 4, 4);
		trap_LinkEntity(ent);

		return;
	}
#endif

#ifdef TA_WEAPSYS
	if (bg_projectileinfo[ent->s.weapon].grappling)
#else
	if (!strcmp(ent->classname, "hook"))
#endif
	{
		gentity_t *nent;
		vec3_t v;

		nent = G_Spawn();
		if ( other->takedamage && other->client ) {

			G_AddEvent( nent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
			nent->s.otherEntityNum = other->s.number;

			ent->enemy = other;

			v[0] = other->r.currentOrigin[0] + (other->s.mins[0] + other->s.maxs[0]) * 0.5;
			v[1] = other->r.currentOrigin[1] + (other->s.mins[1] + other->s.maxs[1]) * 0.5;
			v[2] = other->r.currentOrigin[2] + (other->s.mins[2] + other->s.maxs[2]) * 0.5;

			SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth
		} else {
			VectorCopy(trace->endpos, v);
			G_AddEvent( nent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
#ifdef TA_WEAPSYS // GRAPPLE_MOVE
			ent->enemy = other;
#else
			ent->enemy = NULL;
#endif
		}
#ifdef IOQ3ZTM // IOQ3BUGFIX: Fix grapple wallmark/death-effect/debris (only tested with TA_WEAPSYS...)
		nent->s.weapon = ent->s.weapon;
#endif
#ifdef TA_WEAPSYS
		if (ent->parent)
			nent->s.clientNum = ent->parent->s.number;
		else
			nent->s.clientNum = ENTITYNUM_NONE;
#endif

		SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth

		nent->freeAfterEvent = qtrue;
		// change over to a normal entity right at the point of impact
		nent->s.eType = ET_GENERAL;
		ent->s.eType = ET_GRAPPLE;

		G_SetOrigin( ent, v );
		G_SetOrigin( nent, v );

		ent->think = Weapon_HookThink;
		ent->nextthink = level.time + FRAMETIME;

		ent->parent->client->ps.pm_flags |= PMF_GRAPPLE_PULL;
		VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);

		trap_LinkEntity( ent );
		trap_LinkEntity( nent );

#ifdef TA_WEAPSYS
		// Don't grapple to the entity if you just killed it.
		if (damagedOther && other->health <= 0)
		{
			Weapon_HookFree(ent);
		}
#endif

		return;
	}

#ifdef TA_WEAPSYS
missileExplode:
#endif

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if ( other->takedamage && other->client ) {
		G_AddEvent( ent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
		ent->s.otherEntityNum = other->s.number;
	} else if( trace->surfaceFlags & SURF_METALSTEPS ) {
		G_AddEvent( ent, EV_MISSILE_MISS_METAL, DirToByte( trace->plane.normal ) );
	} else {
		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
	}
#ifdef TA_WEAPSYS
	if (ent->parent)
		ent->s.clientNum = ent->parent->s.number;
	else
		ent->s.clientNum = ENTITYNUM_NONE;
#endif

	ent->freeAfterEvent = qtrue;

	// change over to a normal entity right at the point of impact
#ifndef TA_WEAPSYS // Must be after G_RadiusDamage
	ent->s.eType = ET_GENERAL;
#endif

	SnapVectorTowards( trace->endpos, ent->s.pos.trBase );	// save net bandwidth

	G_SetOrigin( ent, trace->endpos );

	// splash damage (doesn't apply to person directly hit)
	if ( ent->splashDamage ) {
#ifdef TA_WEAPSYS
		if( G_RadiusDamage( trace->endpos, ent, ent->parent, ent->splashDamage, ent->splashRadius, 
			other, ent->splashMethodOfDeath ) )
#else
		if( G_RadiusDamage( trace->endpos, ent->parent, ent->splashDamage, ent->splashRadius, 
			other, ent->splashMethodOfDeath ) )
#endif
		{
			if( !hitClient ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
			}
		}
	}

#ifdef TA_WEAPSYS
	ent->s.eType = ET_GENERAL;
#endif

	trap_LinkEntity( ent );
}

/*
================
G_RunMissile
================
*/
void G_RunMissile( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		tr;
	int			passent;

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// if this missile bounced off an invulnerability sphere
	if ( ent->target_ent ) {
		passent = ent->target_ent->s.number;
	}
#ifdef TA_WEAPSYS
	// missiles that left the owner bbox will attack anything, even the owner
	else if (ent->count & 1)
	{
		passent = ent->s.number;
	}
#elif defined MISSIONPACK
	// prox mines that left the owner bbox will attach to anything, even the owner
	else if (ent->s.weapon == WP_PROX_LAUNCHER && ent->count) {
		passent = ENTITYNUM_NONE;
	}
#endif
	else {
		// ignore interactions with the missile owner
		passent = ent->r.ownerNum;
	}
	// trace a line from the previous position to the current position
	trap_Trace( &tr, ent->r.currentOrigin, ent->s.mins, ent->s.maxs, origin, passent, ent->clipmask );

	if ( tr.startsolid || tr.allsolid ) {
		// make sure the tr.entityNum is set to the entity we're stuck in
		trap_Trace( &tr, ent->r.currentOrigin, ent->s.mins, ent->s.maxs, ent->r.currentOrigin, passent, ent->clipmask );
		tr.fraction = 0;
	}
	else {
		VectorCopy( tr.endpos, ent->r.currentOrigin );
	}

	trap_LinkEntity( ent );

	if ( tr.fraction != 1 ) {
		// never explode or bounce on sky
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// If grapple, reset owner
			if (ent->parent && ent->parent->client && ent->parent->client->hook == ent) {
#ifdef IOQ3ZTM
				Weapon_HookFree(ent);
				return;
#else
				ent->parent->client->hook = NULL;
#endif
			}
			G_FreeEntity( ent );
			return;
		}
		G_MissileImpact( ent, &tr );
		if ( ent->s.eType != ET_MISSILE ) {
			return;		// exploded
		}
	}
#if defined MISSIONPACK || defined TA_WEAPSYS
	// if the prox mine wasn't yet outside the player body
#ifdef TA_WEAPSYS
	if (!(ent->count & 1) && !(ent->flags & FL_MISSILE_NO_DAMAGE_PARENT))
#else
	if (ent->s.weapon == WP_PROX_LAUNCHER && !ent->count)
#endif
	{
		// check if the prox mine is outside the owner bbox
		trap_Trace( &tr, ent->r.currentOrigin, ent->s.mins, ent->s.maxs, ent->r.currentOrigin, ENTITYNUM_NONE, ent->clipmask );
		if (!tr.startsolid || tr.entityNum != ent->r.ownerNum) {
#ifdef TA_WEAPSYS
			ent->count |= 1;
#else
			ent->count = 1;
#endif
		}
	}
#endif
	// check think function after bouncing
	G_RunThink( ent );
}


#ifndef TA_WEAPSYS // There is now a fire_projectile that replaces all of these.
//=============================================================================

/*
=================
fire_plasma

=================
*/
gentity_t *fire_plasma (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "plasma";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PLASMAGUN;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 20;
	bolt->splashDamage = 15;
	bolt->splashRadius = 20;
	bolt->methodOfDeath = MOD_PLASMA;
	bolt->splashMethodOfDeath = MOD_PLASMA_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 2000, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	if ( self->client ) {
		bolt->s.team = self->client->sess.sessionTeam;
	} else {
		bolt->s.team = TEAM_FREE;
	}

	return bolt;
}	

//=============================================================================


/*
=================
fire_grenade
=================
*/
gentity_t *fire_grenade (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "grenade";
	bolt->nextthink = level.time + 2500;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_GRENADE_LAUNCHER;
	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 150;
	bolt->methodOfDeath = MOD_GRENADE;
	bolt->splashMethodOfDeath = MOD_GRENADE_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 700, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	if ( self->client ) {
		bolt->s.team = self->client->sess.sessionTeam;
	} else {
		bolt->s.team = TEAM_FREE;
	}

	return bolt;
}

//=============================================================================


/*
=================
fire_bfg
=================
*/
gentity_t *fire_bfg (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "bfg";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_BFG;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_BFG;
	bolt->splashMethodOfDeath = MOD_BFG_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 2000, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	if ( self->client ) {
		bolt->s.team = self->client->sess.sessionTeam;
	} else {
		bolt->s.team = TEAM_FREE;
	}

	return bolt;
}

//=============================================================================


/*
=================
fire_rocket
=================
*/
gentity_t *fire_rocket (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "rocket";
	bolt->nextthink = level.time + 15000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_ROCKET_LAUNCHER;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_ROCKET;
	bolt->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 900, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	if ( self->client ) {
		bolt->s.team = self->client->sess.sessionTeam;
	} else {
		bolt->s.team = TEAM_FREE;
	}

	return bolt;
}

/*
=================
fire_grapple
=================
*/
gentity_t *fire_grapple (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*hook;

	VectorNormalize (dir);

	hook = G_Spawn();
	hook->classname = "hook";
	hook->nextthink = level.time + 10000;
	hook->think = Weapon_HookFree;
	hook->s.eType = ET_MISSILE;
	hook->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	hook->s.weapon = WP_GRAPPLING_HOOK;
	hook->r.ownerNum = self->s.number;
	hook->methodOfDeath = MOD_GRAPPLE;
	hook->clipmask = MASK_SHOT;
	hook->parent = self;
	hook->target_ent = NULL;

	hook->s.pos.trType = TR_LINEAR;
	hook->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	hook->s.otherEntityNum = self->s.number; // use to match beam in client
	VectorCopy( start, hook->s.pos.trBase );
	VectorScale( dir, 800, hook->s.pos.trDelta );
	SnapVector( hook->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, hook->r.currentOrigin);

	if ( self->client ) {
		self->client->hook = hook;
		hook->s.team = self->client->sess.sessionTeam;
	} else {
		hook->s.team = TEAM_FREE;
	}

	return hook;
}


#ifdef MISSIONPACK
/*
=================
fire_nail
=================
*/
#define NAILGUN_SPREAD	500

gentity_t *fire_nail( gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up ) {
	gentity_t	*bolt;
	vec3_t		dir;
	vec3_t		end;
	float		r, u, scale;

	bolt = G_Spawn();
	bolt->classname = "nail";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_NAILGUN;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 20;
	bolt->methodOfDeath = MOD_NAIL;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;
	VectorCopy( start, bolt->s.pos.trBase );

	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * NAILGUN_SPREAD * 16;
	r = cos(r) * crandom() * NAILGUN_SPREAD * 16;
	VectorMA( start, 8192 * 16, forward, end);
	VectorMA (end, r, right, end);
	VectorMA (end, u, up, end);
	VectorSubtract( end, start, dir );
	VectorNormalize( dir );

	scale = 555 + random() * 1800;
	VectorScale( dir, scale, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );

	VectorCopy( start, bolt->r.currentOrigin );

	if ( self->client ) {
		bolt->s.team = self->client->sess.sessionTeam;
	} else {
		bolt->s.team = TEAM_FREE;
	}

	return bolt;
}	


/*
=================
fire_prox
=================
*/
gentity_t *fire_prox( gentity_t *self, vec3_t start, vec3_t dir ) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "prox mine";
	bolt->nextthink = level.time + 3000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PROX_LAUNCHER;
	bolt->s.eFlags = 0;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 0;
	bolt->splashDamage = 100;
	bolt->splashRadius = 150;
	bolt->methodOfDeath = MOD_PROXIMITY_MINE;
	bolt->splashMethodOfDeath = MOD_PROXIMITY_MINE;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;
	// count is used to check if the prox mine left the player bbox
	// if count == 1 then the prox mine left the player bbox and can attack to it
	bolt->count = 0;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 700, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	if ( self->client ) {
		bolt->s.team = self->client->sess.sessionTeam;
	} else {
		bolt->s.team = TEAM_FREE;
	}

	return bolt;
}
#endif

#endif // #ifndef TA_WEAPSYS
