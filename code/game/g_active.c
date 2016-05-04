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
void P_DamageFeedback( gentity_t *ent ) {
	gplayer_t	*player;
	float	count;
	vec3_t	angles;

	player = ent->player;
	if ( player->ps.pm_type == PM_DEAD ) {
		return;
	}

	// total points of damage shot at the player this frame
#ifdef TURTLEARENA // NOARMOR
	count = player->damage_blood;
#else
	count = player->damage_blood + player->damage_armor;
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
	if ( player->damage_fromWorld ) {
		player->ps.damagePitch = 255;
		player->ps.damageYaw = 255;

		player->damage_fromWorld = qfalse;
	} else {
		vectoangles( player->damage_from, angles );
		player->ps.damagePitch = angles[PITCH]/360.0 * 256;
		player->ps.damageYaw = angles[YAW]/360.0 * 256;
	}

	// play an apropriate pain sound
	if ( (level.time > ent->pain_debounce_time) && !(ent->flags & FL_GODMODE) ) {
		ent->pain_debounce_time = level.time + 700;
		G_AddEvent( ent, EV_PAIN, ent->health );
		player->ps.damageEvent++;
	}


	player->ps.damageCount = count;

	//
	// clear totals
	//
	player->damage_blood = 0;
#ifndef TURTLEARENA // NOARMOR
	player->damage_armor = 0;
#endif
	player->damage_knockback = 0;
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
	if ( ent->player->noclip )
#endif
	{
#ifdef TURTLEARENA // DROWNING
		ent->player->ps.powerups[PW_AIR] = level.time + 31000;	// don't need air
#else
		ent->player->airOutTime = level.time + 12000;	// don't need air
#endif
		return;
	}

	waterlevel = ent->waterlevel;

#ifndef TURTLEARENA // POWERS
	envirosuit = ent->player->ps.powerups[PW_BATTLESUIT] > level.time;
#endif

	//
	// check for drowning
	//
	if ( waterlevel == 3 ) {
#ifndef TURTLEARENA // POWERS
		// envirosuit give air
		if ( envirosuit ) {
			ent->player->airOutTime = level.time + 10000;
		}
#endif

		// if out of air, start drowning
#ifdef TURTLEARENA // DROWNING
		if ( ent->player->ps.powerups[PW_AIR] < level.time )
#else
		if ( ent->player->airOutTime < level.time)
#endif
		{
			// drown!
#ifndef TURTLEARENA // DROWNING
			ent->player->airOutTime += 1000;
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
		else if (ent->player->ps.powerups[PW_AIR] < level.time + 5000 && !(ent->flags & FL_DROWNING_WARNING)) {
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
		if (ent->player->ps.powerups[PW_AIR]+1000 < level.time + 31000) {
			ent->player->ps.powerups[PW_AIR] += 1000;
		} else {
			ent->player->ps.powerups[PW_AIR] = level.time + 31000;
		}
#else
		ent->player->airOutTime = level.time + 12000;
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
			if ( ent->player->ps.powerups[PW_INVUL] > level.time ) {
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
G_SetPlayerSound
===============
*/
void G_SetPlayerSound( gentity_t *ent ) {
#if defined MISSIONPACK && !defined TURTLEARENA // WEAPONS
#ifdef TA_WEAPSYS // Per-projectile tick sound name
	if ((ent->s.eFlags & EF_TICKING) && ent->activator
		&& ent->activator->s.weapon > 0 && ent->activator->s.weapon < BG_NumProjectiles()
		&& bg_projectileinfo[ent->activator->s.weapon].explosionType == PE_PROX
		&& bg_projectileinfo[ent->activator->s.weapon].tickSoundName[0])
	{
		ent->player->ps.loopSound = G_SoundIndex(bg_projectileinfo[ent->activator->s.weapon].tickSoundName);
	}
#else
	if( ent->s.eFlags & EF_TICKING ) {
		ent->player->ps.loopSound = G_SoundIndex( "sound/weapons/proxmine/wstbtick.wav");
	}
#endif
	else
#endif
	if (ent->waterlevel && (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		ent->player->ps.loopSound = level.snd_fry;
	} else {
		ent->player->ps.loopSound = 0;
	}
}



//==============================================================

/*
==============
PlayerImpacts
==============
*/
void PlayerImpacts( gentity_t *ent, pmove_t *pm ) {
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

	if ( !ent->player ) {
		return;
	}

	// dead players don't activate triggers!
	if ( ent->player->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	VectorSubtract( ent->player->ps.origin, range, mins );
	VectorAdd( ent->player->ps.origin, range, maxs );

	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	// can't use ent->absmin, because that has a one unit pad
	VectorAdd( ent->player->ps.origin, ent->s.mins, mins );
	VectorAdd( ent->player->ps.origin, ent->s.maxs, maxs );

	for ( i=0 ; i<num ; i++ ) {
		hit = &g_entities[touch[i]];

		if ( !hit->touch && !ent->touch ) {
			continue;
		}
		if ( !( hit->s.contents & CONTENTS_TRIGGER ) ) {
			continue;
		}

		// ignore most entities if a spectator
		if ( ent->player->sess.sessionTeam == TEAM_SPECTATOR ) {
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
			if ( !BG_PlayerTouchesItem( &ent->player->ps, &hit->s, level.time ) ) {
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
	if ( ent->player->ps.jumppad_frame != ent->player->ps.pmove_framecount ) {
		ent->player->ps.jumppad_frame = 0;
		ent->player->ps.jumppad_ent = 0;
	}
}

/*
=================
SpectatorThink
=================
*/
void SpectatorThink( gentity_t *ent, usercmd_t *ucmd ) {
	pmove_t	pm;
	gplayer_t	*player;

	player = ent->player;

	if ( player->sess.spectatorState != SPECTATOR_FOLLOW ) {
		player->ps.pm_type = PM_SPECTATOR;
		player->ps.speed = 400;	// faster than normal

		// set up for pmove
		memset (&pm, 0, sizeof(pm));
		pm.ps = &player->ps;
#ifdef TA_PLAYERSYS // Pmove
		pm.playercfg = &player->pers.playercfg;
#endif
		pm.cmd = *ucmd;
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;	// spectators can fly through bodies
		if (player->ps.collisionType == CT_CAPSULE) {
			pm.trace = trap_TraceCapsule;
		} else {
			pm.trace = trap_Trace;
		}
		pm.pointcontents = trap_PointContents;

		// perform a pmove
		Pmove (&pm);
		// save results of pmove
		VectorCopy( player->ps.origin, ent->s.origin );

		G_TouchTriggers( ent );
		trap_UnlinkEntity( ent );
	}

	player->oldbuttons = player->buttons;
	player->buttons = ucmd->buttons;

	// attack button cycles through spectators
	if ( ( player->buttons & BUTTON_ATTACK ) && ! ( player->oldbuttons & BUTTON_ATTACK ) ) {
		Cmd_FollowCycle_f( ent, 1 );
	}
#ifdef IOQ3ZTM // SPECTATOR
	else if ( ( player->buttons & BUTTON_USE_HOLDABLE ) && ! ( player->oldbuttons & BUTTON_USE_HOLDABLE ) ) {
		Cmd_FollowCycle_f( ent, -1 );
	}
	// Stop following player when jump or crouch is pressed.
	else if ( ent->player->sess.spectatorState == SPECTATOR_FOLLOW && ucmd->upmove != 0 ) {
		StopFollowing(ent);
	}
#endif
}



/*
=================
PlayerInactivityTimer

Returns qfalse if the player is dropped
=================
*/
qboolean PlayerInactivityTimer( gplayer_t *player ) {
	if ( ! g_inactivity.integer ) {
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		player->inactivityTime = level.time + 60 * 1000;
		player->inactivityWarning = qfalse;
	} else if ( player->pers.cmd.forwardmove || 
		player->pers.cmd.rightmove || 
		player->pers.cmd.upmove ||
#ifdef IOQ3ZTM
		(player->pers.cmd.buttons & BUTTON_USE_HOLDABLE) ||
#endif
		(player->pers.cmd.buttons & BUTTON_ATTACK) ) {
		player->inactivityTime = level.time + g_inactivity.integer * 1000;
		player->inactivityWarning = qfalse;
	} else if ( !player->pers.localClient ) {
		if ( level.time > player->inactivityTime ) {
			trap_DropPlayer( player - level.players, "Dropped due to inactivity" );
			return qfalse;
		}
		if ( level.time > player->inactivityTime - 10000 && !player->inactivityWarning ) {
			player->inactivityWarning = qtrue;
			trap_SendServerCommand( player - level.players, "cp \"Ten seconds until inactivity drop!\n\"" );
		}
	}
	return qtrue;
}

/*
==================
PlayerTimerActions

Actions that happen once a second
==================
*/
void PlayerTimerActions( gentity_t *ent, int msec ) {
	gplayer_t	*player;
#ifndef TURTLEARENA // POWERS
	int			maxHealth;
#endif

	player = ent->player;
	player->timeResidual += msec;

#ifdef TA_WEAPSYS // MELEEATTACK
	if (player->ps.meleeLinkTime > 0)
	{
		player->ps.meleeLinkTime -= msec;
		if (player->ps.meleeLinkTime <= 0)
		{
			player->ps.meleeLinkTime = 0;
			player->ps.meleeAttack = -1;
		}
	}

	if (player->ps.meleeDelay > 0)
	{
		player->ps.meleeDelay -= msec;
		if (player->ps.meleeDelay < 0)
		{
			player->ps.meleeDelay = 0;
		}
	}

	if (player->ps.meleeTime > 0)
	{
		player->ps.meleeTime -= msec;
		if (player->ps.meleeTime <= 0)
		{
			int weap_delay;
			int max_combo;

			max_combo = BG_MaxAttackIndex(&player->ps);
			weap_delay = bg_weapongroupinfo[player->ps.weapon].weapon[0]->attackDelay;

			// ZTM: TEST; with the accel changes (xyspeed) weap_delay is way too long.
			weap_delay /= 2;

			if (player->ps.groundEntityNum == ENTITYNUM_NONE) {
				// Jump attack only uses last attack animation
				max_combo = 1;
				weap_delay = 0;
			} else if (BG_PlayerRunning(player->ps.velocity)) {
				// ZTM: if running have no attack delay and only use first half of animations, like in LoZ:TP
				max_combo /= 2;
				weap_delay = 0;
			}

			player->ps.meleeTime = 0;

			if (player->ps.meleeAttack == max_combo-1)
			{
				//G_Printf("DEBUG: player %i finished last combo (%i)\n", ent - g_entities, player->ps.meleeAttack);

				player->ps.meleeDelay = weap_delay;
				player->ps.weaponTime = player->ps.meleeDelay; // Don't let them use a gun...

				player->ps.meleeAttack = -1;
				player->ps.meleeLinkTime = 0;
			}
		}
	}

	if (player->ps.chainTime > 0)
	{
		player->ps.chainTime -= msec;
		if (player->ps.chainTime <= 0)
		{
			player->ps.chainTime = 0;
			player->ps.chain = 0;
		}
	}
#endif

	while ( player->timeResidual >= 1000 ) {
		player->timeResidual -= 1000;

#ifndef TURTLEARENA // POWERS // Guards having health regen makes them pretty much unkillable, and removed regen powerup.
		// regenerate
#ifdef MISSIONPACK
		if( BG_ItemForItemNum( player->ps.stats[STAT_PERSISTANT_POWERUP] )->giTag == PW_GUARD ) {
			maxHealth = player->ps.stats[STAT_MAX_HEALTH] / 2;
		}
		else
#endif
		if ( player->ps.powerups[PW_REGEN] ) {
			maxHealth = player->ps.stats[STAT_MAX_HEALTH];
		} else {
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
		} else
#endif
		{
			// count down health when over max
			if ( ent->health > player->ps.stats[STAT_MAX_HEALTH] ) {
				ent->health--;
			}
		}

#ifndef TURTLEARENA // NOARMOR
		// count down armor when over max
		if ( player->ps.stats[STAT_ARMOR] > player->ps.stats[STAT_MAX_HEALTH] ) {
			player->ps.stats[STAT_ARMOR]--;
		}
#endif
	}
#ifdef MISSIONPACK
	if( BG_ItemForItemNum( player->ps.stats[STAT_PERSISTANT_POWERUP] )->giTag == PW_AMMOREGEN ) {
#ifdef TA_WEAPSYS_EX
		int w, max, inc, t;

		// Ony gives ammo for current weapon
		w = player->ps.weapon;
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
				player->ammoTimes[w] += msec;
				if ( player->ps.stats[STAT_AMMO] >= max ) {
					player->ammoTimes[w] = 0;
				}
				if ( player->ammoTimes[w] >= t ) {
					while ( player->ammoTimes[w] >= t )
						player->ammoTimes[w] -= t;
					player->ps.stats[STAT_AMMO] += inc;
					if ( player->ps.stats[STAT_AMMO] > max ) {
						player->ps.stats[STAT_AMMO] = max;
					}
				}
			}
#else
#ifdef TA_WEAPSYS
			if (max > 0)
			{
#endif
				player->ammoTimes[w] += msec;
				if ( player->ps.ammo[w] >= max ) {
					player->ammoTimes[w] = 0;
				}
				if ( player->ammoTimes[w] >= t ) {
					while ( player->ammoTimes[w] >= t )
						player->ammoTimes[w] -= t;
					player->ps.ammo[w] += inc;
					if ( player->ps.ammo[w] > max ) {
						player->ps.ammo[w] = max;
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
				player->holdableTimes[h] += msec;
				if ( player->ps.holdable[h] >= max ) {
					player->holdableTimes[h] = 0;
				}
				if ( player->holdableTimes[h] >= t ) {
					while ( player->holdableTimes[h] >= t )
					  player->holdableTimes[h] -= t;
					player->ps.holdable[h] += inc;
					if ( player->ps.holdable[h] > max ) {
					  player->ps.holdable[h] = max;
					}
					// If no holdable is selected auto select this holdable
					if (inc > 0 && player->ps.holdableIndex == HI_NONE)
					{
						player->ps.holdableIndex = h;
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
PlayerIntermissionThink
====================
*/
void PlayerIntermissionThink( gplayer_t *player ) {
	player->ps.eFlags &= ~EF_TALK;
	player->ps.eFlags &= ~EF_FIRING;

	// the level will exit when everyone wants to or after timeouts

	// swap and latch button actions
	player->oldbuttons = player->buttons;
	player->buttons = player->pers.cmd.buttons;
#ifdef TA_SP
	if (g_gametype.integer == GT_SINGLE_PLAYER)
	{
		player->readyToExit = 1;
	}
	else
#endif
	if ( player->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) & ( player->oldbuttons ^ player->buttons ) ) {
		// this used to be an ^1 but once a player says ready, it should stick
		player->readyToExit = 1;
	}
}


/*
================
PlayerEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
void PlayerEvents( gentity_t *ent, int oldEventSequence ) {
	int		i;
	int		event;
	gplayer_t *player;
	int		damage;
#ifndef TURTLEARENA // HOLDABLE // no q3 teleporter
	vec3_t	origin, angles;
#endif
#ifdef TA_WEAPSYS_EX
	gitem_t		*item;
	gentity_t	*drop;
#endif

	player = ent->player;

	if ( oldEventSequence < player->ps.eventSequence - MAX_PS_EVENTS ) {
		oldEventSequence = player->ps.eventSequence - MAX_PS_EVENTS;
	}
	for ( i = oldEventSequence ; i < player->ps.eventSequence ; i++ ) {
		event = player->ps.events[ i & (MAX_PS_EVENTS-1) ];

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

		case EV_FIRE_WEAPON:
			FireWeapon( ent );
			break;

#ifdef TA_WEAPSYS_EX
		case EV_DROP_WEAPON:
			if (ent->player)
			{
				int weapon;
				int ammo;
				int contents;

				// Get drop info
				weapon = ent->player->ps.stats[STAT_DROP_WEAPON];
				ammo = ent->player->ps.stats[STAT_DROP_AMMO];

				// Clear drop info
				ent->player->ps.stats[STAT_DROP_WEAPON] = WP_NONE;
				ent->player->ps.stats[STAT_DROP_AMMO] = 0;

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
			int itemNum = (event & ~EV_EVENT_BITS) - EV_USE_ITEM0;

#ifdef TURTLEARENA // HOLD_SHURIKEN
			G_ThrowShuriken(ent, itemNum);
#endif

			switch ( itemNum ) {
			default:
			case HI_NONE:
				break;

#ifndef TURTLEARENA // HOLDABLE // no q3 teleporter
			case HI_TELEPORTER:
				TossPlayerGametypeItems( ent );
#ifdef TA_PLAYERSYS
				SelectSpawnPoint( ent, origin, angles, qfalse );
#else
				SelectSpawnPoint( ent->player->ps.origin, origin, angles, qfalse );
#endif
				TeleportPlayer( ent, origin, angles );
				break;
#endif

			case HI_MEDKIT:
				ent->health = ent->player->ps.stats[STAT_MAX_HEALTH] + 25;
				break;

#ifdef MISSIONPACK
#ifndef TURTLEARENA // POWERS NO_KAMIKAZE_ITEM
			case HI_KAMIKAZE:
				// make sure the invulnerability is off
				ent->player->invulnerabilityTime = 0;
				// start the kamikze
				G_StartKamikaze( ent );
				break;
#endif

			case HI_PORTAL:
				if ( ent->player->portalID ) {
					DropPortalSource( ent );
				}
				else {
					DropPortalDestination( ent );
				}
				break;

#ifndef TURTLEARENA // POWERS
			case HI_INVULNERABILITY:
				ent->player->invulnerabilityTime = level.time + 10000;
				break;
#endif
#endif // MISSIONPACK
			}
			break;
		}

		default:
			break;
		}
	}

}

#if defined MISSIONPACK && !defined TURTLEARENA // POWERS
/*
==============
StuckInOtherPlayer
==============
*/
static int StuckInOtherPlayer(gentity_t *ent) {
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
		if ( !ent2->player ) {
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
	int event, seq, i;
	int extEvent, number;
	gconnection_t *connection;

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
		t->s.otherEntityNum = ps->playerNum;
		t->s.contents = 0;
		// send to everyone except the client who generated the event
		t->r.svFlags |= SVF_PLAYERMASK;
		Com_ClientListAll( &t->r.sendPlayers );
		connection = &level.connections[ level.players[ ps->playerNum ].pers.connectionNum ];
		for (i = 0; i < connection->numLocalPlayers; i++ ) {
			Com_ClientListRemove( &t->r.sendPlayers, connection->localPlayerNums[i] );
		}
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

	if (!ent || !ent->player)
		return NULL;

	AngleVectors (ent->player->ps.viewangles, forward, right, up);

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
			ent->player->ps.persistant[PERS_TEAM] != TEAM_RED ) {
			return NULL;
		}
		if ( ( traceEnt->spawnflags & 2 ) &&
			ent->player->ps.persistant[PERS_TEAM] != TEAM_BLUE ) {
			return NULL;
		}
#ifdef TA_PLAYERSYS // ABILITY_TECH
		if ( ( traceEnt->spawnflags & 4 ) &&
			ent->player->pers.playercfg.ability != ABILITY_TECH ) {
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
PlayerThink

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

If "g_synchronousClients 1" is set, this will be called exactly
once for each server frame, which makes for smooth demo recording.
==============
*/
void PlayerThink_real( gentity_t *ent ) {
	gplayer_t	*player;
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

	player = ent->player;

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if (player->pers.connected != CON_CONNECTED) {
		return;
	}

	// frameOffset should be about the number of milliseconds into a frame
	// this command packet was received, depending on how fast the server
	// does a G_RunFrame()
	player->frameOffset = trap_Milliseconds() - level.frameStartTime;

	// mark the time, so the connection sprite can be removed
	ucmd = &ent->player->pers.cmd;

	// sanity check the command time to prevent speedup cheating
	if ( ucmd->serverTime > level.time + 200 ) {
		ucmd->serverTime = level.time + 200;
//		G_Printf("serverTime <<<<<\n" );
	}
	if ( ucmd->serverTime < level.time - 1000 ) {
		ucmd->serverTime = level.time - 1000;
//		G_Printf("serverTime >>>>>\n" );
	} 

	player->lastCmdServerTime = ucmd->serverTime;

	msec = ucmd->serverTime - player->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if ( msec < 1 && player->sess.spectatorState != SPECTATOR_FOLLOW ) {
		return;
	}
	if ( msec > 200 ) {
		msec = 200;
	}

	if ( pmove_msec.integer < 8 ) {
		trap_Cvar_SetValue( "pmove_msec", 8 );
	}
	else if ( pmove_msec.integer > 33 ) {
		trap_Cvar_SetValue( "pmove_msec", 33 );
	}

	if ( pmove_fixed.integer || player->pers.pmoveFixed ) {
		ucmd->serverTime = ((ucmd->serverTime + pmove_msec.integer-1) / pmove_msec.integer) * pmove_msec.integer;
		//if (ucmd->serverTime - player->ps.commandTime <= 0)
		//	return;
	}

	//
	// check for exiting intermission
	//
	if ( level.intermissiontime ) {
		PlayerIntermissionThink( player );
		return;
	}

	// spectators don't do much
	if ( player->sess.sessionTeam == TEAM_SPECTATOR
#ifdef IOQ3ZTM // PEAKING
		|| ( G_AllowPeaking() && player->sess.spectatorState == SPECTATOR_FOLLOW )
#endif
		)
	{
#ifndef IOQ3ZTM
		if ( player->sess.spectatorState == SPECTATOR_SCOREBOARD) {
			return;
		}
#endif
		SpectatorThink( ent, ucmd );
		return;
	}

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	if ( !PlayerInactivityTimer( player ) ) {
		return;
	}

#ifdef TURTLEARENA // PLAYERS
	// switch to using waiting animation if idling for awhile.
	if (player->pers.cmd.forwardmove || player->pers.cmd.rightmove || player->pers.cmd.upmove
		|| (player->pers.cmd.buttons & BUTTON_USE_HOLDABLE) || (player->pers.cmd.buttons & BUTTON_ATTACK)
		|| player->damage_blood || (player->ps.pm_flags & PMF_TIME_KNOCKBACK)) {
		player->idleTime = level.time;
		player->ps.eFlags &= ~EF_PLAYER_WAITING;
	} else if (level.time > player->idleTime + TIME_BEFORE_WAITING_ANIMATION) {
		player->ps.eFlags |= EF_PLAYER_WAITING;
	}
#endif

	// clear the rewards if time
	if ( level.time > player->rewardTime ) {
#ifdef IOQ3ZTM
		player->ps.eFlags &= ~EF_AWARD_BITS;
#else
		player->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
#endif
	}

	if ( player->noclip ) {
		player->ps.pm_type = PM_NOCLIP;
	} else if ( player->ps.stats[STAT_HEALTH] <= 0 ) {
		player->ps.pm_type = PM_DEAD;
	} else {
		player->ps.pm_type = PM_NORMAL;
	}

	player->ps.gravity = g_gravity.value;

	// set speed
#ifdef TA_PLAYERSYS // PER_PLAYER_SPEED
	player->ps.speed = player->pers.playercfg.max_speed * (g_speed.value / 320);
#else
	player->ps.speed = g_speed.value;
#endif

#ifdef MISSIONPACK
	if( BG_ItemForItemNum( player->ps.stats[STAT_PERSISTANT_POWERUP] )->giTag == PW_SCOUT ) {
		player->ps.speed *= 1.5;
	}
	else
#endif
	if ( player->ps.powerups[PW_HASTE] ) {
		player->ps.speed *= 1.3;
	}

#ifdef TURTLEARENA // LOCKON
	if (player->ps.eFlags & EF_LOCKON)
	{
		// Walk faster forward like in LOZ:TP
		if (ucmd->forwardmove > 0) {
			player->ps.speed *= 1.5;
		} else {
			player->ps.speed /= 1.4;
		}
	}
#endif

	// Let go of the hook if we aren't firing
#ifdef TA_WEAPSYS
	if (player->hook) {
		qboolean weapon_grapple = player->hook->s.weapon == bg_weapongroupinfo[player->ps.weapon].weapon[0]->projnum
							|| player->hook->s.weapon == bg_weapongroupinfo[player->ps.weapon].weapon[1]->projnum;

		if ( (weapon_grapple && !(ucmd->buttons & BUTTON_ATTACK))
#ifdef TA_HOLDSYS // Allow shurikens to be grappling
			|| (!weapon_grapple && !(ucmd->buttons & BUTTON_USE_HOLDABLE))
#endif
			)
		{
			Weapon_HookFree(player->hook);
		}
	}
#else
	if ( player->ps.weapon == WP_GRAPPLING_HOOK &&
		player->hook && !( ucmd->buttons & BUTTON_ATTACK ) ) {
		Weapon_HookFree(player->hook);
	}
#endif

#ifdef TURTLEARENA // LOCKON
	// Set Vieworigin
	VectorCopy(player->ps.origin, vieworigin);
	vieworigin[2] += player->ps.viewheight;

	// Check if current target is valid.
	if (player->sess.sessionTeam == TEAM_SPECTATOR) {
		ent->enemy = NULL;
	} else {
		if (ent->enemy && (ent->enemy == ent || !ent->enemy->takedamage)) {
			ent->enemy = NULL;
		}

		if (!(player->ps.eFlags & EF_LOCKON)
			|| !G_ValidTarget(ent, ent->enemy, vieworigin, player->ps.viewangles, 768.0f, 90.0f, 2))
		{
			// Search for a new target
			ent->enemy = G_FindTarget(ent, vieworigin, player->ps.viewangles, 768.0f, 90.0f);
		}
	}

	// Set origin of target origin
	if (ent->enemy)
	{
		if (ent->enemy->player) {
			VectorCopy(ent->enemy->player->ps.origin, player->ps.enemyOrigin);
			player->ps.enemyOrigin[2] += ent->enemy->player->ps.viewheight;
		} else {
			VectorCopy(ent->enemy->r.currentOrigin, player->ps.enemyOrigin);
			player->ps.enemyOrigin[2] += ent->enemy->s.maxs[2] * 0.8f;
		}
		player->ps.enemyEnt = ent->enemy-g_entities;
	}
	else
	{
		VectorClear(player->ps.enemyOrigin);
		player->ps.enemyEnt = ENTITYNUM_NONE;
	}

	if (player->hadLockon != (player->ps.eFlags & EF_LOCKON))
	{
		player->hadLockon = (player->ps.eFlags & EF_LOCKON);
		player->lockonTime = level.time;
	}

	// A_FaceTarget!
	if ((player->ps.eFlags & EF_LOCKON) && ent->enemy
		&& ( level.time - player->lockonTime ) > LOCKON_TIME/2)
	{
		vec3_t dir;
		vec3_t viewAngles;
		vec3_t origin;

		VectorCopy(player->ps.origin, origin);
		origin[2] += player->ps.viewheight;

		VectorSubtract( player->ps.enemyOrigin, origin, dir );

		vectoangles( dir, viewAngles );

		BG_SwingAngles( viewAngles[YAW], 40, 90, BG_SWINGSPEED, &ent->player->pers.legs.yawAngle, &ent->player->pers.legs.yawing, (level.time - level.previousTime) );
		viewAngles[YAW] = ent->player->pers.legs.yawAngle;

		SetPlayerViewAngle(ent, viewAngles);
	}
#endif
#ifdef TA_PATHSYS
	if (player->ps.pathMode == PATHMODE_SIDE || player->ps.pathMode == PATHMODE_BACK)
	{
		// A_FaceTarget!
		if (ent->player->pers.cmd.rightmove != 0)
		{
			vec3_t dir;
			vec3_t viewAngles;

			if (ent->player->pers.cmd.rightmove < 0) { // LEFT, pervTrain
				player->ps.eFlags |= EF_TRAINBACKWARD;
				VectorSubtract( ent->pos1, player->ps.origin, dir );
			} else { // RIGHT, nextTrain
				player->ps.eFlags &= ~EF_TRAINBACKWARD;
				VectorSubtract( ent->pos2, player->ps.origin, dir );
			}
			
			dir[2] = 0;

			vectoangles( dir, viewAngles );

			//BG_SwingAngles( viewAngles[YAW], 40, 90, BG_SWINGSPEED, &ent->player->pers.legs.yawAngle, &ent->player->pers.legs.yawing, (level.time - level.previousTime) );
			//viewAngles[YAW] = ent->player->pers.legs.yawAngle;

			SetPlayerViewAngle(ent, viewAngles);
		}

		G_MoveOnPath(ent);
	}
#endif

	// set up for pmove
	oldEventSequence = player->ps.eventSequence;

	memset (&pm, 0, sizeof(pm));

#ifdef TA_WEAPSYS
	// Switch to single handed when close to CTF flag
	// ZTM: TODO: Don't allow CTF flag to be picked up while melee attacking?
	{
		qboolean holdingFlag = (player->ps.powerups[PW_BLUEFLAG]
								|| player->ps.powerups[PW_REDFLAG]
								|| player->ps.powerups[PW_NEUTRALFLAG]);

		// Switch to single handed
		if (holdingFlag || G_ByEnemyFlag(player->ps.persistant[PERS_TEAM], player->ps.origin))
		{
			// The Good: When byFlag and attacking, use both hands to attack.
			// The Bad: If standing by flag, secondary weapon is got out and
			//    put away before/after each attack...
			if (!holdingFlag && player->ps.meleeTime)
			{
				player->ps.eFlags &= ~EF_PRIMARY_HAND;
			}
			else
			{
				player->ps.eFlags |= EF_PRIMARY_HAND;
			}
		}
		else if (player->ps.eFlags & EF_PRIMARY_HAND)
		{
			// Don't change while melee attacking
			if (!player->ps.meleeTime)
			{
				player->ps.eFlags &= ~EF_PRIMARY_HAND;
			}
		}
	}
#endif

#ifdef TA_WEAPSYS // MELEEATTACK
	if (player->ps.pm_type < PM_DEAD && !(player->ps.pm_flags & PMF_RESPAWNED))
	{
		if (BG_WeaponHasType(player->ps.weapon, WT_GAUNTLET))
		{
			if ((ucmd->buttons & BUTTON_ATTACK) && !(ucmd->buttons & BUTTON_TALK)
				&& player->ps.weaponTime <= 0 )
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
				&& player->ps.meleeDelay <= 0 && player->ps.meleeTime <= 0
				&& player->ps.weaponTime <= 0)
			{
				G_StartMeleeAttack(ent);
			}

			G_MeleeDamage( ent, (player->ps.meleeTime > 0) );
		}
	}
#else
	// check for the hit-scan gauntlet, don't let the action
	// go through as an attack unless it actually hits something
	if ( player->ps.weapon == WP_GAUNTLET && !( ucmd->buttons & BUTTON_TALK ) &&
		( ucmd->buttons & BUTTON_ATTACK ) && player->ps.weaponTime <= 0 ) {
		pm.gauntletHit = CheckGauntletAttack( ent );
	}
#endif

	if ( ent->flags & FL_FORCE_GESTURE ) {
		ent->flags &= ~FL_FORCE_GESTURE;
		ent->player->pers.cmd.buttons |= BUTTON_GESTURE;
	}

#ifdef TA_ENTSYS // FUNC_USE
	useEnt = G_FindUseEntity(ent);

	if (useEnt)
	{
		ent->player->ps.eFlags |= EF_USE_ENT;

		if ((ucmd->buttons & BUTTON_USE_HOLDABLE) &&
#ifdef TURTLEARENA // HOLD_SHURIKEN
			ent->player->ps.holdableTime <= 0
#else
			! ( ent->player->ps.pm_flags & PMF_USE_ITEM_HELD )
#endif
			)
		{
			if (useEnt->use) {
				useEnt->use(useEnt, ent, ent);
			}
#ifdef TURTLEARENA // HOLD_SHURIKEN
			ent->player->ps.holdableTime = 500;
#else
			ent->player->ps.pm_flags |= PMF_USE_ITEM_HELD;
#endif
		}
	}
	else
	{
		ent->player->ps.eFlags &= ~EF_USE_ENT;
	}
#endif

#ifndef TURTLEARENA // POWERS
#ifdef MISSIONPACK
	// check for invulnerability expansion before doing the Pmove
	if (player->ps.powerups[PW_INVULNERABILITY] ) {
		if ( !(player->ps.pm_flags & PMF_INVULEXPAND) ) {
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
			if ( !StuckInOtherPlayer(ent) ) {
				// set flag so the expanded size will be set in PM_CheckDuck
				player->ps.pm_flags |= PMF_INVULEXPAND;
			}
			// set back
			VectorCopy (oldmins, ent->s.mins);
			VectorCopy (oldmaxs, ent->s.maxs);
			trap_LinkEntity(ent);
		}
	}
#endif
#endif

	pm.ps = &player->ps;
#ifdef TA_PLAYERSYS // PMove
	pm.playercfg = &player->pers.playercfg;
#endif
	pm.cmd = *ucmd;
	if ( pm.ps->pm_type == PM_DEAD ) {
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else if ( ent->r.svFlags & SVF_BOT ) {
		pm.tracemask = MASK_PLAYERSOLID | CONTENTS_BOTCLIP;
	}
	else {
		pm.tracemask = MASK_PLAYERSOLID;
	}
	if (player->ps.collisionType == CT_CAPSULE) {
		pm.trace = trap_TraceCapsule;
	} else {
		pm.trace = trap_Trace;
	}
	pm.pointcontents = trap_PointContents;
	pm.debugLevel = g_debugMove.integer;
	pm.noFootsteps = ( g_dmflags.integer & DF_NO_FOOTSTEPS ) > 0;

	pm.pmove_fixed = pmove_fixed.integer | player->pers.pmoveFixed;
	pm.pmove_msec = pmove_msec.integer;

	pm.pmove_overbounce = pmove_overbounce.integer;

#if 0 //#ifdef TA_ENTSYS // PUSHABLE
	if ( !player->noclip )
	{
		gentity_t *obstacle;
		vec3_t move;
		vec3_t amove;

		obstacle = NULL;

		VectorSubtract( player->ps.origin, player->oldOrigin, move );
		VectorClear( amove );
		amove[YAW] = player->ps.viewangles[YAW]-player->oldYaw;

		VectorCopy( player->oldOrigin, ent->r.currentOrigin );
		VectorCopy( player->ps.viewangles, ent->r.currentAngles );
		ent->r.currentAngles[YAW] -= amove[YAW];

		G_PlayerPush(ent, move, amove, &obstacle);
	}
	player->oldYaw = player->ps.viewangles[YAW];
#endif


	VectorCopy( player->ps.origin, player->oldOrigin );

#ifdef MISSIONPACK
	if ( level.intermissionQueued != 0 && g_singlePlayer.integer ) {
		if ( level.time - level.intermissionQueued >= 1000 ) {
			pm.cmd.buttons = 0;
			pm.cmd.forwardmove = 0;
			pm.cmd.rightmove = 0;
			pm.cmd.upmove = 0;
			if ( level.time - level.intermissionQueued >= 2000 && level.time - level.intermissionQueued <= 2500 ) {
				trap_Cmd_ExecuteText( EXEC_APPEND, "centerview\n" );
			}
			ent->player->ps.pm_type = PM_SPINTERMISSION;
		}
	}
#endif

	Pmove (&pm);

	// save results of pmove
	if ( ent->player->ps.eventSequence != oldEventSequence ) {
		ent->eventTime = level.time;
	}
	if (g_smoothClients.integer) {
		BG_PlayerStateToEntityStateExtraPolate( &ent->player->ps, &ent->s, ent->player->ps.commandTime, qtrue );
	}
	else {
		BG_PlayerStateToEntityState( &ent->player->ps, &ent->s, qtrue );
	}
	SendPendingPredictableEvents( &ent->player->ps );

	if ( !( ent->player->ps.eFlags & EF_FIRING ) ) {
#ifdef IOQ3ZTM
		player->ps.pm_flags &= ~PMF_FIRE_HELD;
#else
		player->fireHeld = qfalse;		// for grapple
#endif
	}

#ifdef TA_PLAYERSYS // LADDER
	// Stop grappling if grappled to a ladder
	if ((ent->player->ps.eFlags & EF_LADDER) && ent->player->hook) {
		Weapon_HookFree(ent->player->hook);
	}
#endif

	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );

	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;

	// execute player events
	PlayerEvents( ent, oldEventSequence );

	// link entity now, after any personal teleporters have been used
	trap_LinkEntity (ent);
	if ( !ent->player->noclip ) {
		G_TouchTriggers( ent );
	}

	// NOTE: now copy the exact origin over otherwise players can be snapped into solid
	VectorCopy( ent->player->ps.origin, ent->r.currentOrigin );

	//test for solid areas in the AAS file
	BotTestAAS(ent->r.currentOrigin);

	// touch other objects
	PlayerImpacts( ent, &pm );

	// save results of triggers and player events
	if (ent->player->ps.eventSequence != oldEventSequence) {
		ent->eventTime = level.time;
	}

	// swap and latch button actions
	player->oldbuttons = player->buttons;
	player->buttons = ucmd->buttons;
	player->latched_buttons |= player->buttons & ~player->oldbuttons;

	// check for respawning
	if ( player->ps.stats[STAT_HEALTH] <= 0 ) {
#ifdef TA_SP // ZTM: Single player death think
		// wait for the attack button to be pressed
		if (g_gametype.integer == GT_SINGLE_PLAYER)
		{
			if (!player->ps.persistant[PERS_LIVES] && !player->ps.persistant[PERS_CONTINUES])
			{
				// Game Over, don't respawn. In multiplayer you get three lives on the next level.
				player->respawnTime = -1;
			}

			// If not haven't Game Overed,
			//   auto respawn in 3 seconds, or if player pressed attack, use, or jump.
			if ( player->respawnTime > 0 && level.time > player->respawnTime &&
				((level.time - player->respawnTime > 3000)
					|| (ucmd->buttons & (BUTTON_ATTACK|BUTTON_USE_HOLDABLE)) || (ucmd->upmove > 0)) )
			{
				if (!player->ps.persistant[PERS_LIVES])
				{
					/*if (g_singlePlayer.integer && player->ps.pers[PERS_CONTINUES])
					{
						// ZTM: TODO: Open "Contiue?" menu/screen
					}
					else */if (player->ps.persistant[PERS_CONTINUES])
					{
						player->ps.persistant[PERS_CONTINUES]--;
						player->ps.persistant[PERS_LIVES] += 3;
						PlayerRespawn( ent );
					}
				}
				else
				{
					PlayerRespawn( ent );
				}
			}
			return;
		}
#endif
		// wait for the attack button to be pressed
		if ( level.time > player->respawnTime ) {
			// forcerespawn is to prevent users from waiting out powerups
			if ( g_forcerespawn.integer > 0 && 
				( level.time - player->respawnTime ) > g_forcerespawn.integer * 1000 ) {
				PlayerRespawn( ent );
				return;
			}
		
			// pressing attack or use is the normal respawn method
#ifdef TA_MISC
			if ( ( ucmd->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) ) || ucmd->upmove > 0 )
#else
			if ( ucmd->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) )
#endif
			{
				PlayerRespawn( ent );
			}
		}
		return;
	}

	// perform once-a-second actions
	PlayerTimerActions( ent, msec );
}

/*
==================
PlayerThink

A new command has arrived from the client
==================
*/
void PlayerThink( int playerNum ) {
	gentity_t *ent;

	ent = g_entities + playerNum;
	trap_GetUsercmd( playerNum, &ent->player->pers.cmd );

	// mark the time we got info, so we can display the
	// phone jack if they don't get any for a while
	ent->player->lastCmdTime = level.time;

	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
		PlayerThink_real( ent );
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

	VectorCopy( ent->player->ps.viewangles, headAngles );
	headAngles[YAW] = AngleMod( headAngles[YAW] );
	VectorClear( legsAngles );
	VectorClear( torsoAngles );

	// --------- yaw -------------

	// allow yaw to drift a bit
#ifdef TA_WEAPSYS
	if (!BG_PlayerStandAnim(&ent->player->pers.playercfg, AP_LEGS, ent->player->ps.legsAnim)
		|| !BG_PlayerStandAnim(&ent->player->pers.playercfg, AP_TORSO, ent->player->ps.torsoAnim))
#else
	if ( ( ent->player->ps.legsAnim & ~ANIM_TOGGLEBIT ) != LEGS_IDLE
		|| ( ent->player->ps.torsoAnim & ~ANIM_TOGGLEBIT ) != TORSO_STAND  )
#endif
	{
		// if not standing still, always point all in the same direction
		ent->player->pers.torso.yawing = qtrue;	// always center
		ent->player->pers.torso.pitching = qtrue;	// always center
		ent->player->pers.legs.yawing = qtrue;	// always center
	}

	// adjust legs for movement dir
	if ( ent->player->ps.eFlags & EF_DEAD ) {
		// don't let dead bodies twitch
		dir = 0;
	} else {
		dir = ent->player->ps.movementDir;
		if ( dir < 0 || dir > 7 ) {
			Com_Error(ERR_DROP, "G_PlayerAngles: Bad player movement angle" );
		}
	}
	legsAngles[YAW] = headAngles[YAW] + movementOffsets[ dir ];
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * movementOffsets[ dir ];

	// torso
	BG_SwingAngles( torsoAngles[YAW], 25, 90, BG_SWINGSPEED, &ent->player->pers.torso.yawAngle, &ent->player->pers.torso.yawing, frametime );
	BG_SwingAngles( legsAngles[YAW], 40, 90, BG_SWINGSPEED, &ent->player->pers.legs.yawAngle, &ent->player->pers.legs.yawing, frametime );

	torsoAngles[YAW] = ent->player->pers.torso.yawAngle;
	legsAngles[YAW] = ent->player->pers.legs.yawAngle;


	// --------- pitch -------------

#ifdef TA_PLAYERSYS // ZTM: If BOTH_* animation, don't have torso pitch
	if (ent->player->pers.playercfg.fixedtorso || (ent->player->ps.torsoAnim & ~ANIM_TOGGLEBIT) == (ent->player->ps.legsAnim & ~ANIM_TOGGLEBIT)) {
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
	BG_SwingAngles( dest, 15, 30, 0.1f, &ent->player->pers.torso.pitchAngle, &ent->player->pers.torso.pitching, frametime );
	torsoAngles[PITCH] = ent->player->pers.torso.pitchAngle;

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
	if ( ent->player->pers.playercfg.fixedlegs )
#else
	if ( qfalse )
#endif
	{
		legsAngles[YAW] = torsoAngles[YAW];
		legsAngles[PITCH] = 0.0f;
		legsAngles[ROLL] = 0.0f;
	}

#ifdef TA_PLAYERSYS // LADDER
	if (ent->player->ps.eFlags & EF_LADDER) {
		// Ladder dir, plaver legs should always face dir
		vectoangles(ent->player->ps.origin2, legsAngles);
		// If BOTH_* animation, have torso face ladder too
		if ((ent->player->ps.torsoAnim & ~ANIM_TOGGLEBIT) == (ent->player->ps.legsAnim & ~ANIM_TOGGLEBIT)) {
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

	if ( ent->player->pers.legs.yawing &&
#ifdef TA_WEAPSYS
		BG_PlayerStandAnim(&ent->player->pers.playercfg, AP_LEGS, ent->player->ps.legsAnim)
#else
		( ent->player->ps.legsAnim & ~ANIM_TOGGLEBIT ) == LEGS_IDLE
#endif
		)
		legsAnim = LEGS_TURN;
	else
		legsAnim = ent->player->ps.legsAnim;

	BG_RunLerpFrame( &ent->player->pers.legs,
#ifdef TA_PLAYERSYS
		ent->player->pers.playercfg.animations,
#else
#error "ERROR: TA_PLAYERSYS needs to be defined."
		NULL, // ZTM: This must be valid!
#endif
		legsAnim, level.time, ent->player->ps.powerups[PW_HASTE] ? 1.5f : 1.0f );

	BG_RunLerpFrame( &ent->player->pers.torso,
#ifdef TA_PLAYERSYS
		ent->player->pers.playercfg.animations,
#else
#error "ERROR: TA_PLAYERSYS needs to be defined."
		NULL, // ZTM: This must be valid!
#endif
		ent->player->ps.torsoAnim, level.time, ent->player->ps.powerups[PW_HASTE] ? 1.5f : 1.0f );
}
#endif

void G_RunPlayer( gentity_t *ent ) {
#ifdef TA_GAME_MODELS
	G_PlayerAngles(ent, ent->player->pers.legsAxis, ent->player->pers.torsoAxis, ent->player->pers.headAxis);

	G_PlayerAnimation(ent);
#endif
	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
		return;
	}
	ent->player->pers.cmd.serverTime = level.time;
	PlayerThink_real( ent );
}


/*
==================
SpectatorPlayerEndFrame

==================
*/
void SpectatorPlayerEndFrame( gentity_t *ent ) {
	gplayer_t	*cl;

	// if we are doing a chase cam or a remote view, grab the latest info
	if ( ent->player->sess.spectatorState == SPECTATOR_FOLLOW ) {
		int		playerNum, flags;

		playerNum = ent->player->sess.spectatorPlayer;

		// team follow1 and team follow2 go to whatever players are playing
		if ( playerNum == -1 ) {
			playerNum = level.follow1;
		} else if ( playerNum == -2 ) {
			playerNum = level.follow2;
		}
		if ( playerNum >= 0 ) {
			cl = &level.players[ playerNum ];
			if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR ) {
				flags = (cl->ps.eFlags & ~(EF_VOTED | EF_TEAMVOTED)) | (ent->player->ps.eFlags & (EF_VOTED | EF_TEAMVOTED));
				ent->player->ps = cl->ps;
				ent->player->ps.pm_flags |= PMF_FOLLOW;
				ent->player->ps.eFlags = flags;
				return;
			} else {
				// drop them to free spectators unless they are dedicated camera followers
				if ( ent->player->sess.spectatorPlayer >= 0 ) {
					ent->player->sess.spectatorState = SPECTATOR_FREE;
					PlayerBegin( ent->player - level.players );
				}
			}
		}
	}

	if ( ent->player->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
		ent->player->ps.pm_flags |= PMF_SCOREBOARD;
	} else {
		ent->player->ps.pm_flags &= ~PMF_SCOREBOARD;
	}
}

/*
==============
PlayerEndFrame

Called at the end of each server frame for each connected player
A fast client will have multiple PlayerThink for each PlayerEndFrame,
while a slow client may have multiple PlayerEndFrame between PlayerThink.
==============
*/
void PlayerEndFrame( gentity_t *ent ) {
	int			i;

	if ( ent->player->sess.sessionTeam == TEAM_SPECTATOR
#ifdef IOQ3ZTM // PEAKING
		|| ( G_AllowPeaking() && ent->player->sess.spectatorState == SPECTATOR_FOLLOW )
#endif
		)
	{
		SpectatorPlayerEndFrame( ent );
		return;
	}

	// turn off any expired powerups
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ent->player->ps.powerups[ i ] < level.time ) {
#ifdef NIGHTSMODE
			if (ent->player->ps.powerups[ i ] > 0
				&& i == PW_FLIGHT)
			{
				G_DeNiGHTSizePlayer(ent);
			}
			else
#endif
			ent->player->ps.powerups[ i ] = 0;
		}
	}

#ifdef MISSIONPACK
	// set powerup for player animation
	if( BG_ItemForItemNum( ent->player->ps.stats[STAT_PERSISTANT_POWERUP] )->giTag == PW_GUARD ) {
		ent->player->ps.powerups[PW_GUARD] = level.time;
	}
	if( BG_ItemForItemNum( ent->player->ps.stats[STAT_PERSISTANT_POWERUP] )->giTag == PW_SCOUT ) {
		ent->player->ps.powerups[PW_SCOUT] = level.time;
	}
	if( BG_ItemForItemNum( ent->player->ps.stats[STAT_PERSISTANT_POWERUP] )->giTag == PW_DOUBLER ) {
		ent->player->ps.powerups[PW_DOUBLER] = level.time;
	}
	if( BG_ItemForItemNum( ent->player->ps.stats[STAT_PERSISTANT_POWERUP] )->giTag == PW_AMMOREGEN ) {
		ent->player->ps.powerups[PW_AMMOREGEN] = level.time;
	}
#ifndef TURTLEARENA // POWERS
	if ( ent->player->invulnerabilityTime > level.time ) {
		ent->player->ps.powerups[PW_INVULNERABILITY] = level.time;
	}
#endif
#endif

	// save network bandwidth
#if 0
	if ( !g_synchronousClients->integer && ent->player->ps.pm_type == PM_NORMAL ) {
		// FIXME: this must change eventually for non-sync demo recording
		VectorClear( ent->player->ps.viewangles );
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
	if ( level.time - ent->player->lastCmdTime > 1000 ) {
		ent->player->ps.eFlags |= EF_CONNECTION;
	} else {
		ent->player->ps.eFlags &= ~EF_CONNECTION;
	}

	ent->player->ps.stats[STAT_HEALTH] = ent->health;	// FIXME: get rid of ent->health...

	G_SetPlayerSound (ent);

	// set the latest infor
	if (g_smoothClients.integer) {
		BG_PlayerStateToEntityStateExtraPolate( &ent->player->ps, &ent->s, ent->player->ps.commandTime, qtrue );
	}
	else {
		BG_PlayerStateToEntityState( &ent->player->ps, &ent->s, qtrue );
	}
	SendPendingPredictableEvents( &ent->player->ps );

	// store the client's position for backward reconciliation later
	G_StoreHistory( ent );

	// set the bit for the reachability area the player is currently in
//	i = trap_AAS_PointReachabilityAreaIndex( ent->player->ps.origin );
//	ent->player->areabits[i >> 3] |= 1 << (i & 7);
}


