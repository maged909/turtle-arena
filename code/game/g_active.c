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
===============
G_DamageFeedback

Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the player_state_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
===============
*/
void P_DamageFeedback( gentity_t *player ) {
	gclient_t	*client;
	float	count;
	vec3_t	angles;

#ifdef IOQ3ZTM // LASERTAG
	if (g_laserTag.integer) {
		return;
	}
#endif

	client = player->client;
	if ( client->ps.pm_type == PM_DEAD ) {
		return;
	}

	// total points of damage shot at the player this frame
#ifdef TURTLEARENA // NOARMOR
	count = client->damage_blood;
#else
	count = client->damage_blood + client->damage_armor;
#endif
	if ( count == 0 ) {
		return;		// didn't take any damage
	}

	if ( count > 255 ) {
		count = 255;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if ( client->damage_fromWorld ) {
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->damage_fromWorld = qfalse;
	} else {
		vectoangles( client->damage_from, angles );
		client->ps.damagePitch = angles[PITCH]/360.0 * 256;
		client->ps.damageYaw = angles[YAW]/360.0 * 256;
	}

	// play an apropriate pain sound
	if ( (level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) ) {
		player->pain_debounce_time = level.time + 700;
		G_AddEvent( player, EV_PAIN, player->health );
		client->ps.damageEvent++;
	}


	client->ps.damageCount = count;

	//
	// clear totals
	//
	client->damage_blood = 0;
#ifndef TURTLEARENA // NOARMOR
	client->damage_armor = 0;
#endif
	client->damage_knockback = 0;
}



/*
=============
P_WorldEffects

Check for lava / slime contents and drowning
=============
*/
void P_WorldEffects( gentity_t *ent ) {
#ifndef TURTLEARENA // POWERS
	qboolean	envirosuit;
#endif
	int			waterlevel;

#ifdef IOQ3ZTM // Drowned in noclip, but not godmode.
	if ( ent->flags & FL_GODMODE )
#else
	if ( ent->client->noclip )
#endif
	{
#ifdef TURTLEARENA // DROWNING
		ent->client->ps.powerups[PW_AIR] = level.time + 31000;	// don't need air
#else
		ent->client->airOutTime = level.time + 12000;	// don't need air
#endif
		return;
	}

	waterlevel = ent->waterlevel;

#ifndef TURTLEARENA // POWERS
	envirosuit = ent->client->ps.powerups[PW_BATTLESUIT] > level.time;
#endif

	//
	// check for drowning
	//
	if ( waterlevel == 3 ) {
#ifndef TURTLEARENA // POWERS
		// envirosuit give air
		if ( envirosuit ) {
			ent->client->airOutTime = level.time + 10000;
		}
#endif

		// if out of air, start drowning
#ifdef TURTLEARENA // DROWNING
		if (ent->client->ps.powerups[PW_AIR] < level.time)
#else
		if ( ent->client->airOutTime < level.time)
#endif
		{
			// drown!
#ifndef TURTLEARENA // DROWNING
			ent->client->airOutTime += 1000;
#endif
			if ( ent->health > 0 ) {
#ifdef TURTLEARENA // DROWNING
				// don't play a normal pain sound
				ent->pain_debounce_time = level.time + 200;

				G_Damage (ent, NULL, NULL, NULL, NULL, 
					10000, DAMAGE_NO_ARMOR, MOD_WATER);
#else
				// take more damage the longer underwater
				ent->damage += 2;
				if (ent->damage > 15)
					ent->damage = 15;

				// don't play a normal pain sound
				ent->pain_debounce_time = level.time + 200;

				G_Damage (ent, NULL, NULL, NULL, NULL, 
					ent->damage, DAMAGE_NO_ARMOR, MOD_WATER);
#endif
			}
		}
#ifdef TURTLEARENA // DROWNING
		// Low air warning
		else if (ent->client->ps.powerups[PW_AIR] < level.time + 5000 && !(ent->flags & FL_DROWNING_WARNING)) {
			ent->flags |= FL_DROWNING_WARNING;

			// don't play a normal pain sound
			ent->pain_debounce_time = level.time + 200;

			G_Damage (ent, NULL, NULL, NULL, NULL, 
				2, DAMAGE_NO_ARMOR, MOD_WATER);
		}
#endif
	} else {
#ifdef TURTLEARENA // DROWNING
		ent->flags &= ~FL_DROWNING_WARNING;
		if (ent->client->ps.powerups[PW_AIR]+1000 < level.time + 31000) {
			ent->client->ps.powerups[PW_AIR] += 1000;
		} else {
			ent->client->ps.powerups[PW_AIR] = level.time + 31000;
		}
#else
		ent->client->airOutTime = level.time + 12000;
#endif
		ent->damage = 2;
	}

	//
	// check for sizzle damage (move to pmove?)
	//
	if (waterlevel && 
		(ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		if (ent->health > 0
			&& ent->pain_debounce_time <= level.time	) {

#ifdef TURTLEARENA // POWERS
			if ( ent->client->ps.powerups[PW_INVUL] > level.time ) {
				G_AddEvent( ent, EV_POWERUP_INVUL, 0 );
			} else
#else
			if ( envirosuit ) {
				G_AddEvent( ent, EV_POWERUP_BATTLESUIT, 0 );
			} else
#endif
			{
				if (ent->watertype & CONTENTS_LAVA) {
					G_Damage (ent, NULL, NULL, NULL, NULL, 
						30*waterlevel, 0, MOD_LAVA);
				}

				if (ent->watertype & CONTENTS_SLIME) {
					G_Damage (ent, NULL, NULL, NULL, NULL, 
						10*waterlevel, 0, MOD_SLIME);
				}
			}
		}
	}
}



/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound( gentity_t *ent ) {
#if defined MISSIONPACK && !defined TURTLEARENA // WEAPONS
#ifdef TA_WEAPSYS // Per-projectile tick sound name
	if ((ent->s.eFlags & EF_TICKING) && ent->activator
		&& ent->activator->s.weapon > 0 && ent->activator->s.weapon < BG_NumProjectiles()
		&& bg_projectileinfo[ent->activator->s.weapon].explosionType == PE_PROX
		&& bg_projectileinfo[ent->activator->s.weapon].tickSoundName[0])
	{
		ent->client->ps.loopSound = G_SoundIndex(bg_projectileinfo[ent->activator->s.weapon].tickSoundName);
	}
#else
	if( ent->s.eFlags & EF_TICKING ) {
		ent->client->ps.loopSound = G_SoundIndex( "sound/weapons/proxmine/wstbtick.wav");
	}
#endif
	else
#endif
	if (ent->waterlevel && (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		ent->client->ps.loopSound = level.snd_fry;
	} else {
		ent->client->ps.loopSound = 0;
	}
}



//==============================================================

/*
==============
ClientImpacts
==============
*/
void ClientImpacts( gentity_t *ent, pmove_t *pm ) {
	int		i, j;
	trace_t	trace;
	gentity_t	*other;

	memset( &trace, 0, sizeof( trace ) );
	for (i=0 ; i<pm->numtouch ; i++) {
		for (j=0 ; j<i ; j++) {
			if (pm->touchents[j] == pm->touchents[i] ) {
				break;
			}
		}
		if (j != i) {
			continue;	// duplicated
		}
		other = &g_entities[ pm->touchents[i] ];

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, other, &trace );
		}

		if ( !other->touch ) {
			continue;
		}

		other->touch( other, ent, &trace );
	}

}

/*
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void	G_TouchTriggers( gentity_t *ent ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	trace_t		trace;
	vec3_t		mins, maxs;
	static vec3_t	range = { 40, 40, 52 };

	if ( !ent->client ) {
		return;
	}

	// dead clients don't activate triggers!
	if ( ent->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	VectorSubtract( ent->client->ps.origin, range, mins );
	VectorAdd( ent->client->ps.origin, range, maxs );

	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	// can't use ent->absmin, because that has a one unit pad
	VectorAdd( ent->client->ps.origin, ent->s.mins, mins );
	VectorAdd( ent->client->ps.origin, ent->s.maxs, maxs );

	for ( i=0 ; i<num ; i++ ) {
		hit = &g_entities[touch[i]];

		if ( !hit->touch && !ent->touch ) {
			continue;
		}
		if ( !( hit->s.contents & CONTENTS_TRIGGER ) ) {
			continue;
		}

		// ignore most entities if a spectator
		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			if ( hit->s.eType != ET_TELEPORT_TRIGGER &&
				// this is ugly but adding a new ET_? type will
				// most likely cause network incompatibilities
				hit->touch != Touch_DoorTrigger) {
				continue;
			}
		}

		// use seperate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		if ( hit->s.eType == ET_ITEM ) {
			if ( !BG_PlayerTouchesItem( &ent->client->ps, &hit->s, level.time ) ) {
				continue;
			}
		} else {
			if ( !trap_EntityContact( mins, maxs, hit ) ) {
				continue;
			}
		}

		memset( &trace, 0, sizeof(trace) );

		if ( hit->touch ) {
			hit->touch (hit, ent, &trace);
		}

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, hit, &trace );
		}
	}

	// if we didn't touch a jump pad this pmove frame
	if ( ent->client->ps.jumppad_frame != ent->client->ps.pmove_framecount ) {
		ent->client->ps.jumppad_frame = 0;
		ent->client->ps.jumppad_ent = 0;
	}
}

/*
=================
SpectatorThink
=================
*/
void SpectatorThink( gentity_t *ent, usercmd_t *ucmd ) {
	pmove_t	pm;
	gclient_t	*client;

	client = ent->client;

	if ( client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		client->ps.pm_type = PM_SPECTATOR;
		client->ps.speed = 400;	// faster than normal

		// set up for pmove
		memset (&pm, 0, sizeof(pm));
		pm.ps = &client->ps;
#ifdef TA_PLAYERSYS // Pmove
		pm.playercfg = &client->pers.playercfg;
#endif
		pm.cmd = *ucmd;
#ifdef TURTLEARENA // NO_BODY_TRACE
		pm.tracemask = MASK_PLAYERSOLID;
#else
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;	// spectators can fly through bodies
#endif
		if (client->ps.capsule) {
			pm.trace = trap_TraceCapsule;
		} else {
			pm.trace = trap_Trace;
		}
		pm.pointcontents = trap_PointContents;

		// perform a pmove
		Pmove (&pm);
		// save results of pmove
		VectorCopy( client->ps.origin, ent->s.origin );

		G_TouchTriggers( ent );
		trap_UnlinkEntity( ent );
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;

	// attack button cycles through spectators
	if ( ( client->buttons & BUTTON_ATTACK ) && ! ( client->oldbuttons & BUTTON_ATTACK ) ) {
		Cmd_FollowCycle_f( ent, 1 );
	}
#ifdef IOQ3ZTM // SPECTATOR
	else if ( ( client->buttons & BUTTON_USE_HOLDABLE ) && ! ( client->oldbuttons & BUTTON_USE_HOLDABLE ) ) {
		Cmd_FollowCycle_f( ent, -1 );
	}
	// Stop following client when jump or crouch is pressed.
	else if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW && ucmd->upmove != 0 ) {
		StopFollowing(ent);
	}
#endif
}



/*
=================
ClientInactivityTimer

Returns qfalse if the client is dropped
=================
*/
qboolean ClientInactivityTimer( gclient_t *client ) {
	if ( ! g_inactivity.integer ) {
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		client->inactivityTime = level.time + 60 * 1000;
		client->inactivityWarning = qfalse;
	} else if ( client->pers.cmd.forwardmove || 
		client->pers.cmd.rightmove || 
		client->pers.cmd.upmove ||
#ifdef IOQ3ZTM
		(client->pers.cmd.buttons & BUTTON_USE_HOLDABLE) ||
#endif
		(client->pers.cmd.buttons & BUTTON_ATTACK) ) {
		client->inactivityTime = level.time + g_inactivity.integer * 1000;
		client->inactivityWarning = qfalse;
	} else if ( !client->pers.localClient ) {
		if ( level.time > client->inactivityTime ) {
			trap_DropClient( client - level.clients, "Dropped due to inactivity" );
			return qfalse;
		}
		if ( level.time > client->inactivityTime - 10000 && !client->inactivityWarning ) {
			client->inactivityWarning = qtrue;
			trap_SendServerCommand( client - level.clients, "cp \"Ten seconds until inactivity drop!\n\"" );
		}
	}
	return qtrue;
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
void ClientTimerActions( gentity_t *ent, int msec ) {
	gclient_t	*client;
#ifndef TURTLEARENA // POWERS
#ifdef MISSIONPACK
	int			maxHealth;
#endif
#endif

	client = ent->client;
	client->timeResidual += msec;

#ifdef TA_WEAPSYS // MELEEATTACK
	if (client->ps.meleeLinkTime > 0)
	{
		client->ps.meleeLinkTime -= msec;
		if (client->ps.meleeLinkTime <= 0)
		{
			client->ps.meleeLinkTime = 0;
			client->ps.meleeAttack = -1;
		}
	}

	if (client->ps.meleeDelay > 0)
	{
		client->ps.meleeDelay -= msec;
		if (client->ps.meleeDelay < 0)
		{
			client->ps.meleeDelay = 0;
		}
	}

	if (client->ps.meleeTime > 0)
	{
		client->ps.meleeTime -= msec;
		if (client->ps.meleeTime <= 0)
		{
			int weap_delay;
			int max_combo;

			max_combo = BG_MaxAttackIndex(&client->ps);
			weap_delay = bg_weapongroupinfo[client->ps.weapon].weapon[0]->attackDelay;

			// ZTM: TEST; with the accel changes (xyspeed) weap_delay is way too long.
			weap_delay /= 2;

			if (client->ps.groundEntityNum == ENTITYNUM_NONE) {
				// Jump attack only uses last attack animation
				max_combo = 1;
				weap_delay = 0;
			} else if (BG_PlayerRunning(client->ps.velocity)) {
				// ZTM: if running have no attack delay and only use first half of animations, like in LoZ:TP
				max_combo /= 2;
				weap_delay = 0;
			}

			client->ps.meleeTime = 0;

			if (client->ps.meleeAttack == max_combo-1)
			{
				//G_Printf("DEBUG: client %i finished last combo (%i)\n", ent - g_entities, client->ps.meleeAttack);

				client->ps.meleeDelay = weap_delay;
				client->ps.weaponTime = client->ps.meleeDelay; // Don't let them use a gun...

				client->ps.meleeAttack = -1;
				client->ps.meleeLinkTime = 0;
			}
		}
	}

	if (client->ps.chainTime > 0)
	{
		client->ps.chainTime -= msec;
		if (client->ps.chainTime <= 0)
		{
			client->ps.chainTime = 0;
			client->ps.chain = 0;
		}
	}
#endif

	while ( client->timeResidual >= 1000 ) {
		client->timeResidual -= 1000;

#ifndef TURTLEARENA // POWERS // Guards having health regen makes them pretty much unkillable, and removed regen powerup.
		// regenerate
#ifdef MISSIONPACK
#ifdef TA_ITEMSYS
		if( BG_ItemForItemNum(client->ps.stats[STAT_PERSISTANT_POWERUP])->giTag == PW_GUARD )
#else
		if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD )
#endif
		{
			maxHealth = client->ps.stats[STAT_MAX_HEALTH] / 2;
		}
		else if ( client->ps.powerups[PW_REGEN] ) {
			maxHealth = client->ps.stats[STAT_MAX_HEALTH];
		}
		else {
			maxHealth = 0;
		}
		if( maxHealth ) {
			if ( ent->health < maxHealth ) {
				ent->health += 15;
				if ( ent->health > maxHealth * 1.1 ) {
					ent->health = maxHealth * 1.1;
				}
				G_AddEvent( ent, EV_POWERUP_REGEN, 0 );
			} else if ( ent->health < maxHealth * 2) {
				ent->health += 5;
				if ( ent->health > maxHealth * 2 ) {
					ent->health = maxHealth * 2;
				}
				G_AddEvent( ent, EV_POWERUP_REGEN, 0 );
			}
#else
		if ( client->ps.powerups[PW_REGEN] ) {
			if ( ent->health < client->ps.stats[STAT_MAX_HEALTH]) {
				ent->health += 15;
				if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] * 1.1 ) {
					ent->health = client->ps.stats[STAT_MAX_HEALTH] * 1.1;
				}
				G_AddEvent( ent, EV_POWERUP_REGEN, 0 );
			} else if ( ent->health < client->ps.stats[STAT_MAX_HEALTH] * 2) {
				ent->health += 5;
				if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] * 2 ) {
					ent->health = client->ps.stats[STAT_MAX_HEALTH] * 2;
				}
				G_AddEvent( ent, EV_POWERUP_REGEN, 0 );
			}
#endif
		} else
#endif
		{
			// count down health when over max
			if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] ) {
				ent->health--;
			}
		}

#ifndef TURTLEARENA // NOARMOR
		// count down armor when over max
		if ( client->ps.stats[STAT_ARMOR] > client->ps.stats[STAT_MAX_HEALTH] ) {
			client->ps.stats[STAT_ARMOR]--;
		}
#endif
	}
#ifdef MISSIONPACK
#ifdef TA_ITEMSYS
	if( BG_ItemForItemNum(client->ps.stats[STAT_PERSISTANT_POWERUP])->giTag == PW_AMMOREGEN )
#else
	if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_AMMOREGEN )
#endif
	{
#ifdef TA_WEAPSYS_EX
		int w, max, inc, t;

		// Ony gives ammo for current weapon
		w = client->ps.weapon;
#else
		int w, max, inc, t, i;
#ifdef TA_WEAPSYS
		int weapList[MAX_BG_WEAPON_GROUPS];
		int weapCount = 0;
		max = BG_NumWeaponGroups();
		for (i = 1; i < max; i++) {
			if (BG_WeapUseAmmo(i))
			{
				weapList[weapCount] = i;
				weapCount++;
			}
		}
#else
		int weapList[]={WP_MACHINEGUN,WP_SHOTGUN,WP_GRENADE_LAUNCHER,WP_ROCKET_LAUNCHER,WP_LIGHTNING,WP_RAILGUN,WP_PLASMAGUN,WP_BFG,WP_NAILGUN,WP_PROX_LAUNCHER,WP_CHAINGUN};
		int weapCount = ARRAY_LEN( weapList );
#endif
		//
		for (i = 0; i < weapCount; i++) {
			w = weapList[i];
#endif

#ifdef TA_WEAPSYS // ZTM: TODO: Make ammo regen less-hacky, have it make sense.
			// NOTE: max is wrong for WP_BFG: max should be 10 instead of 20
			//                        WP_MACHINEGUN: max should be 50 instead of 40
			//       inc is wrong for WP_MACHINEGUN: inc should be 4 instead of 5
			max = bg_weapongroupinfo[w].item->quantity;

			if (max <= 25)
				inc = 1;
			else
				inc = 5;

#if 0 // Less hacky
			if (inc > 1) {
				t = (bg_weapongroupinfo[w].weapon[0]->attackDelay * inc) * 2;
			} else {
				t = bg_weapongroupinfo[w].weapon[0]->attackDelay;
			}

			if (t < 400) {
				t += t * 0.01f;
			}

			t += bg_weapongroupinfo[w].weapon[0]->proj->damage * 0.01f;
#else
			// Ugly, but supports; machinegun, shotgun, lightning. railgun, plasma
			// rocket is close to Q3, 50 msec longer
			// grenade is 200 msec faster
			// bfg is 1000 msec faster...
			if (inc > 1) {
				t = (bg_weapongroupinfo[w].weapon[0]->attackDelay * inc) * 2;
			} else {
				// 1.166666667f is a hack for railgun
				t = bg_weapongroupinfo[w].weapon[0]->attackDelay * 1.166666667f;
			}

			// shotgun/plasmagun hack, other wise would be 1000.
			if (t == 1000 && bg_weapongroupinfo[w].weapon[0]->proj->damage >= 10) {
				t = 1500;
			}

			if (t < 1000) {
				if (bg_weapongroupinfo[w].weapon[0]->proj->instantDamage) {
					t += 1000;
				} else {
					t += bg_weapongroupinfo[w].weapon[0]->proj->speed;
				}
			}
#endif

			t = Com_Clamp(1000, 4000, t);
#else
			switch(w) {
				case WP_MACHINEGUN: max = 50; inc = 4; t = 1000; break;
				case WP_SHOTGUN: max = 10; inc = 1; t = 1500; break;
				case WP_GRENADE_LAUNCHER: max = 10; inc = 1; t = 2000; break;
				case WP_ROCKET_LAUNCHER: max = 10; inc = 1; t = 1750; break;
				case WP_LIGHTNING: max = 50; inc = 5; t = 1500; break;
				case WP_RAILGUN: max = 10; inc = 1; t = 1750; break;
				case WP_PLASMAGUN: max = 50; inc = 5; t = 1500; break;
				case WP_BFG: max = 10; inc = 1; t = 4000; break;
				case WP_NAILGUN: max = 10; inc = 1; t = 1250; break;
				case WP_PROX_LAUNCHER: max = 5; inc = 1; t = 2000; break;
				case WP_CHAINGUN: max = 100; inc = 5; t = 1000; break;
				default: max = 0; inc = 0; t = 1000; break;
			}
#endif
#ifdef TA_WEAPSYS_EX
			if (max > 0)
			{
				client->ammoTimes[w] += msec;
				if ( client->ps.stats[STAT_AMMO] >= max ) {
					client->ammoTimes[w] = 0;
				}
				if ( client->ammoTimes[w] >= t ) {
					while ( client->ammoTimes[w] >= t )
						client->ammoTimes[w] -= t;
					client->ps.stats[STAT_AMMO] += inc;
					if ( client->ps.stats[STAT_AMMO] > max ) {
						client->ps.stats[STAT_AMMO] = max;
					}
				}
			}
#else
#ifdef TA_WEAPSYS
			if (max > 0)
			{
#endif
				client->ammoTimes[w] += msec;
				if ( client->ps.ammo[w] >= max ) {
					client->ammoTimes[w] = 0;
				}
				if ( client->ammoTimes[w] >= t ) {
					while ( client->ammoTimes[w] >= t )
						client->ammoTimes[w] -= t;
					client->ps.ammo[w] += inc;
					if ( client->ps.ammo[w] > max ) {
						client->ps.ammo[w] = max;
					}
				}
#ifdef TA_WEAPSYS
			}
#endif
    }
#endif
#ifdef TURTLEARENA // REGEN_SHURIKENS
		// Shuriken regen
		{
			int h;

			for (h = 0; h < MAX_HOLDABLE; h++) {
				if (!BG_ProjectileIndexForHoldable(h)) {
					continue;
				}
				switch(h) {
					// Only weapons that use ammo.
					case HI_SHURIKEN: max = 20; inc = 1; t = 1500; break;
					case HI_ELECTRICSHURIKEN: max = 20; inc = 1; t = 2000; break;
					case HI_FIRESHURIKEN: max = 20; inc = 1; t = 3000; break;
					case HI_LASERSHURIKEN: max = 20; inc = 1; t = 2500; break;
					default: max = 20; inc = 1; t = 2000; break;
				}
				client->holdableTimes[h] += msec;
				if ( client->ps.holdable[h] >= max ) {
				client->holdableTimes[h] = 0;
				}
				if ( client->holdableTimes[h] >= t ) {
					while ( client->holdableTimes[h] >= t )
					  client->holdableTimes[h] -= t;
					client->ps.holdable[h] += inc;
					if ( client->ps.holdable[h] > max ) {
					  client->ps.holdable[h] = max;
					}
					// If no holdable is selected auto select this holdable
					if (inc > 0 && client->ps.holdableIndex == HI_NONE)
					{
						client->ps.holdableIndex = h;
					}
				}
			}
		}
#endif
	}
#endif
}

/*
====================
ClientIntermissionThink
====================
*/
void ClientIntermissionThink( gclient_t *client ) {
	client->ps.eFlags &= ~EF_TALK;
	client->ps.eFlags &= ~EF_FIRING;

	// the level will exit when everyone wants to or after timeouts

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->pers.cmd.buttons;
#ifdef TA_SP
	if (g_gametype.integer == GT_SINGLE_PLAYER)
	{
		client->readyToExit = 1;
	}
	else
#endif
	if ( client->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) & ( client->oldbuttons ^ client->buttons ) ) {
		// this used to be an ^1 but once a player says ready, it should stick
		client->readyToExit = 1;
	}
}


/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
void ClientEvents( gentity_t *ent, int oldEventSequence ) {
	int		i;
	int		event;
	gclient_t *client;
	int		damage;
#ifndef TURTLEARENA // HOLDABLE // no q3 teleporter
	vec3_t	origin, angles;
#endif
//	qboolean	fired;
#ifdef TA_WEAPSYS_EX
	gitem_t *item;
	gentity_t *drop;
#endif
#ifdef TA_HOLDSYS
	int itemNum;
#endif

	client = ent->client;

	if ( oldEventSequence < client->ps.eventSequence - MAX_PS_EVENTS ) {
		oldEventSequence = client->ps.eventSequence - MAX_PS_EVENTS;
	}
	for ( i = oldEventSequence ; i < client->ps.eventSequence ; i++ ) {
		event = client->ps.events[ i & (MAX_PS_EVENTS-1) ];

		switch ( event ) {
		case EV_FALL_MEDIUM:
		case EV_FALL_FAR:
			if ( ent->s.eType != ET_PLAYER ) {
				break;		// not in the player model
			}
			if ( g_dmflags.integer & DF_NO_FALLING ) {
				break;
			}
			if ( event == EV_FALL_FAR ) {
				damage = 10;
			} else {
				damage = 5;
			}
			ent->pain_debounce_time = level.time + 200;	// no normal pain sound
			G_Damage (ent, NULL, NULL, NULL, NULL, damage, 0, MOD_FALLING);
			break;

#ifdef TA_WEAPSYS_EX
		case EV_DROP_WEAPON:
			if (ent->client)
			{
				int weapon;
				int ammo;
				int contents;

				// Get drop info
				weapon = ent->client->ps.stats[STAT_DROP_WEAPON];
				ammo = ent->client->ps.stats[STAT_DROP_AMMO];

				// Clear drop info
				ent->client->ps.stats[STAT_DROP_WEAPON] = WP_NONE;
				ent->client->ps.stats[STAT_DROP_AMMO] = 0;

				if (weapon <= WP_NONE) {
					break;
				}

				if (ammo == 0)
				{
					// Gun with no ammo. if someone picks it up, they will get default ammo.
					// ZTM: TODO: Throw the weapon and have it fade alpha? (and can't pickup)
					break;
				}

				// Don't drop weapon in no drop areas.
				contents = trap_PointContents( ent->r.currentOrigin, -1 );
				if ( ( contents & CONTENTS_NODROP )) {
					break;
				}

				// find the item type for this weapon
				item = BG_FindItemForWeapon(weapon);

				if (item) {
					drop = Drop_Item(ent, item, 0);
					drop->count = ammo;

					// Override weapon removal time.
					drop->nextthink = level.time + 15000;
#ifdef IOQ3ZTM // ITEMS_DISAPPEAR
					drop->s.frame = 15000;
#endif
				}
			}
			break;
#endif

		case EV_FIRE_WEAPON:
			FireWeapon( ent );
			break;

#ifdef TA_HOLDSYS // HI_* is not hooked to EV_USE_ITEM in game now.
		case EV_USE_ITEM0:
		case EV_USE_ITEM1:
		case EV_USE_ITEM2:
		case EV_USE_ITEM3:
		case EV_USE_ITEM4:
		case EV_USE_ITEM5:
		case EV_USE_ITEM6:
		case EV_USE_ITEM7:
		case EV_USE_ITEM8:
		case EV_USE_ITEM9:
		case EV_USE_ITEM10:
		case EV_USE_ITEM11:
		case EV_USE_ITEM12:
		case EV_USE_ITEM13:
		case EV_USE_ITEM14:
		case EV_USE_ITEM15:
		{
			itemNum = (event & ~EV_EVENT_BITS) - EV_USE_ITEM0;
#ifdef TURTLEARENA // HOLD_SHURIKEN
			G_ThrowShuriken(ent, itemNum);
#endif
			switch (itemNum)
			{
				case HI_NONE:
					break;
#endif // TA_HOLDSYS

#ifndef TURTLEARENA // HOLDABLE // no q3 teleprter
#ifdef TA_HOLDSYS
				case HI_TELEPORTER:
#else
		case EV_USE_ITEM1:		// teleporter
#endif // TA_HOLDSYS
			TossClientGametypeItems(ent);
#ifdef TA_PLAYERSYS
			SelectSpawnPoint( ent, origin, angles, qfalse );
#else
			SelectSpawnPoint( ent->client->ps.origin, origin, angles, qfalse );
#endif
			TeleportPlayer( ent, origin, angles );
			break;
#endif

#ifdef TA_HOLDSYS
				case HI_MEDKIT:
#else
		case EV_USE_ITEM2:		// medkit
#endif
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH] + 25;

			break;

#ifdef MISSIONPACK
#ifndef TURTLEARENA // POWERS NO_KAMIKAZE_ITEM
#ifdef TA_HOLDSYS
				case HI_KAMIKAZE:
#else
		case EV_USE_ITEM3:		// kamikaze
#endif
			// make sure the invulnerability is off
			ent->client->invulnerabilityTime = 0;
			// start the kamikze
			G_StartKamikaze( ent );
			break;
#endif

#ifdef TA_HOLDSYS
				case HI_PORTAL:
#else
		case EV_USE_ITEM4:		// portal
#endif
			if( ent->client->portalID ) {
				DropPortalSource( ent );
			}
			else {
				DropPortalDestination( ent );
			}
			break;

#ifndef TURTLEARENA // POWERS
#ifdef TA_HOLDSYS
				case HI_INVULNERABILITY:
#else
		case EV_USE_ITEM5:		// invulnerability
#endif
			ent->client->invulnerabilityTime = level.time + 10000;
			break;
#endif // TURTLEARENA // POWERS

#endif // MISSIONPACK

#ifdef TA_HOLDSYS
				default:
#ifdef TURTLEARENA // HOLDABLE
					if (!BG_ProjectileIndexForHoldable(itemNum))
#endif
						G_Printf("  EV_USE_ITEM: No code for holdable %d.\n", itemNum);
					break;
			}
		}
#endif

		default:
			break;
		}
	}

}

#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
/*
==============
StuckInOtherClient
==============
*/
static int StuckInOtherClient(gentity_t *ent) {
	int i;
	gentity_t	*ent2;

	ent2 = &g_entities[0];
	for ( i = 0; i < MAX_CLIENTS; i++, ent2++ ) {
		if ( ent2 == ent ) {
			continue;
		}
		if ( !ent2->inuse ) {
			continue;
		}
		if ( !ent2->client ) {
			continue;
		}
		if ( ent2->health <= 0 ) {
			continue;
		}
		//
		if (ent2->r.absmin[0] > ent->r.absmax[0])
			continue;
		if (ent2->r.absmin[1] > ent->r.absmax[1])
			continue;
		if (ent2->r.absmin[2] > ent->r.absmax[2])
			continue;
		if (ent2->r.absmax[0] < ent->r.absmin[0])
			continue;
		if (ent2->r.absmax[1] < ent->r.absmin[1])
			continue;
		if (ent2->r.absmax[2] < ent->r.absmin[2])
			continue;
		return qtrue;
	}
	return qfalse;
}
#endif

void BotTestSolid(vec3_t origin);

/*
==============
SendPendingPredictableEvents
==============
*/
void SendPendingPredictableEvents( playerState_t *ps ) {
	gentity_t *t;
	int event, seq;
	int extEvent, number;

	// if there are still events pending
	if ( ps->entityEventSequence < ps->eventSequence ) {
		// create a temporary entity for this event which is sent to everyone
		// except the client who generated the event
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		// set external event to zero before calling BG_PlayerStateToEntityState
		extEvent = ps->externalEvent;
		ps->externalEvent = 0;
		// create temporary entity for event
		t = G_TempEntity( ps->origin, event );
		number = t->s.number;
		BG_PlayerStateToEntityState( ps, &t->s, qtrue );
		t->s.number = number;
		t->s.eType = ET_EVENTS + event;
		t->s.eFlags |= EF_PLAYER_EVENT;
		t->s.otherEntityNum = ps->clientNum;
		t->s.contents = 0;
		// send to everyone except the client who generated the event
		t->r.svFlags |= SVF_NOTSINGLECLIENT;
		t->r.singleClient = ps->clientNum;
		// set back external event
		ps->externalEvent = extEvent;
	}
}



#ifdef TA_WEAPSYS
// Switch to onhanded if by a CTF flag that can be picked up,
//    so that its like the "player" is planning on picking up the flag.
static qboolean G_ByEnemyFlag(int team, vec3_t origin)
{
	char *classname;
	gentity_t *ent;

	// get the goal flag this player should have been going for
	if ( g_gametype.integer == GT_CTF ) {
		if ( team == TEAM_BLUE ) {
			classname = "team_CTF_redflag";
		}
		else {
			classname = "team_CTF_blueflag";
		}
	}
	else if ( g_gametype.integer == GT_1FCTF ) {
		classname = "team_CTF_neutralflag";
	}
	else {
		return qfalse;
	}
	ent = G_Find(NULL, FOFS(classname), classname);

	// if we found the destination flag and it's not picked up
	if (ent && !(ent->r.svFlags & SVF_NOCLIENT) ) {
		vec3_t dir;
		// if the player was *very* close
		VectorSubtract( origin, ent->s.origin, dir );
		if ( VectorLength(dir) < 200 ) {
			// Player is by flag, change to one handed animations.
			return qtrue;
		}
	}

	return qfalse;
}
#endif

#ifdef TA_SP
void ExitLevel( void );
#endif

#ifdef TA_ENTSYS // FUNC_USE
/*
==============
G_FindUseEntity
==============
*/
gentity_t *G_FindUseEntity( gentity_t *ent )
{
	trace_t tr;
	vec3_t muzzle;
	vec3_t forward, right, up;
	vec3_t end;
	gentity_t *traceEnt;

	if (!ent || !ent->client)
		return NULL;

	AngleVectors (ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePoint ( ent, forward, right, up, muzzle );

	VectorMA (muzzle, 32, forward, end);

	trap_Trace( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);

	if (tr.fraction == 1.0)
		return NULL;

	if (tr.entityNum >= ENTITYNUM_MAX_NORMAL)
		return NULL;

	traceEnt = &g_entities[ tr.entityNum ];
	if (traceEnt->use && traceEnt->classname && strcmp(traceEnt->classname, "func_use") == 0)
	{
		// Check to make sure multi_trigger will trigger
		if ( ( traceEnt->spawnflags & 1 ) &&
			ent->client->ps.persistant[PERS_TEAM] != TEAM_RED ) {
			return NULL;
		}
		if ( ( traceEnt->spawnflags & 2 ) &&
			ent->client->ps.persistant[PERS_TEAM] != TEAM_BLUE ) {
			return NULL;
		}
#ifdef TA_PLAYERSYS // ABILITY_TECH
		if ( ( traceEnt->spawnflags & 4 ) &&
			ent->client->pers.playercfg.ability != ABILITY_TECH ) {
			return NULL;
		}
#endif

		return traceEnt;
	}

	return NULL;
}
#endif

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

If "g_synchronousClients 1" is set, this will be called exactly
once for each server frame, which makes for smooth demo recording.
==============
*/
void ClientThink_real( gentity_t *ent ) {
	gclient_t	*client;
	pmove_t		pm;
	int			oldEventSequence;
	int			msec;
	usercmd_t	*ucmd;
#ifdef TURTLEARENA // LOCKON
	vec3_t vieworigin;
#endif
#ifdef TA_ENTSYS // FUNC_USE
	gentity_t *useEnt;
#endif

	client = ent->client;

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if (client->pers.connected != CON_CONNECTED) {
		return;
	}
	// mark the time, so the connection sprite can be removed
	ucmd = &ent->client->pers.cmd;

	// sanity check the command time to prevent speedup cheating
	if ( ucmd->serverTime > level.time + 200 ) {
		ucmd->serverTime = level.time + 200;
//		G_Printf("serverTime <<<<<\n" );
	}
	if ( ucmd->serverTime < level.time - 1000 ) {
		ucmd->serverTime = level.time - 1000;
//		G_Printf("serverTime >>>>>\n" );
	} 

	msec = ucmd->serverTime - client->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if ( msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		return;
	}
	if ( msec > 200 ) {
		msec = 200;
	}

	if ( pmove_msec.integer < 8 ) {
		trap_Cvar_Set("pmove_msec", "8");
	}
	else if (pmove_msec.integer > 33) {
		trap_Cvar_Set("pmove_msec", "33");
	}

	if ( pmove_fixed.integer || client->pers.pmoveFixed ) {
		ucmd->serverTime = ((ucmd->serverTime + pmove_msec.integer-1) / pmove_msec.integer) * pmove_msec.integer;
		//if (ucmd->serverTime - client->ps.commandTime <= 0)
		//	return;
	}

	//
	// check for exiting intermission
	//
	if ( level.intermissiontime ) {
		ClientIntermissionThink( client );
		return;
	}

	// spectators don't do much
	if ( client->sess.sessionTeam == TEAM_SPECTATOR
#ifdef IOQ3ZTM // PEAKING
		|| ( G_AllowPeaking() && client->sess.spectatorState == SPECTATOR_FOLLOW )
#endif
		)
	{
#ifndef IOQ3ZTM
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD) {
			return;
		}
#endif
		SpectatorThink( ent, ucmd );
		return;
	}

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	if ( !ClientInactivityTimer( client ) ) {
		return;
	}

#ifdef TURTLEARENA // PLAYERS
	// switch to using waiting animation if idling for awhile.
	if (client->pers.cmd.forwardmove || client->pers.cmd.rightmove || client->pers.cmd.upmove
		|| (client->pers.cmd.buttons & BUTTON_USE_HOLDABLE) || (client->pers.cmd.buttons & BUTTON_ATTACK)
		|| client->damage_blood || (client->ps.pm_flags & PMF_TIME_KNOCKBACK)) {
		client->idleTime = level.time;
		client->ps.eFlags &= ~EF_PLAYER_WAITING;
	} else if (level.time > client->idleTime + TIME_BEFORE_WAITING_ANIMATION) {
		client->ps.eFlags |= EF_PLAYER_WAITING;
	}
#endif

	// clear the rewards if time
	if ( level.time > client->rewardTime ) {
#ifdef IOQ3ZTM
		client->ps.eFlags &= ~EF_AWARD_BITS;
#else
		client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
#endif
	}

	if ( client->noclip ) {
		client->ps.pm_type = PM_NOCLIP;
	} else if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		client->ps.pm_type = PM_DEAD;
	} else {
		client->ps.pm_type = PM_NORMAL;
	}

	client->ps.gravity = g_gravity.value;

	// set speed
#ifdef TA_PLAYERSYS // PER_PLAYER_SPEED
	client->ps.speed = client->pers.playercfg.max_speed * (g_speed.value / 320);
#else
	client->ps.speed = g_speed.value;
#endif

#ifdef MISSIONPACK
#ifdef TA_ITEMSYS
	if( BG_ItemForItemNum(client->ps.stats[STAT_PERSISTANT_POWERUP])->giTag == PW_SCOUT )
#else
	if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_SCOUT )
#endif
	{
		client->ps.speed *= 1.5;
	}
	else
#endif
	if ( client->ps.powerups[PW_HASTE] ) {
		client->ps.speed *= 1.3;
	}

#ifdef TURTLEARENA // LOCKON
	if (client->ps.eFlags & EF_LOCKON)
	{
		// Walk faster forward like in LOZ:TP
		if (ucmd->forwardmove > 0) {
			client->ps.speed *= 1.5;
		} else {
			client->ps.speed /= 1.4;
		}
	}
#endif

	// Let go of the hook if we aren't firing
#ifdef TA_WEAPSYS
	if (client->hook) {
		qboolean weapon_grapple = client->hook->s.weapon == bg_weapongroupinfo[client->ps.weapon].weapon[0]->projnum
							|| client->hook->s.weapon == bg_weapongroupinfo[client->ps.weapon].weapon[1]->projnum;

		if ( (weapon_grapple && !(ucmd->buttons & BUTTON_ATTACK))
#ifdef TA_HOLDSYS // Allow shurikens to be grappling
			|| (!weapon_grapple && !(ucmd->buttons & BUTTON_USE_HOLDABLE))
#endif
			)
		{
			Weapon_HookFree(client->hook);
		}
	}
#else
	if ( client->ps.weapon == WP_GRAPPLING_HOOK &&
		client->hook && !( ucmd->buttons & BUTTON_ATTACK ) ) {
		Weapon_HookFree(client->hook);
	}
#endif

#ifdef TURTLEARENA // LOCKON
	// Set Vieworigin
	VectorCopy(client->ps.origin, vieworigin);
	vieworigin[2] += client->ps.viewheight;

	// Check if current target is valid.
	if (client->sess.sessionTeam == TEAM_SPECTATOR) {
		ent->enemy = NULL;
	} else {
		if (ent->enemy && (ent->enemy == ent || !ent->enemy->takedamage)) {
			ent->enemy = NULL;
		}

		if (!(client->ps.eFlags & EF_LOCKON)
			|| !G_ValidTarget(ent, ent->enemy, vieworigin, client->ps.viewangles, 768.0f, 90.0f, 2))
		{
			// Search for a new target
			ent->enemy = G_FindTarget(ent, vieworigin, client->ps.viewangles, 768.0f, 90.0f);
		}
	}

	// Set origin of target origin
	if (ent->enemy)
	{
		if (ent->enemy->client) {
			VectorCopy(ent->enemy->client->ps.origin, client->ps.enemyOrigin);
			client->ps.enemyOrigin[2] += ent->enemy->client->ps.viewheight;
		} else {
			VectorCopy(ent->enemy->r.currentOrigin, client->ps.enemyOrigin);
			client->ps.enemyOrigin[2] += ent->enemy->s.maxs[2] * 0.8f;
		}
		client->ps.enemyEnt = ent->enemy-g_entities;
	}
	else
	{
		VectorClear(client->ps.enemyOrigin);
		client->ps.enemyEnt = ENTITYNUM_NONE;
	}

	if (client->hadLockon != (client->ps.eFlags & EF_LOCKON))
	{
		client->hadLockon = (client->ps.eFlags & EF_LOCKON);
		client->lockonTime = level.time;
	}

	// A_FaceTarget!
	if ((client->ps.eFlags & EF_LOCKON) && ent->enemy
		&& ( level.time - client->lockonTime ) > LOCKON_TIME/2)
	{
		vec3_t dir;
		vec3_t viewAngles;
		vec3_t origin;

		VectorCopy(client->ps.origin, origin);
		origin[2] += client->ps.viewheight;

		VectorSubtract( client->ps.enemyOrigin, origin, dir );

		vectoangles( dir, viewAngles );

		BG_SwingAngles( viewAngles[YAW], 40, 90, BG_SWINGSPEED, &ent->client->pers.legs.yawAngle, &ent->client->pers.legs.yawing, (level.time - level.previousTime) );
		viewAngles[YAW] = ent->client->pers.legs.yawAngle;

		SetClientViewAngle(ent, viewAngles);
	}
#endif
#ifdef TA_PATHSYS
	if (client->ps.pathMode == PATHMODE_SIDE || client->ps.pathMode == PATHMODE_BACK)
	{
		// A_FaceTarget!
		if (ent->client->pers.cmd.rightmove != 0)
		{
			vec3_t dir;
			vec3_t viewAngles;

			if (ent->client->pers.cmd.rightmove < 0) { // LEFT, pervTrain
				client->ps.eFlags |= EF_TRAINBACKWARD;
				VectorSubtract( ent->pos1, client->ps.origin, dir );
			} else { // RIGHT, nextTrain
				client->ps.eFlags &= ~EF_TRAINBACKWARD;
				VectorSubtract( ent->pos2, client->ps.origin, dir );
			}
			
			dir[2] = 0;

			vectoangles( dir, viewAngles );

			//BG_SwingAngles( viewAngles[YAW], 40, 90, BG_SWINGSPEED, &ent->client->pers.legs.yawAngle, &ent->client->pers.legs.yawing, (level.time - level.previousTime) );
			//viewAngles[YAW] = ent->client->pers.legs.yawAngle;

			SetClientViewAngle(ent, viewAngles);
		}

		G_MoveOnPath(ent);
	}
#endif

	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	memset (&pm, 0, sizeof(pm));

#ifdef TA_WEAPSYS
	// Switch to single handed when close to CTF flag
	// ZTM: TODO: Don't allow CTF flag to be picked up while melee attacking?
	{
		qboolean holdingFlag = (client->ps.powerups[PW_BLUEFLAG]
								|| client->ps.powerups[PW_REDFLAG]
								|| client->ps.powerups[PW_NEUTRALFLAG]);

		// Switch to single handed
		if (holdingFlag || G_ByEnemyFlag(client->ps.persistant[PERS_TEAM], client->ps.origin))
		{
			// The Good: When byFlag and attacking, use both hands to attack.
			// The Bad: If standing by flag, secondary weapon is got out and
			//    put away before/after each attack...
			if (!holdingFlag && client->ps.meleeTime)
			{
				client->ps.eFlags &= ~EF_PRIMARY_HAND;
			}
			else
			{
				client->ps.eFlags |= EF_PRIMARY_HAND;
			}
		}
		else if (client->ps.eFlags & EF_PRIMARY_HAND)
		{
			// Don't change while melee attacking
			if (!client->ps.meleeTime)
			{
				client->ps.eFlags &= ~EF_PRIMARY_HAND;
			}
		}
	}
#endif

#ifdef TA_WEAPSYS // MELEEATTACK
	if (client->ps.pm_type < PM_DEAD && !(client->ps.pm_flags & PMF_RESPAWNED))
	{
		if (BG_WeaponHasType(client->ps.weapon, WT_GAUNTLET))
		{
			if ((ucmd->buttons & BUTTON_ATTACK) && !(ucmd->buttons & BUTTON_TALK)
				&& client->ps.weaponTime <= 0 )
			{
				pm.gauntletHit = G_MeleeDamage( ent, qtrue );
			}
			else
			{
				// If always does damage, do damage anyway.
				G_MeleeDamage( ent, qfalse );
			}
		}
		else
		{
			if ((ucmd->buttons & BUTTON_ATTACK) && !( ucmd->buttons & BUTTON_TALK )
				&& client->ps.meleeDelay <= 0 && client->ps.meleeTime <= 0
				&& client->ps.weaponTime <= 0)
			{
				G_StartMeleeAttack(ent);
			}

			G_MeleeDamage( ent, (client->ps.meleeTime > 0) );
		}
	}
#else
	// check for the hit-scan gauntlet, don't let the action
	// go through as an attack unless it actually hits something
	if ( client->ps.weapon == WP_GAUNTLET && !( ucmd->buttons & BUTTON_TALK ) &&
		( ucmd->buttons & BUTTON_ATTACK ) && client->ps.weaponTime <= 0 ) {
		pm.gauntletHit = CheckGauntletAttack( ent );
	}
#endif

	if ( ent->flags & FL_FORCE_GESTURE ) {
		ent->flags &= ~FL_FORCE_GESTURE;
		ent->client->pers.cmd.buttons |= BUTTON_GESTURE;
	}

#ifdef TA_ENTSYS // FUNC_USE
	useEnt = G_FindUseEntity(ent);

	if (useEnt)
	{
		ent->client->ps.eFlags |= EF_USE_ENT;

		if ((ucmd->buttons & BUTTON_USE_HOLDABLE) &&
#ifdef TURTLEARENA // HOLD_SHURIKEN
			ent->client->ps.holdableTime <= 0
#else
			! ( ent->client->ps.pm_flags & PMF_USE_ITEM_HELD )
#endif
			)
		{
			if (useEnt->use) {
				useEnt->use(useEnt, ent, ent);
			}
#ifdef TURTLEARENA // HOLD_SHURIKEN
			ent->client->ps.holdableTime = 500;
#else
			ent->client->ps.pm_flags |= PMF_USE_ITEM_HELD;
#endif
		}
	}
	else
	{
		ent->client->ps.eFlags &= ~EF_USE_ENT;
	}
#endif

#ifndef TURTLEARENA // POWERS
#ifdef MISSIONPACK
	// check for invulnerability expansion before doing the Pmove
	if (client->ps.powerups[PW_INVULNERABILITY] ) {
		if ( !(client->ps.pm_flags & PMF_INVULEXPAND) ) {
			vec3_t mins = { -42, -42, -42 };
			vec3_t maxs = { 42, 42, 42 };
			vec3_t oldmins, oldmaxs;

			VectorCopy (ent->s.mins, oldmins);
			VectorCopy (ent->s.maxs, oldmaxs);
			// expand
			VectorCopy (mins, ent->s.mins);
			VectorCopy (maxs, ent->s.maxs);
			trap_LinkEntity(ent);
			// check if this would get anyone stuck in this player
			if ( !StuckInOtherClient(ent) ) {
				// set flag so the expanded size will be set in PM_CheckDuck
				client->ps.pm_flags |= PMF_INVULEXPAND;
			}
			// set back
			VectorCopy (oldmins, ent->s.mins);
			VectorCopy (oldmaxs, ent->s.maxs);
			trap_LinkEntity(ent);
		}
	}
#endif
#endif

	pm.ps = &client->ps;
#ifdef TA_PLAYERSYS // PMove
	pm.playercfg = &client->pers.playercfg;
#endif
	pm.cmd = *ucmd;
#ifndef TURTLEARENA // NO_BODY_TRACE
	if ( pm.ps->pm_type == PM_DEAD ) {
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else
#endif
	if ( ent->r.svFlags & SVF_BOT ) {
		pm.tracemask = MASK_PLAYERSOLID | CONTENTS_BOTCLIP;
	}
	else {
		pm.tracemask = MASK_PLAYERSOLID;
	}
	if (client->ps.capsule) {
		pm.trace = trap_TraceCapsule;
	} else {
		pm.trace = trap_Trace;
	}
	pm.pointcontents = trap_PointContents;
	pm.debugLevel = g_debugMove.integer;
	pm.noFootsteps = ( g_dmflags.integer & DF_NO_FOOTSTEPS ) > 0;

	pm.pmove_fixed = pmove_fixed.integer | client->pers.pmoveFixed;
	pm.pmove_msec = pmove_msec.integer;

#if 0 //#ifdef TA_ENTSYS // PUSHABLE
	if ( !client->noclip )
	{
		gentity_t *obstacle;
		vec3_t move;
		vec3_t amove;

		obstacle = NULL;

		VectorSubtract( client->ps.origin, client->oldOrigin, move );
		VectorClear( amove );
		amove[YAW] = client->ps.viewangles[YAW]-client->oldYaw;

		VectorCopy( client->oldOrigin, ent->r.currentOrigin );
		VectorCopy( client->ps.viewangles, ent->r.currentAngles );
		ent->r.currentAngles[YAW] -= amove[YAW];

		G_PlayerPush(ent, move, amove, &obstacle);
	}
	client->oldYaw = client->ps.viewangles[YAW];
#endif


	VectorCopy( client->ps.origin, client->oldOrigin );

#ifdef MISSIONPACK
		if (level.intermissionQueued != 0 && g_singlePlayer.integer) {
			if ( level.time - level.intermissionQueued >= 1000  ) {
				pm.cmd.buttons = 0;
				pm.cmd.forwardmove = 0;
				pm.cmd.rightmove = 0;
				pm.cmd.upmove = 0;
				if ( level.time - level.intermissionQueued >= 2000 && level.time - level.intermissionQueued <= 2500 ) {
					trap_Cmd_ExecuteText( EXEC_APPEND, "centerview\n");
				}
				ent->client->ps.pm_type = PM_SPINTERMISSION;
			}
		}
#endif
	Pmove (&pm);

	// save results of pmove
	if ( ent->client->ps.eventSequence != oldEventSequence ) {
		ent->eventTime = level.time;
	}
	if (g_smoothClients.integer) {
		BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, qtrue );
	}
	else {
		BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );
	}
	SendPendingPredictableEvents( &ent->client->ps );

	if ( !( ent->client->ps.eFlags & EF_FIRING ) ) {
#ifdef IOQ3ZTM
		client->ps.pm_flags &= ~PMF_FIRE_HELD;
#else
		client->fireHeld = qfalse;		// for grapple
#endif
	}

#ifdef TA_PLAYERSYS // LADDER
	// Stop grappling if grappled to a ladder
	if ((ent->client->ps.eFlags & EF_LADDER) && ent->client->hook) {
		Weapon_HookFree(ent->client->hook);
	}
#endif

	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );

	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;

	// execute client events
	ClientEvents( ent, oldEventSequence );

	// link entity now, after any personal teleporters have been used
	trap_LinkEntity (ent);
	if ( !ent->client->noclip ) {
		G_TouchTriggers( ent );
	}

	// NOTE: now copy the exact origin over otherwise clients can be snapped into solid
	VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );

	//test for solid areas in the AAS file
	BotTestAAS(ent->r.currentOrigin);

	// touch other objects
	ClientImpacts( ent, &pm );

	// save results of triggers and client events
	if (ent->client->ps.eventSequence != oldEventSequence) {
		ent->eventTime = level.time;
	}

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// check for respawning
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
#ifdef TA_SP // ZTM: Single player death think
		// wait for the attack button to be pressed
		if (g_gametype.integer == GT_SINGLE_PLAYER)
		{
			if (!client->ps.persistant[PERS_LIVES] && !client->ps.persistant[PERS_CONTINUES])
			{
				// Game Over, don't respawn. In multiplayer you get three lives on the next level.
				client->respawnTime = -1;
			}

			// If not haven't Game Overed,
			//   auto respawn in 3 seconds, or if client pressed attack, use, or jump.
			if ( client->respawnTime > 0 && level.time > client->respawnTime &&
				((level.time - client->respawnTime > 3000)
					|| (ucmd->buttons & (BUTTON_ATTACK|BUTTON_USE_HOLDABLE)) || (ucmd->upmove > 0)) )
			{
				if (!client->ps.persistant[PERS_LIVES])
				{
					/*if (g_singlePlayer.integer && client->ps.pers[PERS_CONTINUES])
					{
						// ZTM: TODO: Open "Contiue?" menu/screen
					}
					else */if (client->ps.persistant[PERS_CONTINUES])
					{
						client->ps.persistant[PERS_CONTINUES]--;
						client->ps.persistant[PERS_LIVES] += 3;
						ClientRespawn( ent );
					}
				}
				else
				{
					ClientRespawn( ent );
				}
			}
			return;
		}
#endif
		// wait for the attack button to be pressed
		if ( level.time > client->respawnTime ) {
			// forcerespawn is to prevent users from waiting out powerups
			if ( g_forcerespawn.integer > 0 && ( level.time - client->respawnTime ) > g_forcerespawn.integer * 1000 ) {
				ClientRespawn( ent );
				return;
			}
		
			// pressing attack or use is the normal respawn method
#ifdef TA_MISC
			if ( ( ucmd->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) ) || ucmd->upmove > 0 )
#else
			if ( ucmd->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) )
#endif
			{
				ClientRespawn( ent );
			}
		}
		return;
	}

	// perform once-a-second actions
	ClientTimerActions( ent, msec );
}

/*
==================
ClientThink

A new command has arrived from the client
==================
*/
void ClientThink( int clientNum ) {
	gentity_t *ent;

	ent = g_entities + clientNum;
	trap_GetUsercmd( clientNum, &ent->client->pers.cmd );

	// mark the time we got info, so we can display the
	// phone jack if they don't get any for a while
	ent->client->lastCmdTime = level.time;

	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
		ClientThink_real( ent );
	}
}

#ifdef TA_GAME_MODELS
/*
===============
CG_PlayerAngles

Handles seperate torso motion

  legs pivot based on direction of movement

  head always looks exactly at ent->angles (cent->lerpAngles in cgame)

  if motion < 20 degrees, show in head only
  if < 45 degrees, also show in torso
===============
*/
// Game version of CG_PlayerAngles
void G_PlayerAngles( gentity_t *ent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3] )
{
	vec3_t		legsAngles, torsoAngles, headAngles;
	float		dest;
	static	int	movementOffsets[8] = { 0, 22, 45, -22, 0, 22, -45, -22 };
	vec3_t		velocity;
	float		speed;
	int			dir;
	int frametime = (level.time - level.previousTime);

	VectorCopy( ent->client->ps.viewangles, headAngles );
	headAngles[YAW] = AngleMod( headAngles[YAW] );
	VectorClear( legsAngles );
	VectorClear( torsoAngles );

	// --------- yaw -------------

	// allow yaw to drift a bit
#ifdef TA_WEAPSYS
	if (!BG_PlayerStandAnim(&ent->client->pers.playercfg, AP_LEGS, ent->client->ps.legsAnim)
		|| !BG_PlayerStandAnim(&ent->client->pers.playercfg, AP_TORSO, ent->client->ps.torsoAnim))
#else
	if ( ( ent->client->ps.legsAnim & ~ANIM_TOGGLEBIT ) != LEGS_IDLE
		|| ( ent->client->ps.torsoAnim & ~ANIM_TOGGLEBIT ) != TORSO_STAND  )
#endif
	{
		// if not standing still, always point all in the same direction
		ent->client->pers.torso.yawing = qtrue;	// always center
		ent->client->pers.torso.pitching = qtrue;	// always center
		ent->client->pers.legs.yawing = qtrue;	// always center
	}

	// adjust legs for movement dir
	if ( ent->client->ps.eFlags & EF_DEAD ) {
		// don't let dead bodies twitch
		dir = 0;
	} else {
		dir = ent->client->ps.movementDir;
		if ( dir < 0 || dir > 7 ) {
			Com_Error(ERR_DROP, "G_PlayerAngles: Bad player movement angle" );
		}
	}
	legsAngles[YAW] = headAngles[YAW] + movementOffsets[ dir ];
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * movementOffsets[ dir ];

	// torso
	BG_SwingAngles( torsoAngles[YAW], 25, 90, BG_SWINGSPEED, &ent->client->pers.torso.yawAngle, &ent->client->pers.torso.yawing, frametime );
	BG_SwingAngles( legsAngles[YAW], 40, 90, BG_SWINGSPEED, &ent->client->pers.legs.yawAngle, &ent->client->pers.legs.yawing, frametime );

	torsoAngles[YAW] = ent->client->pers.torso.yawAngle;
	legsAngles[YAW] = ent->client->pers.legs.yawAngle;


	// --------- pitch -------------

#ifdef TA_PLAYERSYS // ZTM: If BOTH_* animation, don't have torso pitch
	if (ent->client->pers.playercfg.fixedtorso || (ent->client->ps.torsoAnim & ~ANIM_TOGGLEBIT) == (ent->client->ps.legsAnim & ~ANIM_TOGGLEBIT)) {
		dest = 0;
		headAngles[PITCH] = Com_Clamp( -65, 20, headAngles[PITCH] );
	} else
#endif
	// only show a fraction of the pitch angle in the torso
	if ( headAngles[PITCH] > 180 ) {
		dest = (-360 + headAngles[PITCH]) * 0.75f;
	} else {
		dest = headAngles[PITCH] * 0.75f;
	}
	BG_SwingAngles( dest, 15, 30, 0.1f, &ent->client->pers.torso.pitchAngle, &ent->client->pers.torso.pitching, frametime );
	torsoAngles[PITCH] = ent->client->pers.torso.pitchAngle;

#ifndef TA_PLAYERSYS
	//
	if ( qfalse )
	{
		torsoAngles[PITCH] = 0.0f;
	}
#endif

	// --------- roll -------------


	// lean towards the direction of travel
	VectorCopy( ent->s.pos.trDelta, velocity );
	speed = VectorNormalize( velocity );
	if ( speed ) {
		vec3_t	axis[3];
		float	side;

		speed *= 0.05f;

		AnglesToAxis( legsAngles, axis );
		side = speed * DotProduct( velocity, axis[1] );
		legsAngles[ROLL] -= side;

		side = speed * DotProduct( velocity, axis[0] );
		legsAngles[PITCH] += side;
	}

	//
#ifdef TA_PLAYERSYS
	if ( ent->client->pers.playercfg.fixedlegs )
#else
	if ( qfalse )
#endif
	{
		legsAngles[YAW] = torsoAngles[YAW];
		legsAngles[PITCH] = 0.0f;
		legsAngles[ROLL] = 0.0f;
	}

#ifdef TA_PLAYERSYS // LADDER
	if (ent->client->ps.eFlags & EF_LADDER) {
		// Ladder dir, plaver legs should always face dir
		vectoangles(ent->client->ps.origin2, legsAngles);
		// If BOTH_* animation, have torso face ladder too
		if ((ent->client->ps.torsoAnim & ~ANIM_TOGGLEBIT) == (ent->client->ps.legsAnim & ~ANIM_TOGGLEBIT)) {
			VectorCopy(legsAngles, torsoAngles);
			headAngles[0] += torsoAngles[0];
			headAngles[1] += torsoAngles[1];
			headAngles[2] += torsoAngles[2];
		}
	}
#endif

	// ZTM: TODO: Add pain twitch ?
	//CG_AddPainTwitch( cent, torsoAngles );

	// pull the angles back out of the hierarchial chain
	AnglesSubtract( headAngles, torsoAngles, headAngles );
	AnglesSubtract( torsoAngles, legsAngles, torsoAngles );
	AnglesToAxis( legsAngles, legs );
	AnglesToAxis( torsoAngles, torso );
	AnglesToAxis( headAngles, head );
}

// Game version of CG_PlayerAnimation / UI_PlayerAnimation
void G_PlayerAnimation( gentity_t *ent )
{
	int legsAnim;

	if ( ent->client->pers.legs.yawing &&
#ifdef TA_WEAPSYS
		BG_PlayerStandAnim(&ent->client->pers.playercfg, AP_LEGS, ent->client->ps.legsAnim)
#else
		( ent->client->ps.legsAnim & ~ANIM_TOGGLEBIT ) == LEGS_IDLE
#endif
		)
		legsAnim = LEGS_TURN;
	else
		legsAnim = ent->client->ps.legsAnim;

	BG_RunLerpFrame( &ent->client->pers.legs,
#ifdef TA_PLAYERSYS
		ent->client->pers.playercfg.animations,
#else
#error "ERROR: TA_PLAYERSYS needs to be defined."
		NULL, // ZTM: This must be valid!
#endif
		legsAnim, level.time, ent->client->ps.powerups[PW_HASTE] ? 1.5f : 1.0f );

	BG_RunLerpFrame( &ent->client->pers.torso,
#ifdef TA_PLAYERSYS
		ent->client->pers.playercfg.animations,
#else
#error "ERROR: TA_PLAYERSYS needs to be defined."
		NULL, // ZTM: This must be valid!
#endif
		ent->client->ps.torsoAnim, level.time, ent->client->ps.powerups[PW_HASTE] ? 1.5f : 1.0f );
}
#endif

void G_RunClient( gentity_t *ent ) {
#ifdef TA_GAME_MODELS
	G_PlayerAngles(ent, ent->client->pers.legsAxis, ent->client->pers.torsoAxis, ent->client->pers.headAxis);

	G_PlayerAnimation(ent);
#endif
	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
		return;
	}
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink_real( ent );
}


/*
==================
SpectatorClientEndFrame

==================
*/
void SpectatorClientEndFrame( gentity_t *ent ) {
	gclient_t	*cl;

	// if we are doing a chase cam or a remote view, grab the latest info
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		int		clientNum, flags;

		clientNum = ent->client->sess.spectatorClient;

		// team follow1 and team follow2 go to whatever clients are playing
		if ( clientNum == -1 ) {
			clientNum = level.follow1;
		} else if ( clientNum == -2 ) {
			clientNum = level.follow2;
		}
		if ( clientNum >= 0 ) {
			cl = &level.clients[ clientNum ];
			if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR ) {
				flags = (cl->ps.eFlags & ~(EF_VOTED | EF_TEAMVOTED)) | (ent->client->ps.eFlags & (EF_VOTED | EF_TEAMVOTED));
				ent->client->ps = cl->ps;
				ent->client->ps.pm_flags |= PMF_FOLLOW;
				ent->client->ps.eFlags = flags;
				return;
			} else {
				// drop them to free spectators unless they are dedicated camera followers
				if ( ent->client->sess.spectatorClient >= 0 ) {
					ent->client->sess.spectatorState = SPECTATOR_FREE;
					ClientBegin( ent->client - level.clients );
				}
			}
		}
	}

	if ( ent->client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
		ent->client->ps.pm_flags |= PMF_SCOREBOARD;
	} else {
		ent->client->ps.pm_flags &= ~PMF_SCOREBOARD;
	}
}

/*
==============
ClientEndFrame

Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEdFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
==============
*/
void ClientEndFrame( gentity_t *ent ) {
	int			i;

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR
#ifdef IOQ3ZTM // PEAKING
		|| ( G_AllowPeaking() && ent->client->sess.spectatorState == SPECTATOR_FOLLOW )
#endif
		)
	{
		SpectatorClientEndFrame( ent );
		return;
	}

	// turn off any expired powerups
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ent->client->ps.powerups[ i ] < level.time ) {
#ifdef NIGHTSMODE
			if (ent->client->ps.powerups[ i ] > 0
				&& i == PW_FLIGHT)
			{
				G_DeNiGHTSizePlayer(ent);
			}
			else
#endif
			ent->client->ps.powerups[ i ] = 0;
		}
	}

#ifdef MISSIONPACK
	// set powerup for player animation
#ifdef TA_ITEMSYS
	if( BG_ItemForItemNum(ent->client->ps.stats[STAT_PERSISTANT_POWERUP])->giTag == PW_GUARD )
#else
	if( bg_itemlist[ent->client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD )
#endif
	{
		ent->client->ps.powerups[PW_GUARD] = level.time;
	}
#ifdef TA_ITEMSYS
	if( BG_ItemForItemNum(ent->client->ps.stats[STAT_PERSISTANT_POWERUP])->giTag == PW_SCOUT )
#else
	if( bg_itemlist[ent->client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_SCOUT )
#endif
	{
		ent->client->ps.powerups[PW_SCOUT] = level.time;
	}
#ifdef TA_ITEMSYS
	if( BG_ItemForItemNum(ent->client->ps.stats[STAT_PERSISTANT_POWERUP])->giTag == PW_DOUBLER )
#else
	if( bg_itemlist[ent->client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_DOUBLER )
#endif
	{
		ent->client->ps.powerups[PW_DOUBLER] = level.time;
	}
#ifdef TA_ITEMSYS
	if( BG_ItemForItemNum(ent->client->ps.stats[STAT_PERSISTANT_POWERUP])->giTag == PW_AMMOREGEN )
#else
	if( bg_itemlist[ent->client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_AMMOREGEN )
#endif
	{
		ent->client->ps.powerups[PW_AMMOREGEN] = level.time;
	}
#ifndef TURTLEARENA // POWERS
	if ( ent->client->invulnerabilityTime > level.time ) {
		ent->client->ps.powerups[PW_INVULNERABILITY] = level.time;
	}
#endif
#endif

	// save network bandwidth
#if 0
	if ( !g_synchronousClients->integer && ent->client->ps.pm_type == PM_NORMAL ) {
		// FIXME: this must change eventually for non-sync demo recording
		VectorClear( ent->client->ps.viewangles );
	}
#endif

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if ( level.intermissiontime ) {
		return;
	}

	// burn from lava, etc
	P_WorldEffects (ent);

	// apply all the damage taken this frame
	P_DamageFeedback (ent);

	// add the EF_CONNECTION flag if we haven't gotten commands recently
	if ( level.time - ent->client->lastCmdTime > 1000 ) {
		ent->client->ps.eFlags |= EF_CONNECTION;
	} else {
		ent->client->ps.eFlags &= ~EF_CONNECTION;
	}

	ent->client->ps.stats[STAT_HEALTH] = ent->health;	// FIXME: get rid of ent->health...

	G_SetClientSound (ent);

	// set the latest infor
	if (g_smoothClients.integer) {
		BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, qtrue );
	}
	else {
		BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );
	}
	SendPendingPredictableEvents( &ent->client->ps );

	// set the bit for the reachability area the client is currently in
//	i = trap_AAS_PointReachabilityAreaIndex( ent->client->ps.origin );
//	ent->client->areabits[i >> 3] |= 1 << (i & 7);
}


