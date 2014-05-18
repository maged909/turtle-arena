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
// g_combat.c

#include "g_local.h"


/*
============
ScorePlum
============
*/
void ScorePlum( gentity_t *ent, vec3_t origin, int score ) {
	gentity_t *plum;

	plum = G_TempEntity( origin, EV_SCOREPLUM );
	// only send this temp entity to a single client
	plum->r.svFlags |= SVF_CLIENTMASK;
	Com_ClientListClear( &plum->r.sendClients );
	Com_ClientListAdd( &plum->r.sendClients, ent->s.number );
	//
	plum->s.otherEntityNum = ent->s.number;
	plum->s.time = score;
}

#ifdef TURTLEARENA // NIGHTS_ITEMS
/*
============
ChainPlum
============
*/
void ChainPlum( gentity_t *ent, vec3_t origin, int score, int chain) {
	gentity_t *plum;

	plum = G_TempEntity( origin, EV_CHAINPLUM );
	// only send this temp entity to a single client
	plum->r.svFlags |= SVF_CLIENTMASK;
	Com_ClientListClear( &plum->r.sendClients );
	Com_ClientListAdd( &plum->r.sendClients, ent->s.number );
	//
	plum->s.otherEntityNum = ent->s.number;
	plum->s.time = score;
	plum->s.generic1 = chain;
	plum->s.time2 = (ent->client->ps.eFlags & EF_BONUS);
}
#endif

/*
============
AddScore

Adds score to both the client and his team
============
*/
#ifdef TURTLEARENA // NIGHTS_ITEMS
void AddScoreEx( gentity_t *ent, vec3_t origin, int score, int chain)
#else
void AddScore( gentity_t *ent, vec3_t origin, int score )
#endif
{
	if ( !ent->client ) {
		return;
	}
	// no scoring during pre-match warmup
	if ( level.warmupTime ) {
		return;
	}
#ifdef IOQ3ZTM // TURTLEARENA // SCORING // CTF_FRAG_CARRIER_BONUS is 0, so don't show they got 0 points.
	if (!score) {
		return;
	}
#endif
#ifdef TURTLEARENA // NIGHTS_ITEMS
	// NiGHTS mode bonus time
	if (ent->client->ps.eFlags & EF_BONUS) {
		score *= 2;
	}

	if (chain > 0) {
		ChainPlum(ent, origin, score, chain);
	} else
#endif
	// show score plum
	ScorePlum(ent, origin, score);
	//
	ent->client->ps.persistant[PERS_SCORE] += score;
	if ( g_gametype.integer == GT_TEAM ) {
		AddTeamScore( origin, ent->client->ps.persistant[PERS_TEAM], score );
	}
#ifdef TURTLEARENA // SCORING
	// No negative scores
	if (ent->client->ps.persistant[PERS_SCORE] < 0) {
		ent->client->ps.persistant[PERS_SCORE] = 0;
	}
	if ( g_gametype.integer == GT_TEAM && level.teamScores[ ent->client->ps.persistant[PERS_TEAM] ] < 0) {
		level.teamScores[ ent->client->ps.persistant[PERS_TEAM] ] = 0;
	}
#endif
	CalculateRanks();
}

#ifdef TURTLEARENA // NIGHTS_ITEMS
void AddScore( gentity_t *ent, vec3_t origin, int score) {
	AddScoreEx(ent, origin, score, 0);
}
#endif

/*
=================
TossClientItems

Toss the weapon and powerups for the killed player
=================
*/
void TossClientItems( gentity_t *self ) {
	gitem_t			*item;
	int				weapon;
	float			angle;
	int				i;
	gentity_t		*drop;
#ifdef TA_WEAPSYS_EX
	int				statAmmo = -1;
#endif

#ifdef IOQ3ZTM
	if (!self || !self->client) {
		return;
	}
#endif

#ifdef TA_WEAPSYS_EX
	if (self->client)
	{
		// Clients can have two weapon, pickup and default,
		//   only the pickup weapon will be dropped.

		weapon = WP_NONE;

		// don't drop default weapon
		if (self->client->ps.stats[STAT_PENDING_WEAPON]
			!= self->client->ps.stats[STAT_DEFAULTWEAPON])
		{
			weapon = self->client->ps.stats[STAT_PENDING_WEAPON];
			self->client->ps.stats[STAT_PENDING_WEAPON] = WP_NONE;
			statAmmo = STAT_PENDING_AMMO;
		}
		// else if has pickup weapon
		else if (self->s.weapon != self->client->ps.stats[STAT_DEFAULTWEAPON])
		{
			weapon = self->s.weapon;
			self->s.weapon = WP_NONE;
			statAmmo = STAT_AMMO;
		}
	}
	else
	{
		weapon = self->s.weapon;
	}
#else
	// drop the weapon if not a gauntlet or machinegun
	weapon = self->s.weapon;

	// make a special check to see if they are changing to a new
	// weapon that isn't the mg or gauntlet.  Without this, a client
	// can pick up a weapon, be killed, and not drop the weapon because
	// their weapon change hasn't completed yet and they are still holding the MG.
#ifdef TA_WEAPSYS
	if (weapon == self->client->ps.stats[STAT_DEFAULTWEAPON]) {
#else
	if ( weapon == WP_MACHINEGUN || weapon == WP_GRAPPLING_HOOK ) {
#endif
		if ( self->client->ps.weaponstate == WEAPON_DROPPING ) {
			BG_DecomposeUserCmdValue( self->client->pers.cmd.stateValue, &weapon );
		}
		if ( !( self->client->ps.stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
			weapon = WP_NONE;
		}
	}
#endif // !TA_WEAPSYS_EX

#ifdef TA_WEAPSYS_EX
	// ZTM: Drop valid selected weapon to drop
	if (weapon > WP_NONE && weapon < BG_NumWeaponGroups())
#elif defined TA_WEAPSYS
	// ZTM: Drop all weapons except default.
	if ( weapon != self->client->ps.stats[STAT_DEFAULTWEAPON] )
#else
	if ( weapon > WP_MACHINEGUN && weapon != WP_GRAPPLING_HOOK && 
		self->client->ps.ammo[ weapon ] )
#endif
	{
		// find the item type for this weapon
		item = BG_FindItemForWeapon( weapon );

		if (item) {
			// spawn the item
#ifdef TA_WEAPSYS
			drop = Drop_Item( self, item, 0 );
			if (drop) {
				int ammo;
#ifdef TA_WEAPSYS_EX
				if (statAmmo != -1)
				{
					ammo = self->client->ps.stats[statAmmo];
					self->client->ps.stats[statAmmo] = 0;
				}
				else
				{
					// Give default weapon ammo on pickup
					ammo = 0;
				}
#else
				ammo = self->client->ps.ammo[weapon];
#endif

				drop->count = ammo;
			}

			// Don't have corpse hold dropped weapon
			self->s.weapon = self->client->ps.weapon = WP_NONE;
#else
			Drop_Item( self, item, 0 );
#endif
		}
	}

#ifdef TA_HOLDSYS
	angle = 45;
#endif
	// drop all the powerups if not in teamplay
	if ( g_gametype.integer != GT_TEAM ) {
#ifndef TA_HOLDSYS
		angle = 45;
#endif
		for ( i = 1 ; i < PW_NUM_POWERUPS ; i++ ) {
			if ( self->client->ps.powerups[ i ] > level.time ) {
				item = BG_FindItemForPowerup( i );
				if ( !item ) {
					continue;
				}
				drop = Drop_Item( self, item, angle );
				// decide how many seconds it has left
				drop->count = ( self->client->ps.powerups[ i ] - level.time ) / 1000;
				if ( drop->count < 1 ) {
					drop->count = 1;
				}
				angle += 45;
			}
		}
	}
#ifdef TA_HOLDSYS
	// drop all the holdable items
	for ( i = 1 ; i < BG_NumHoldableItems() ; i++ ) {
		if ( self->client->ps.holdable[ i ] != 0 ) {
			item = BG_FindItemForHoldable( i );
			if ( !item ) {
				continue;
			}
			drop = Drop_Item( self, item, angle );
			// Save the use count
			drop->count = self->client->ps.holdable[ i ];
			angle += 45;
		}
	}
#endif
}

/*
===========
TossClientGametypeItems

Drop Flag in CTF, other gametypes could drop gametype specific items as well.
===========
*/
void TossClientGametypeItems(gentity_t *ent)
{
	int j;
	gitem_t *item;
	gentity_t *drop;
	int angle = 0;

	// drop flags in CTF
	item = NULL;
	j = 0;

	if ( ent->client->ps.powerups[ PW_REDFLAG ] ) {
		item = BG_FindItemForPowerup( PW_REDFLAG );
		j = PW_REDFLAG;
	} else if ( ent->client->ps.powerups[ PW_BLUEFLAG ] ) {
		item = BG_FindItemForPowerup( PW_BLUEFLAG );
		j = PW_BLUEFLAG;
	} else if ( ent->client->ps.powerups[ PW_NEUTRALFLAG ] ) {
		item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		j = PW_NEUTRALFLAG;
	}

	if ( item ) {
		drop = Drop_Item( ent, item, angle );
#ifdef IOQ3ZTM
		angle += 45;
#endif
		// decide how many seconds it has left
		drop->count = ( ent->client->ps.powerups[ j ] - level.time ) / 1000;
		if ( drop->count < 1 ) {
			drop->count = 1;
		}
		ent->client->ps.powerups[ j ] = 0;
	}

#ifdef MISSIONPACK_HARVESTER
	if ( g_gametype.integer == GT_HARVESTER ) {
		if ( ent->client->ps.tokens > 0 ) {
			if ( ent->client->sess.sessionTeam == TEAM_RED ) {
				item = BG_FindItem( "Blue Cube" );
			} else {
				item = BG_FindItem( "Red Cube" );
			}
			if ( item ) {
				for ( j = 0; j < ent->client->ps.tokens; j++ ) {
					drop = Drop_Item( ent, item, angle );
#ifdef IOQ3ZTM
					angle += 45;
#endif
					if ( ent->client->sess.sessionTeam == TEAM_RED ) {
						drop->s.team = TEAM_BLUE;
					} else {
						drop->s.team = TEAM_RED;
					}
				}
			}
			ent->client->ps.tokens = 0;
		}
	}
#endif
}

#ifdef MISSIONPACK

/*
=================
TossClientCubes
=================
*/
extern gentity_t	*neutralObelisk;

#ifdef MISSIONPACK_HARVESTER
void TossClientCubes( gentity_t *self ) {
	gitem_t		*item;
	gentity_t	*drop;
	vec3_t		velocity;
	vec3_t		angles;
	vec3_t		origin;

	self->client->ps.tokens = 0;

	// this should never happen but we should never
	// get the server to crash due to skull being spawned in
	if (!G_EntitiesFree()) {
		return;
	}

	if( self->client->sess.sessionTeam == TEAM_RED ) {
		item = BG_FindItem( "Red Cube" );
	}
	else {
		item = BG_FindItem( "Blue Cube" );
	}

	angles[YAW] = (float)(level.time % 360);
	angles[PITCH] = 0;	// always forward
	angles[ROLL] = 0;

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 150, velocity );
	velocity[2] += 200 + crandom() * 50;

	if( neutralObelisk ) {
		VectorCopy( neutralObelisk->s.pos.trBase, origin );
		origin[2] += 44;
	} else {
		VectorClear( origin ) ;
	}

	drop = LaunchItem( item, origin, velocity );

	drop->nextthink = level.time + g_cubeTimeout.integer * 1000;
	drop->think = G_FreeEntity;
	drop->s.team = self->client->sess.sessionTeam;
}
#endif


/*
=================
TossClientPersistantPowerups
=================
*/
void TossClientPersistantPowerups( gentity_t *ent ) {
	gentity_t	*powerup;

	if( !ent->client ) {
		return;
	}

	if( !ent->client->persistantPowerup ) {
		return;
	}

	powerup = ent->client->persistantPowerup;

	powerup->r.svFlags &= ~SVF_NOCLIENT;
	powerup->s.eFlags &= ~EF_NODRAW;
	powerup->s.contents = CONTENTS_TRIGGER;
	trap_LinkEntity( powerup );

	ent->client->ps.stats[STAT_PERSISTANT_POWERUP] = 0;
	ent->client->persistantPowerup = NULL;
}
#endif


/*
==================
LookAtKiller
==================
*/
void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker ) {
	vec3_t		dir;

	if ( attacker && attacker != self ) {
		VectorSubtract (attacker->s.pos.trBase, self->s.pos.trBase, dir);
	} else if ( inflictor && inflictor != self ) {
		VectorSubtract (inflictor->s.pos.trBase, self->s.pos.trBase, dir);
	} else {
		self->client->ps.stats[STAT_DEAD_YAW] = self->s.angles[YAW];
		return;
	}

	self->client->ps.stats[STAT_DEAD_YAW] = vectoyaw ( dir );
}

#ifndef NOTRATEDM // No gibs.
/*
==================
GibEntity
==================
*/
void GibEntity( gentity_t *self, int killer ) {
	gentity_t *ent;
	int i;

#ifndef TURTLEARENA // NO_KAMIKAZE_ITEM
	//if this entity still has kamikaze
	if (self->s.eFlags & EF_KAMIKAZE) {
		// check if there is a kamikaze timer around for this owner
		for (i = 0; i < level.num_entities; i++) {
			ent = &g_entities[i];
			if (!ent->inuse)
				continue;
			if (ent->activator != self)
				continue;
			if (strcmp(ent->classname, "kamikaze timer"))
				continue;
			G_FreeEntity(ent);
			break;
		}
	}
#endif
	G_AddEvent( self, EV_GIB_PLAYER, killer );
	self->takedamage = qfalse;
	self->s.eType = ET_INVISIBLE;
	self->s.contents = 0;

	if (self->client) {
		self->client->ps.contents = 0;
	}
}
#endif

/*
==================
body_die
==================
*/
void body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
#ifndef NOTRATEDM // No gibs.
	if ( self->health > GIB_HEALTH ) {
		return;
	}
	if ( !g_blood.integer ) {
		self->health = GIB_HEALTH+1;
		return;
	}

	GibEntity( self, 0 );
#endif
}


#if defined MISSIONPACK && !defined TURTLEARENA // NO_KAMIKAZE_ITEM
/*
==================
Kamikaze_DeathActivate
==================
*/
void Kamikaze_DeathActivate( gentity_t *ent ) {
	G_StartKamikaze(ent);
	G_FreeEntity(ent);
}

/*
==================
Kamikaze_DeathTimer
==================
*/
void Kamikaze_DeathTimer( gentity_t *self ) {
	gentity_t *ent;

	ent = G_Spawn();
	ent->classname = "kamikaze timer";
	VectorCopy(self->s.pos.trBase, ent->s.pos.trBase);
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->think = Kamikaze_DeathActivate;
	ent->nextthink = level.time + 5 * 1000;

	ent->activator = self;
}

#endif

#ifndef NOTRATEDM // Disable strong lang.
/*
==================
CheckAlmostCapture
==================
*/
void CheckAlmostCapture( gentity_t *self, gentity_t *attacker ) {
	gentity_t	*ent;
	vec3_t		dir;
	char		*classname;

	// if this player was carrying a flag
	if ( self->client->ps.powerups[PW_REDFLAG] ||
		self->client->ps.powerups[PW_BLUEFLAG] ||
		self->client->ps.powerups[PW_NEUTRALFLAG] ) {
		// get the goal flag this player should have been going for
		if ( g_gametype.integer == GT_CTF ) {
			if ( self->client->sess.sessionTeam == TEAM_BLUE ) {
				classname = "team_CTF_blueflag";
			}
			else {
				classname = "team_CTF_redflag";
			}
		}
		else {
			if ( self->client->sess.sessionTeam == TEAM_BLUE ) {
				classname = "team_CTF_redflag";
			}
			else {
				classname = "team_CTF_blueflag";
			}
		}
		ent = NULL;
		do
		{
			ent = G_Find(ent, FOFS(classname), classname);
		} while (ent && (ent->flags & FL_DROPPED_ITEM));
		// if we found the destination flag and it's not picked up
		if (ent && !(ent->r.svFlags & SVF_NOCLIENT) ) {
			// if the player was *very* close
			VectorSubtract( self->client->ps.origin, ent->s.origin, dir );
			if ( VectorLength(dir) < 200 ) {
				self->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				if ( attacker->client ) {
					attacker->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				}
			}
		}
	}
}

/*
==================
CheckAlmostScored
==================
*/
void CheckAlmostScored( gentity_t *self, gentity_t *attacker ) {
	gentity_t	*ent;
	vec3_t		dir;
	char		*classname;

	// if the player was carrying cubes
	if ( self->client->ps.tokens ) {
		if ( self->client->sess.sessionTeam == TEAM_BLUE ) {
			classname = "team_redobelisk";
		}
		else {
			classname = "team_blueobelisk";
		}
		ent = G_Find(NULL, FOFS(classname), classname);
		// if we found the destination obelisk
		if ( ent ) {
			// if the player was *very* close
			VectorSubtract( self->client->ps.origin, ent->s.origin, dir );
			if ( VectorLength(dir) < 200 ) {
				self->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				if ( attacker->client ) {
					attacker->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				}
			}
		}
	}
}
#endif

/*
==================
player_die
==================
*/
void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	gentity_t	*ent;
	int			anim;
#ifndef NOTRATEDM // No gibs.
	int			contents;
#endif
	int			killer;
	int			i;
	char		*killerName, *obit;
#ifdef TA_WEAPSYS
	int			weaponGroup = 0;
	int			projectile = 0;
#endif

	if ( self->client->ps.pm_type == PM_DEAD ) {
		return;
	}

	if ( level.intermissiontime ) {
		return;
	}

#ifndef NOTRATEDM // Disable strong lang.
	// check for an almost capture
	CheckAlmostCapture( self, attacker );
	// check for a player that almost brought in cubes
	CheckAlmostScored( self, attacker );
#endif

#ifdef IOQ3ZTM // Better grapple.
	self->client->ps.pm_type = PM_DEAD;
#endif

	if (self->client && self->client->hook) {
		Weapon_HookFree(self->client->hook);
	}
#if defined MISSIONPACK && !defined TURTLEARENA // WEAPONS
	if ((self->client->ps.eFlags & EF_TICKING) && self->activator) {
		self->client->ps.eFlags &= ~EF_TICKING;
		self->activator->think = G_FreeEntity;
		self->activator->nextthink = level.time;
	}
#endif
#ifndef IOQ3ZTM // Better grapple.
	self->client->ps.pm_type = PM_DEAD;
#endif

	if ( attacker ) {
		killer = attacker->s.number;
		if ( attacker->client ) {
			killerName = attacker->client->pers.netname;
		} else {
			killerName = "<non-client>";
		}
	} else {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( killer < 0 || killer >= MAX_CLIENTS ) {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( meansOfDeath < 0 || meansOfDeath >= modNamesSize ) {
		obit = "<bad obituary>";
#ifdef TA_WEAPSYS
	} else if (meansOfDeath == MOD_PROJECTILE || meansOfDeath == MOD_PROJECTILE_EXPLOSION) {
		if (inflictor && inflictor->s.eType == ET_MISSILE) {
			projectile = inflictor->s.weapon;
		}
		// Check for instant damage guns
		else if (attacker && attacker == inflictor && attacker->client
			&& bg_weapongroupinfo[attacker->s.weapon].weapon[0]->proj->instantDamage)
		{
			projectile = bg_weapongroupinfo[attacker->s.weapon].weapon[0]->projnum;
		}

		if (projectile > 0 && projectile < BG_NumProjectiles()) {
			obit = va("%s [%s]", modNames[meansOfDeath], bg_projectileinfo[projectile].name);
		} else {
			projectile = 0;
			obit = va("%s <unknown-projectile>", modNames[meansOfDeath]);
		}
	} else if (meansOfDeath == MOD_WEAPON_PRIMARY || meansOfDeath == MOD_WEAPON_SECONDARY) {
		if (attacker && attacker->client) {
			weaponGroup = attacker->s.weapon;
		}

		if (weaponGroup > 0 && weaponGroup < BG_NumWeaponGroups()) {
			obit = va("%s [%s]", modNames[meansOfDeath], bg_weapongroupinfo[weaponGroup].weapon[meansOfDeath-MOD_WEAPON_PRIMARY]->name);
		} else {
			weaponGroup = 0;
			obit = va("%s <unknown-weapon>", modNames[meansOfDeath]);
		}
#endif
	} else {
		obit = modNames[meansOfDeath];
	}

	G_LogPrintf("Kill: %i %i %i: %s killed %s by %s\n", 
		killer, self->s.number, meansOfDeath, killerName, 
		self->client->pers.netname, obit );

	// don't send death obituary when swiching teams
	if (meansOfDeath != MOD_SUICIDE_TEAM_CHANGE) {
		// broadcast the death event to everyone
		ent = G_TempEntity( self->r.currentOrigin, EV_OBITUARY );
		ent->s.eventParm = meansOfDeath;
		ent->s.otherEntityNum = self->s.number;
		ent->s.otherEntityNum2 = killer;
#ifdef TA_WEAPSYS
		// projectile or weapon group number
		if (meansOfDeath == MOD_PROJECTILE || meansOfDeath == MOD_PROJECTILE_EXPLOSION) {
			ent->s.weapon = projectile;
		} else if (meansOfDeath == MOD_WEAPON_PRIMARY || meansOfDeath == MOD_WEAPON_SECONDARY) {
			ent->s.weapon = weaponGroup;
		} else {
			ent->s.weapon = 0;
		}
#endif
		ent->r.svFlags = SVF_BROADCAST;	// send to everyone
	}

	self->enemy = attacker;

	self->client->ps.persistant[PERS_KILLED]++;
#ifdef TA_SP // Loss a life when you die.
	if (self->client->ps.persistant[PERS_LIVES] > 0) {
		self->client->ps.persistant[PERS_LIVES]--;
	}
#endif

	if (attacker && attacker->client) {
		attacker->client->lastkilled_client = self->s.number;

#ifdef TURTLEARENA // SCORING
		if ( attacker == self )
		{
			// Suicide penalty
			AddScore( attacker, self->r.currentOrigin, -50 );
		}
		else if (OnSameTeam (self, attacker))
		{
			// Don't loss or gain points for killing team mate.
		}
#else
		if ( attacker == self || OnSameTeam (self, attacker ) ) {
			AddScore( attacker, self->r.currentOrigin, -1 );
		}
#endif
		else
		{
#ifdef TURTLEARENA // SCORING
			AddScore( attacker, self->r.currentOrigin, 50 );
#else
			AddScore( attacker, self->r.currentOrigin, 1 );
#endif

#ifndef TURTLEARENA // AWARDS
			if( meansOfDeath == MOD_GAUNTLET ) {
				
				// play humiliation on player
				attacker->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;

				// add the sprite over the player's head
#ifdef IOQ3ZTM
				attacker->client->ps.eFlags &= ~EF_AWARD_BITS;
#else
				attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
#endif
				attacker->client->ps.eFlags |= EF_AWARD_GAUNTLET;
				attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

				// also play humiliation on target
				self->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_GAUNTLETREWARD;
			}

			// check for two kills in a short amount of time
			// if this is close enough to the last kill, give a reward sound
			if ( level.time - attacker->client->lastKillTime < CARNAGE_REWARD_TIME ) {
				// play excellent on player
				attacker->client->ps.persistant[PERS_EXCELLENT_COUNT]++;

				// add the sprite over the player's head
#ifdef IOQ3ZTM
				attacker->client->ps.eFlags &= ~EF_AWARD_BITS;
#else
				attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
#endif
				attacker->client->ps.eFlags |= EF_AWARD_EXCELLENT;
				attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;
			}
			attacker->client->lastKillTime = level.time;
#endif
		}
	} else {
#ifdef TURTLEARENA // SCORING
		// Suicide penalty
		AddScore( self, self->r.currentOrigin, -50 );
#else
		AddScore( self, self->r.currentOrigin, -1 );
#endif
	}

	// Add team bonuses
	Team_FragBonuses(self, inflictor, attacker);

#ifndef TA_MISC // DROP_FLAG
	// if I committed suicide, the flag does not fall, it returns.
	if (meansOfDeath == MOD_SUICIDE || meansOfDeath == MOD_SUICIDE_TEAM_CHANGE) {
		if ( self->client->ps.powerups[PW_NEUTRALFLAG] ) {		// only happens in One Flag CTF
			Team_ReturnFlag( TEAM_FREE );
			self->client->ps.powerups[PW_NEUTRALFLAG] = 0;
		}
		else if ( self->client->ps.powerups[PW_REDFLAG] ) {		// only happens in standard CTF
			Team_ReturnFlag( TEAM_RED );
			self->client->ps.powerups[PW_REDFLAG] = 0;
		}
		else if ( self->client->ps.powerups[PW_BLUEFLAG] ) {	// only happens in standard CTF
			Team_ReturnFlag( TEAM_BLUE );
			self->client->ps.powerups[PW_BLUEFLAG] = 0;
		}
	}
#endif

	TossClientItems( self );
#ifdef MISSIONPACK
	TossClientPersistantPowerups( self );
#ifdef MISSIONPACK_HARVESTER
	if( g_gametype.integer == GT_HARVESTER ) {
		TossClientCubes( self );
	}
#endif
#endif

	Cmd_Score_f( self );		// show scores
	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		gclient_t	*client;

		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
#ifdef IOQ3ZTM // SPECTATOR_FIX
		if ( client->sess.spectatorState != SPECTATOR_FOLLOW ) {
			continue;
		}
#endif
		if ( client->sess.spectatorClient == self->s.number ) {
			Cmd_Score_f( g_entities + i );
		}
	}

	self->takedamage = qtrue;	// can still be gibbed

#ifndef TA_WEAPSYS // Players don't always let go of weapon. (Set by TossClientItems if needed)
	self->s.weapon = WP_NONE;
#endif
	self->s.powerups = 0;
	self->client->ps.contents = self->s.contents = CONTENTS_CORPSE;

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;
	LookAtKiller (self, inflictor, attacker);

	VectorCopy( self->s.angles, self->client->ps.viewangles );

	self->s.loopSound = 0;

#ifdef TA_PLAYERSYS // Use per-player bounding box
	self->client->ps.maxs[2] = self->s.maxs[2] = self->client->pers.playercfg.deadmax;
#else
	self->client->ps.maxs[2] = self->s.maxs[2] = -8;
#endif

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	self->client->respawnTime = level.time + 1700;

	// remove powerups
	memset( self->client->ps.powerups, 0, sizeof(self->client->ps.powerups) );

	// never gib in a nodrop
#ifndef NOTRATEDM // No gibs.
	contents = trap_PointContents( self->r.currentOrigin, -1 );

	if ( (self->health <= GIB_HEALTH && !(contents & CONTENTS_NODROP) && g_blood.integer) || meansOfDeath == MOD_SUICIDE) {
		// gib death
		GibEntity( self, killer );
	} else
#endif
	{
		// normal death
		static int i;

		switch ( i ) {
		case 0:
			anim = BOTH_DEATH1;
			break;
		case 1:
			anim = BOTH_DEATH2;
			break;
		case 2:
		default:
			anim = BOTH_DEATH3;
			break;
		}

#ifdef TA_PLAYERSYS
		// Wait for death animation to end before respawning
		self->client->respawnTime = level.time + BG_AnimationTime(&self->client->pers.playercfg.animations[anim]);
#endif

#ifndef NOTRATEDM // No gibs.
		// for the no-blood option, we need to prevent the health
		// from going to gib level
		if ( self->health <= GIB_HEALTH ) {
			self->health = GIB_HEALTH+1;
		}
#endif

		self->client->ps.legsAnim = 
			( ( self->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
		self->client->ps.torsoAnim = 
			( ( self->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;

		G_AddEvent( self, EV_DEATH1 + i, killer );

		// the body can still be gibbed
		self->die = body_die;

		// globally cycle through the different death animations
		i = ( i + 1 ) % 3;

#if defined MISSIONPACK && !defined TURTLEARENA // NO_KAMIKAZE_ITEM
		if (self->s.eFlags & EF_KAMIKAZE) {
			Kamikaze_DeathTimer( self );
		}
#endif
	}

	trap_LinkEntity (self);

}


#ifndef TURTLEARENA // NOARMOR
/*
================
CheckArmor
================
*/
int CheckArmor (gentity_t *ent, int damage, int dflags)
{
	gclient_t	*client;
	int			save;
	int			count;

	if (!damage)
		return 0;

	client = ent->client;

	if (!client)
		return 0;

	if (dflags & DAMAGE_NO_ARMOR)
		return 0;

	// armor
	count = client->ps.stats[STAT_ARMOR];
	save = ceil( damage * ARMOR_PROTECTION );
	if (save >= count)
		save = count;

	if (!save)
		return 0;

	client->ps.stats[STAT_ARMOR] -= save;

	return save;
}
#endif

/*
================
RaySphereIntersections
================
*/
int RaySphereIntersections( vec3_t origin, float radius, vec3_t point, vec3_t dir, vec3_t intersections[2] ) {
	float b, c, d, t;

	//	| origin - (point + t * dir) | = radius
	//	a = dir[0]^2 + dir[1]^2 + dir[2]^2;
	//	b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	//	c = (point[0] - origin[0])^2 + (point[1] - origin[1])^2 + (point[2] - origin[2])^2 - radius^2;

	// normalize dir so a = 1
	VectorNormalize(dir);
	b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	c = (point[0] - origin[0]) * (point[0] - origin[0]) +
		(point[1] - origin[1]) * (point[1] - origin[1]) +
		(point[2] - origin[2]) * (point[2] - origin[2]) -
		radius * radius;

	d = b * b - 4 * c;
	if (d > 0) {
		t = (- b + sqrt(d)) / 2;
		VectorMA(point, t, dir, intersections[0]);
		t = (- b - sqrt(d)) / 2;
		VectorMA(point, t, dir, intersections[1]);
		return 2;
	}
	else if (d == 0) {
		t = (- b ) / 2;
		VectorMA(point, t, dir, intersections[0]);
		return 1;
	}
	return 0;
}

#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
/*
================
G_InvulnerabilityEffect
================
*/
int G_InvulnerabilityEffect( gentity_t *targ, vec3_t dir, vec3_t point, vec3_t impactpoint, vec3_t bouncedir ) {
	gentity_t	*impact;
	vec3_t		intersections[2], vec;
	int			n;

	if ( !targ->client ) {
		return qfalse;
	}
	VectorCopy(dir, vec);
	VectorInverse(vec);
	// sphere model radius = 42 units
	n = RaySphereIntersections( targ->client->ps.origin, 42, point, vec, intersections);
	if (n > 0) {
		impact = G_TempEntity( targ->client->ps.origin, EV_INVUL_IMPACT );
		VectorSubtract(intersections[0], targ->client->ps.origin, vec);
		vectoangles(vec, impact->s.angles);
		impact->s.angles[0] += 90;
		if (impact->s.angles[0] > 360)
			impact->s.angles[0] -= 360;
		if ( impactpoint ) {
			VectorCopy( intersections[0], impactpoint );
		}
		if ( bouncedir ) {
			VectorCopy( vec, bouncedir );
			VectorNormalize( bouncedir );
		}
		return qtrue;
	}
	else {
		return qfalse;
	}
}
#endif

#ifdef TA_ENTSYS // BREAKABLE
/*
=================
G_BreakableDebris

Spawn debris from breakable, the debris itself is not beakable.
=================
*/
void G_BreakableDebris( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point)
{
	int surfaceFlags;
	gentity_t   *tent;
	vec3_t      size;
 
 	VectorSubtract(targ->s.maxs, targ->s.mins, size);
 	VectorScale(size, 0.5, size);
 
	// point doesn't work with splash damage weapons?
	if (targ->health <= 0 || !dir || VectorLength(dir) == 0)
	{
		vec3_t      center;

#ifdef TA_MISC // MATERIALS
		surfaceFlags = targ->deathMaterial;
		if (surfaceFlags == 0) {
			return;
		}
#else
		surfaceFlags = -1;
#endif

		// Find the center of the brush
		VectorAdd(targ->s.mins, size, center);

		tent = G_TempEntity( center, EV_SPAWN_DEBRIS );
		tent->s.eventParm = 255;

		if (targ->health <= 0)
		{
			// Play explosion sound
			tent->s.generic1 = targ->noise_index;
		}

		tent->s.time2 = surfaceFlags;
	}
	else
	{
#ifdef TA_MISC // MATERIALS
		surfaceFlags = targ->damageMaterial;
		if (surfaceFlags == 0) {
			return;
		}
#else
		surfaceFlags = -1;
#endif

		tent = G_TempEntity( point, EV_SPAWN_DEBRIS );
		VectorInverse(dir);
		tent->s.eventParm = DirToByte(dir);
		tent->s.time2 = surfaceFlags;
	}

	tent->s.otherEntityNum2 = inflictor->s.number;

	// only use small size debris if not exploding
	if (targ->health > 0 && inflictor->s.weapon != WP_NONE)
	{
#ifdef TA_WEAPSYS
		if (inflictor->s.eType == ET_MISSILE)
			tent->s.otherEntityNum = bg_projectileinfo[inflictor->s.weapon].hitMarkRadius;
		else if (inflictor == attacker && inflictor->client) // Melee damage
			tent->s.otherEntityNum = bg_weapongroupinfo[inflictor->s.weapon].weapon[0]->impactMarkRadius;
#else
		// ZTM: TODO: Support non-TA_WEAPSYS weapons here?
#endif
	}

	if (!tent->s.otherEntityNum)
		tent->s.otherEntityNum = VectorLength(size);
}
#endif

/*
============
G_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

inflictor, attacker, dir, and point can be NULL for environmental effects

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/

qboolean G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
			   vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	gclient_t	*client;
	int			take;
#ifndef TURTLEARENA // NOARMOR
	int			asave;
#endif
	int			knockback;
	int			max;
#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
	vec3_t		bouncedir, impactpoint;
#endif

	if (!targ->takedamage) {
		return qfalse;
	}

	// the intermission has allready been qualified for, so don't
	// allow any extra scoring
	if ( level.intermissionQueued ) {
		return qfalse;
	}
#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
	if ( targ->client && mod != MOD_JUICED) {
		if ( targ->client->invulnerabilityTime > level.time) {
			if ( dir && point ) {
				G_InvulnerabilityEffect( targ, dir, point, impactpoint, bouncedir );
			}
			return qfalse;
		}
	}
#endif
#ifdef TA_WEAPSYS
	if (targ->mustcut && !(dflags & DAMAGE_CUTS))
	{
		// ZTM: TODO: Have a effect to user knows it is can only be broke by WIF_CUTS?
		//                       or have textures that users know can only be broke by WIF_CUTS?
		return qfalse;
	}
#endif
	if ( !inflictor ) {
		inflictor = &g_entities[ENTITYNUM_WORLD];
	}
	if ( !attacker ) {
		attacker = &g_entities[ENTITYNUM_WORLD];
	}

	// shootable doors / buttons don't actually have any health
	if ( targ->s.eType == ET_MOVER
#ifdef TA_ENTSYS // BREAKABLE
		&& targ->health == -1
#endif
	) {
		if ( targ->use && targ->moverState == MOVER_POS1 ) {
			targ->use( targ, inflictor, attacker );
		}
		return qtrue;
	}
#ifdef TA_ENTSYS // BREAKABLE
	// Allow breakable movers
	if ( targ->s.eType == ET_MOVER && targ->health > 0
		&& Q_stricmp(targ->classname, "func_voodoo"))
	{
		targ->health -= damage;
		if (targ->health < 0)
			targ->health = 0;

		G_BreakableDebris(targ, inflictor, attacker, dir, point);

		if (targ->health <= 0 && targ->die) {
			targ->die (targ, inflictor, attacker, damage, mod);
		}
		return qtrue;
	}
#endif

#ifdef MISSIONPACK
	if( g_gametype.integer == GT_OBELISK && CheckObeliskAttack( targ, attacker ) ) {
		return qfalse;
	}
#endif
	// reduce damage by the attacker's handicap value
	// unless they are rocket jumping
	if ( attacker->client && attacker != targ ) {
		max = attacker->client->ps.stats[STAT_MAX_HEALTH];
#ifdef MISSIONPACK
		if( BG_ItemForItemNum( attacker->client->ps.stats[STAT_PERSISTANT_POWERUP] )->giTag == PW_GUARD ) {
			max /= 2;
		}
#endif
		damage = damage * max / 100;
	}

	client = targ->client;

	if ( client ) {
#ifdef IOQ3ZTM
		if ( client->noclip && (targ->flags & FL_GODMODE) ) {
			return qfalse;
		}
#else
		if ( client->noclip ) {
			return qfalse;
		}
#endif
#ifdef TURTLEARENA // POWERS
		if ( client->ps.powerups[PW_FLASHING] && !(dflags & DAMAGE_NO_PROTECTION)) {
			return qfalse;
		}
#endif
	}

	if ( !dir ) {
		dflags |= DAMAGE_NO_KNOCKBACK;
	} else {
		VectorNormalize(dir);
	}

	knockback = damage;
	if ( knockback > 200 ) {
		knockback = 200;
	}
	if ( targ->flags & FL_NO_KNOCKBACK ) {
		knockback = 0;
	}
	if ( dflags & DAMAGE_NO_KNOCKBACK ) {
		knockback = 0;
	}

	// figure momentum add, even if the damage won't be taken
	if ( knockback && targ->client ) {
		vec3_t	kvel;
		float	mass;

#ifdef TURTLEARENA // WEAPONS
		mass = 100;
#else
		mass = 200;
#endif

		VectorScale (dir, g_knockback.value * (float)knockback / mass, kvel);
		VectorAdd (targ->client->ps.velocity, kvel, targ->client->ps.velocity);

		// set the timer so that the other client can't cancel
		// out the movement immediately
		if ( !targ->client->ps.pm_time ) {
			int		t;

			t = knockback * 2;
			if ( t < 50 ) {
				t = 50;
			}
			if ( t > 200 ) {
				t = 200;
			}
			targ->client->ps.pm_time = t;
			targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}
	}
#ifdef TA_ENTSYS // MISC_OBJECT
	else if (knockback)
	{
		vec3_t	kvel;
		float	mass;

		// Increase knockback with melee weapons
		if (inflictor == attacker) {
			knockback *= 2;
			if (knockback > 200) {
				knockback = 200;
			}
		}

#ifdef TURTLEARENA // WEAPONS
		mass = 100;
#else
		mass = 200;
#endif

		VectorScale (dir, g_knockback.value * (float)knockback / mass, kvel);
		kvel[2]  += g_knockback.value * (float)knockback / mass;// Fly into the air
		VectorAdd (targ->s.pos.trDelta, kvel, targ->s.pos.trDelta);
		targ->s.pos.trTime = level.time;

		if (targ->s.pos.trType == TR_STATIONARY) {
			targ->s.pos.trType = TR_GRAVITY;
		}

#if 0 // ZTM: TODO: Spin angles
		targ->s.apos.trType = TR_LINEAR_STOP;
		targ->s.apos.trTime = level.time;
		targ->s.angles[YAW] += 180.0f*crandom();
		VectorCopy(targ->s.angles, targ->s.apos.trDelta);
#endif
	}
#endif

	// check for completely getting out of the damage
	if ( !(dflags & DAMAGE_NO_PROTECTION) ) {

		// if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
		// if the attacker was on the same team
#ifdef MISSIONPACK
		if (
#ifndef TURTLEARENA // POWERS
			mod != MOD_JUICED &&
#endif
			targ != attacker && !(dflags & DAMAGE_NO_TEAM_PROTECTION) && OnSameTeam (targ, attacker)  )
#else	
		if ( targ != attacker && OnSameTeam (targ, attacker)  )
#endif
		{
			if ( !g_friendlyFire.integer ) {
				return qfalse;
			}
		}
#ifdef TA_WEAPSYS
		if (bg_projectileinfo[inflictor->s.weapon].explosionType == PE_PROX) {
			if (inflictor->parent && OnSameTeam(targ, inflictor->parent)) {
				return qfalse;
			}
			if (targ == attacker) {
				return qfalse;
			}
		}
#else
#ifdef MISSIONPACK
		if (mod == MOD_PROXIMITY_MINE) {
			if (inflictor && inflictor->parent && OnSameTeam(targ, inflictor->parent)) {
				return qfalse;
			}
			if (targ == attacker) {
				return qfalse;
			}
		}
#endif
#endif

#ifdef TURTLEARENA // POWERS
		// never take any damage.
		if ( client && client->ps.powerups[PW_INVUL] ) {
			G_AddEvent( targ, EV_POWERUP_INVUL, 0 );
			return qfalse;
		}
#endif

		// check for godmode
		if ( targ->flags & FL_GODMODE ) {
			return qfalse;
		}
	}

	// battlesuit protects from all radius damage (but takes knockback)
	// and protects 50% against all damage
	if ( client && client->ps.powerups[PW_BATTLESUIT] ) {
		G_AddEvent( targ, EV_POWERUP_BATTLESUIT, 0 );
#ifndef TURTLEARENA // POWERS
		if ( ( dflags & DAMAGE_RADIUS ) || ( mod == MOD_FALLING ) ) {
			return qfalse;
		}
#endif
		damage *= 0.5;
	}

	// add to the attacker's hit counter (if the target isn't a general entity like a prox mine)
	if ( attacker->client && client
			&& targ != attacker && targ->health > 0
			&& targ->s.eType != ET_MISSILE
			&& targ->s.eType != ET_GENERAL) {
		if ( OnSameTeam( targ, attacker ) ) {
			attacker->client->ps.persistant[PERS_HITS]--;
		} else {
			attacker->client->ps.persistant[PERS_HITS]++;
		}
#ifndef TURTLEARENA // NOARMOR
		attacker->client->ps.persistant[PERS_ATTACKEE_ARMOR] = (targ->health<<8)|(client->ps.stats[STAT_ARMOR]);
#endif
	}

	// always give half damage if hurting self
	// calculated after knockback, so rocket jumping works
	if ( targ == attacker) {
		damage *= 0.5;
	}

	if ( damage < 1 ) {
		damage = 1;
	}
	take = damage;

#ifndef TURTLEARENA // NOARMOR
	// save some from armor
	asave = CheckArmor (targ, take, dflags);
	take -= asave;
#endif

	if ( g_debugDamage.integer ) {
#ifdef TURTLEARENA // NOARMOR
		G_Printf( "%i: client:%i health:%i damage:%i\n", level.time, targ->s.number,
			targ->health, take );
#else
		G_Printf( "%i: client:%i health:%i damage:%i armor:%i\n", level.time, targ->s.number,
			targ->health, take, asave );
#endif
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if ( client ) {
		if ( attacker ) {
			client->ps.persistant[PERS_ATTACKER] = attacker->s.number;
		} else {
			client->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;
		}
#ifndef TURTLEARENA // NOARMOR
		client->damage_armor += asave;
#endif
		client->damage_blood += take;
		client->damage_knockback += knockback;
		if ( dir ) {
			VectorCopy ( dir, client->damage_from );
			client->damage_fromWorld = qfalse;
		} else {
			VectorCopy ( targ->r.currentOrigin, client->damage_from );
			client->damage_fromWorld = qtrue;
		}

#ifdef TURTLEARENA
		// Drop CTF flag (and other gametype items) if hit by missile or explosion.
		if (inflictor->s.eType == ET_MISSILE || (dflags & DAMAGE_RADIUS)) {
			TossClientGametypeItems(targ);
		}
#endif
	}

	// See if it's the player hurting the emeny flag carrier
#ifdef MISSIONPACK
	if( g_gametype.integer == GT_CTF || g_gametype.integer == GT_1FCTF ) {
#else	
	if( g_gametype.integer == GT_CTF) {
#endif
		Team_CheckHurtCarrier(targ, attacker);
	}

	if (targ->client) {
		// set the last client who damaged the target
		targ->client->lasthurt_client = attacker->s.number;
		targ->client->lasthurt_mod = mod;
#ifdef TA_WEAPSYS
		if (mod == MOD_PROJECTILE
			|| mod == MOD_PROJECTILE_EXPLOSION)
			targ->client->lasthurt_weapon = inflictor->s.weapon;
		else if (mod == MOD_WEAPON_PRIMARY
			|| mod == MOD_WEAPON_SECONDARY)
			targ->client->lasthurt_weapon = attacker->s.weapon;
		else
			targ->client->lasthurt_weapon = 0;
#endif
	}

	// do the damage
	if (take) {
#ifdef TA_ENTSYS // FUNC_VOODOO
		// Voodoo gives damage to others
		if (Q_stricmp(targ->classname, "func_voodoo") == 0) {
			G_BreakableDebris(targ, inflictor, attacker, dir, point);

			if (targ->die) {
				targ->die (targ, inflictor, attacker, take, mod);
			}
		} else {
#endif
		targ->health = targ->health - take;
		if ( targ->client ) {
			targ->client->ps.stats[STAT_HEALTH] = targ->health;
		}
			
		if ( targ->health <= 0 ) {
			if ( client )
				targ->flags |= FL_NO_KNOCKBACK;

			if (targ->health < -999)
				targ->health = -999;

			targ->enemy = attacker;
			targ->die (targ, inflictor, attacker, take, mod);
		} else if ( targ->pain ) {
			targ->pain (targ, attacker, take);
		}
#ifdef TA_ENTSYS // FUNC_VOODOO
		}
#endif

#ifdef TA_ENTSYS
		// Trigger entity's paintarget
		if (targ->health > 0 && !targ->client && targ->paintarget && level.time > targ->pain_debounce_time) {
			G_UseTargets2(targ, attacker, targ->paintarget);
			// Limit how soon to call paintarget again
			targ->pain_debounce_time = level.time+100; // ZTM: TODO: Set in entity
		}
#endif
		return qtrue;
	}

	return qfalse;
}


/*
============
CanDamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage (gentity_t *targ, vec3_t origin) {
	vec3_t	dest;
	trace_t	tr;
	vec3_t	midpoint;
	vec3_t	offsetmins = {-15, -15, -15};
	vec3_t	offsetmaxs = {15, 15, 15};

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	VectorAdd (targ->r.absmin, targ->r.absmax, midpoint);
	VectorScale (midpoint, 0.5, midpoint);

	VectorCopy(midpoint, dest);
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;

	// this should probably check in the plane of projection, 
	// rather than in world coordinate
	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmins[2];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	return qfalse;
}


/*
============
G_RadiusDamage
============
*/
#ifdef TA_WEAPSYS
qboolean G_RadiusDamage ( vec3_t origin, gentity_t *inflictor, gentity_t *attacker, float damage,
					float radius, gentity_t *ignore, int mod)
#else
qboolean G_RadiusDamage ( vec3_t origin, gentity_t *attacker, float damage, float radius,
					 gentity_t *ignore, int mod)
#endif
{
	float		points, dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;
	qboolean	hitClient = qfalse;

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

		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

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

		points = damage * ( 1.0 - dist / radius );

		if( CanDamage (ent, origin) ) {
			if( LogAccuracyHit( ent, attacker ) ) {
				hitClient = qtrue;
			}
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;
#ifdef TA_WEAPSYS
			G_Damage (ent, inflictor, attacker, dir, origin, (int)points, DAMAGE_RADIUS, mod);
#else
			G_Damage (ent, NULL, attacker, dir, origin, (int)points, DAMAGE_RADIUS, mod);
#endif
		}
	}

	return hitClient;
}
