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
// bg_misc.c -- both games misc functions, all completely stateless

#include "../qcommon/q_shared.h"
#include "bg_misc.h"

// entityState_t fields
#define	NETF(x) (size_t)&((entityState_t*)0)->x, 1
#define	NETA(x) (size_t)&((entityState_t*)0)->x, ARRAY_LEN( ((entityState_t*)0)->x )

vmNetField_t	bg_entityStateFields[] = 
{
{ NETF(pos.trTime), 32 },
{ NETF(pos.trBase[0]), 0 },
{ NETF(pos.trBase[1]), 0 },
{ NETF(pos.trDelta[0]), 0 },
{ NETF(pos.trDelta[1]), 0 },
{ NETF(pos.trBase[2]), 0 },
{ NETF(apos.trBase[1]), 0 },
{ NETF(pos.trDelta[2]), 0 },
{ NETF(apos.trBase[0]), 0 },
{ NETF(event), 10 },
{ NETF(angles2[1]), 0 },
{ NETF(eType), 8 },
{ NETF(torsoAnim), 8 },
{ NETF(eventParm), 8 },
{ NETF(legsAnim), 8 },
{ NETF(groundEntityNum), GENTITYNUM_BITS },
{ NETF(pos.trType), 8 },
{ NETF(eFlags), 32 },
{ NETF(otherEntityNum), GENTITYNUM_BITS },
{ NETF(weapon), 8 },
{ NETF(clientNum), 8 },
{ NETF(angles[1]), 0 },
{ NETF(pos.trDuration), 32 },
{ NETF(apos.trType), 8 },
{ NETF(origin[0]), 0 },
{ NETF(origin[1]), 0 },
{ NETF(origin[2]), 0 },
{ NETF(contents), 32 },
{ NETF(bmodel), 1 },
{ NETF(capsule), 1 },
{ NETF(mins[0]), 0 },
{ NETF(mins[1]), 0 },
{ NETF(mins[2]), 0 },
{ NETF(maxs[0]), 0 },
{ NETF(maxs[1]), 0 },
{ NETF(maxs[2]), 0 },
{ NETF(powerups), MAX_POWERUPS },
{ NETF(modelindex), 8 },
{ NETF(otherEntityNum2), GENTITYNUM_BITS },
{ NETF(loopSound), 8 },
{ NETF(tokens), 8 },
{ NETF(team), 8 },
#ifdef TURTLEARENA
{ NETF(generic1), 8 },
#endif
{ NETF(origin2[2]), 0 },
{ NETF(origin2[0]), 0 },
{ NETF(origin2[1]), 0 },
{ NETF(modelindex2), 8 },
{ NETF(angles[0]), 0 },
{ NETF(time), 32 },
{ NETF(apos.trTime), 32 },
{ NETF(apos.trDuration), 32 },
{ NETF(apos.trBase[2]), 0 },
{ NETF(apos.trDelta[0]), 0 },
{ NETF(apos.trDelta[1]), 0 },
{ NETF(apos.trDelta[2]), 0 },
{ NETF(time2), 32 },
{ NETF(angles[2]), 0 },
{ NETF(angles2[0]), 0 },
{ NETF(angles2[2]), 0 },
{ NETF(constantLight), 32 },
{ NETF(frame), 16 }
#ifdef TA_WEAPSYS
,{ NETF(weaponHands), 4 }
#endif
#ifdef IOQ3ZTM_NO_COMPAT // DAMAGE_SKINS
,{ NETF(skinFraction), 0 }
#endif
};

int bg_numEntityStateFields = ARRAY_LEN(bg_entityStateFields);

// playerState_t fields
#define	PSF(x) (size_t)&((playerState_t*)0)->x, 1
#define	PSA(x) (size_t)&((playerState_t*)0)->x, ARRAY_LEN( ((playerState_t*)0)->x )

vmNetField_t	bg_playerStateFields[] = 
{
{ PSF(commandTime), 32 },				
{ PSF(origin[0]), 0 },
{ PSF(origin[1]), 0 },
{ PSF(bobCycle), 8 },
{ PSF(velocity[0]), 0 },
{ PSF(velocity[1]), 0 },
{ PSF(viewangles[1]), 0 },
{ PSF(viewangles[0]), 0 },
{ PSF(weaponTime), -16 },
{ PSF(origin[2]), 0 },
{ PSF(velocity[2]), 0 },
{ PSF(legsTimer), 8 },
{ PSF(pm_time), -16 },
{ PSF(eventSequence), 16 },
{ PSF(torsoAnim), 8 },
{ PSF(movementDir), 4 },
{ PSF(events[0]), 8 },
{ PSF(legsAnim), 8 },
{ PSF(events[1]), 8 },
{ PSF(pm_flags), 16 },
{ PSF(groundEntityNum), GENTITYNUM_BITS },
{ PSF(weaponstate), 4 },
{ PSF(eFlags), 32 },
{ PSA(stats), -16 },
{ PSA(persistant), -16 },
#ifndef TA_WEAPSYS_EX
{ PSA(ammo), -16 },
#endif
{ PSA(powerups), 32 },
#ifdef TA_HOLDSYS
{ PSA(holdable), -16 },
#endif
{ PSF(contents), 32 },
{ PSF(capsule), 1 },
{ PSF(linked), 1 },
{ PSF(externalEvent), 10 },
{ PSF(gravity), 16 },
{ PSF(speed), 16 },
{ PSF(delta_angles[1]), 16 },
{ PSF(externalEventParm), 8 },
{ PSF(viewheight), -8 },
{ PSF(damageEvent), 8 },
{ PSF(damageYaw), 8 },
{ PSF(damagePitch), 8 },
{ PSF(damageCount), 8 },
{ PSF(tokens), 8 },
{ PSF(pm_type), 8 },					
{ PSF(delta_angles[0]), 16 },
{ PSF(delta_angles[2]), 16 },
{ PSF(torsoTimer), 12 },
{ PSF(eventParms[0]), 8 },
{ PSF(eventParms[1]), 8 },
{ PSF(clientNum), 8 },
{ PSF(weapon), WEAPONNUM_BITS },
{ PSF(viewangles[2]), 0 },
{ PSF(grapplePoint[0]), 0 },
{ PSF(grapplePoint[1]), 0 },
{ PSF(grapplePoint[2]), 0 },
#ifdef TA_PATHSYS
{ PSF(nextPoint[0]), 0 },
{ PSF(nextPoint[1]), 0 },
{ PSF(nextPoint[2]), 0 },
#endif
{ PSF(jumppad_ent), GENTITYNUM_BITS },
{ PSF(loopSound), 16 },
{ PSF(mins[0]), 0 },
{ PSF(mins[1]), 0 },
{ PSF(mins[2]), 0 },
{ PSF(maxs[0]), 0 },
{ PSF(maxs[1]), 0 },
{ PSF(maxs[2]), 0 }
#ifdef TA_HOLDSYS
,{ PSF(holdableIndex), HOLDABLENUM_BITS }
#endif
#ifdef TURTLEARENA // HOLD_SHURIKEN
,{ PSF(holdableTime), -16 }
#endif
#ifdef TURTLEARENA // LOCKON
,{ PSF(enemyEnt), GENTITYNUM_BITS },
{ PSF(enemyOrigin[0]), 0 },
{ PSF(enemyOrigin[1]), 0 },
{ PSF(enemyOrigin[2]), 0 }
#endif
#ifdef TA_WEAPSYS // MELEEATTACK
,{ PSF(meleeAttack), 8 },
{ PSF(meleeTime), 16 },
{ PSF(meleeDelay), 16 },
{ PSF(meleeLinkTime), 16 },
{ PSF(chain), 16 },
{ PSF(chainTime), 16 },
{ PSF(weaponHands), 4 }
#endif
#ifdef TA_PLAYERSYS // LADDER
,{ PSF(origin2[2]), 0 },
{ PSF(origin2[0]), 0 },
{ PSF(origin2[1]), 0 }
#endif
#ifdef TA_PATHSYS // 2DMODE
,{ PSF(pathMode), 8 }
#endif
};

int bg_numPlayerStateFields = ARRAY_LEN(bg_playerStateFields);

char	*modNames[MOD_MAX] = {
	"MOD_UNKNOWN",
#ifndef TURTLEARENA // MOD
	"MOD_SHOTGUN",
	"MOD_GAUNTLET",
	"MOD_MACHINEGUN",
	"MOD_GRENADE",
	"MOD_GRENADE_SPLASH",
	"MOD_ROCKET",
	"MOD_ROCKET_SPLASH",
	"MOD_PLASMA",
	"MOD_PLASMA_SPLASH",
	"MOD_RAILGUN",
	"MOD_LIGHTNING",
	"MOD_BFG",
	"MOD_BFG_SPLASH",
#endif
	"MOD_WATER",
	"MOD_SLIME",
	"MOD_LAVA",
	"MOD_CRUSH",
	"MOD_TELEFRAG",
	"MOD_FALLING",
	"MOD_SUICIDE",
#ifdef IOQ3ZTM // SWITCH_TEAM
	"MOD_SUICIDE_TEAM_CHANGE",
#endif
	"MOD_TARGET_LASER",
	"MOD_TRIGGER_HURT",
#ifdef TA_ENTSYS
	"MOD_EXPLOSION",
#endif
#ifdef MISSIONPACK
#ifndef TURTLEARENA // MOD NO_KAMIKAZE_ITEM POWERS
	"MOD_NAIL",
	"MOD_CHAINGUN",
	"MOD_PROXIMITY_MINE",
	"MOD_KAMIKAZE",
	"MOD_JUICED",
#endif
#endif
	"MOD_GRAPPLE",
#ifdef TA_WEAPSYS
	"MOD_PROJECTILE",
	"MOD_PROJECTILE_EXPLOSION",
	"MOD_WEAPON_PRIMARY",
	"MOD_WEAPON_SECONDARY"
#endif
};
int modNamesSize = ARRAY_LEN( modNames );

#ifdef TA_MISC // MATERIALS
materialInfo_t materialInfo[NUM_MATERIAL_TYPES] = {
	{"none",	0 },
	{"dirt",	SURF_DIRT },
	{"grass",	SURF_GRASS },
	{"wood",	SURF_WOOD },
	{"stone",	SURF_STONE },
	{"sparks",	SURF_SPARKS },
	{"glass",	SURF_GLASS },
	{"metal",	SURF_METAL }
};
#endif

/*
==============
BG_CheckSpawnEntity
==============
*/
qboolean BG_CheckSpawnEntity( const bgEntitySpawnInfo_t *info ) {
	int			i, gametype;
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

	gametype = info->gametype;

	// check for "notsingle" flag
#ifdef TA_SP
	if ( info->singlePlayerActive && gametype == GT_SINGLE_PLAYER )
#else
	if ( gametype == GT_SINGLE_PLAYER )
#endif
	{
		info->spawnInt( "notsingle", "0", &i );
		if ( i ) {
			return qfalse;
		}
	}

	// check for "notteam" flag (GT_FFA, GT_TOURNAMENT, GT_SINGLE_PLAYER)
	if ( gametype >= GT_TEAM ) {
		info->spawnInt( "notteam", "0", &i );
		if ( i ) {
			return qfalse;
		}
	} else {
		info->spawnInt( "notfree", "0", &i );
		if ( i ) {
			return qfalse;
		}
	}

#ifdef TURTLEARENA
	info->spawnInt( "notturtlearena", "0", &i );
	if ( i ) {
			return qfalse;
	}
#else
#ifdef MISSIONPACK
	info->spawnInt( "notta", "0", &i );
	if ( i ) {
			return qfalse;
	}
#else
	info->spawnInt( "notq3a", "0", &i );
	if ( i ) {
			return qfalse;
	}
#endif
#endif

#ifdef TA_SP // ZTM: Support single player and coop separately.
	if ( info->singlePlayerActive && gametype == GT_SINGLE_PLAYER )
		gametypeName = "single";
	else if ( gametype >= 0 && gametype < GT_MAX_GAME_TYPE ) {
		gametypeName = gametypeNames[gametype];
	} else {
		gametypeName = NULL;
	}
#endif

	if( info->spawnString( "!gametype", NULL, &value ) ) {
#ifndef TA_SP
		if( gametype >= 0 && gametype < GT_MAX_GAME_TYPE ) {
			gametypeName = gametypeNames[gametype];
#endif

			s = strstr( value, gametypeName );
			if( s ) {
				return qfalse;
			}
#ifndef TA_SP
		}
#endif
	}

	if( info->spawnString( "gametype", NULL, &value ) ) {
#ifndef TA_SP
		if( gametype >= 0 && gametype < GT_MAX_GAME_TYPE ) {
			gametypeName = gametypeNames[gametype];
#endif

			s = strstr( value, gametypeName );
			if( !s ) {
				return qfalse;
			}
#ifndef TA_SP
		}
#endif
	}

	return qtrue;
}

#ifdef TA_HOLDSYS
/*
==============
BG_ItemNumForHoldableNum

Returns the item number of the holdable item
Returns 0 if not found.
==============
*/
int BG_ItemNumForHoldableNum(holdable_t holdablenum)
{
	bg_iteminfo_t	*it;
	int i;

	for (i = BG_NumItems()-1; i > 0; i--)
	{
		it = BG_ItemForItemNum(i);
		if (!it->classname[0])
			continue;
		if ( it->giType == IT_HOLDABLE
			&& it->giTag == holdablenum )
		{
			return i;
		}
	}
	return 0;
}
#endif

/*
==============
BG_FindItemForPowerup
==============
*/
bg_iteminfo_t	*BG_FindItemForPowerup( powerup_t pw ) {
	bg_iteminfo_t	*it;
	int i;

	for (i = BG_NumItems()-1; i > 0; i--)
	{
		it = BG_ItemForItemNum(i);
		if (!it->classname[0])
			continue;
		if ( (it->giType == IT_POWERUP
				|| it->giType == IT_TEAM
				|| it->giType == IT_PERSISTANT_POWERUP)
			&& it->giTag == pw )
		{
			return it;
		}
	}

	return NULL;
}


/*
==============
BG_FindItemForHoldable
==============
*/
bg_iteminfo_t	*BG_FindItemForHoldable( holdable_t pw ) {
	bg_iteminfo_t	*it;
	int i;

	for (i = BG_NumItems()-1; i > 0; i--)
	{
		it = BG_ItemForItemNum(i);
		if (!it->classname[0])
			continue;
		if ( it->giType == IT_HOLDABLE && it->giTag == pw ) {
			return it;
		}
	}

	return NULL;
}


/*
===============
BG_FindItemForWeapon

===============
*/
bg_iteminfo_t	*BG_FindItemForWeapon( weapon_t weapon ) {
	bg_iteminfo_t	*it;
	int i;

	for (i = BG_NumItems()-1; i > 0; i--)
	{
		it = BG_ItemForItemNum(i);
		if (!it->classname[0])
			continue;
		if ( it->giType == IT_WEAPON && it->giTag == weapon ) {
			return it;
		}
	}

	return NULL;
}

/*
===============
BG_FindItem

===============
*/
bg_iteminfo_t	*BG_FindItem( const char *pickupName ) {
	bg_iteminfo_t	*it;
	int i;

	for (i = BG_NumItems()-1; i > 0; i--)
	{
		it = BG_ItemForItemNum(i);
		if (!it->classname[0])
			continue;
		if ( !Q_stricmp( it->pickup_name, pickupName ) )
			return it;
	}

	return NULL;
}

#ifdef IOQ3ZTM
/*
===============
BG_FindItemForClassname
===============
*/
bg_iteminfo_t	*BG_FindItemForClassname( const char *classname ) {
	bg_iteminfo_t	*it;
	int i;

	for (i = BG_NumItems()-1; i > 0; i--)
	{
		it = BG_ItemForItemNum(i);
		if (!it->classname[0])
			continue;
		if ( !Q_stricmp( it->classname, classname ) )
			return it;
	}

	return NULL;
}
#endif

/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds to make
grabbing them easier
============
*/
qboolean	BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime ) {
	vec3_t		origin;

	BG_EvaluateTrajectory( &item->pos, atTime, origin );

	// we are ignoring ducked differences here
	if ( ps->origin[0] - origin[0] > 44
		|| ps->origin[0] - origin[0] < -50
		|| ps->origin[1] - origin[1] > 36
		|| ps->origin[1] - origin[1] < -36
		|| ps->origin[2] - origin[2] > 36
		|| ps->origin[2] - origin[2] < -36 ) {
		return qfalse;
	}

	return qtrue;
}



/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/
qboolean BG_CanItemBeGrabbed( int gametype, const entityState_t *ent, const playerState_t *ps ) {
	bg_iteminfo_t	*item;
#if defined MISSIONPACK || defined TA_HOLDSYS
	int		upperBound;
#endif

	if ( ent->modelindex < 0 || ent->modelindex >= BG_NumItems() ) {
		Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: index out of range" );
	}

	item = BG_ItemForItemNum(ent->modelindex);

#ifdef IOQ3ZTM // DROP_ITEM_FIX
	// If it was dropped by this player and is still in their Bounding Box
	if (ent->modelindex2 == 1 && ent->generic1-1 == ps->clientNum) {
		//Com_Printf("DEBUG: Player touched item they can't pickup!\n");
		return qfalse;
	}
#endif

	switch( item->giType ) {
	case IT_WEAPON:
#ifdef TA_WEAPSYS_EX
		if (ps->weapon != ps->stats[STAT_PENDING_WEAPON])
		{
			return qfalse;
		}

		// AUTO_DROP_WEAPON
		//if (ps->weapon != ps->stats[STAT_DEFAULTWEAPON]
		//	&& ps->weapon != item->giTag
		//	&& !(ps->pm_flags & PMF_GRAPPLE_PULL)) {
		//	return qfalse;
		//}

		// If using/dropping/picking up a weapon
		if (ps->weaponTime || ps->meleeTime) {
			//Com_Printf("DEBUG: Player touched weapon they can't pickup!\n");
			return qfalse;
		}
#endif
#ifdef TA_WEAPSYS
		// Don't pick up a melee weapon (or another weapon that doesn't use ammo)
		//   if your holding one of the same type.
		if (item->giTag == WP_DEFAULT)
		{
			if (ps->weapon == ps->stats[STAT_DEFAULTWEAPON]
				&& !BG_WeapUseAmmo(ps->stats[STAT_DEFAULTWEAPON]))
			{
				return qfalse;
			}
		}
		else if (ps->weapon == item->giTag && !BG_WeapUseAmmo(item->giTag))
		{
			return qfalse;
		}
#endif

		return qtrue;	// weapons are always picked up

	case IT_AMMO:
#ifdef TA_WEAPSYS_EX
		{
			int stat;

			if (item->giTag == ps->weapon)
				stat = STAT_AMMO;
			else if (item->giTag == ps->stats[STAT_PENDING_WEAPON])
				stat = STAT_PENDING_AMMO;
			else if (item->giTag == ps->stats[STAT_DROP_WEAPON])
				stat = STAT_DROP_AMMO;
			else
				return qfalse; // no where to put ammo.

			if ( ps->stats[ stat ] >= 200 ) {
				return qfalse;		// can't hold any more
			}
		}
#else
		if ( ps->ammo[ item->giTag ] >= 200 ) {
			return qfalse;		// can't hold any more
		}
#endif
		return qtrue;

#ifdef TURTLEARENA // NIGHTS_ITEMS
	case IT_SCORE:
        return qtrue;
#endif

#ifndef TURTLEARENA // NOARMOR
	case IT_ARMOR:
#ifdef MISSIONPACK
		if( BG_ItemForItemNum(ps->stats[STAT_PERSISTANT_POWERUP])->giTag == PW_SCOUT ) {
			return qfalse;
		}

		// we also clamp armor to the maxhealth for handicapping
		if( BG_ItemForItemNum(ps->stats[STAT_PERSISTANT_POWERUP])->giTag == PW_GUARD ) {
			upperBound = ps->stats[STAT_MAX_HEALTH];
		}
		else {
			upperBound = ps->stats[STAT_MAX_HEALTH] * 2;
		}

		if ( ps->stats[STAT_ARMOR] >= upperBound ) {
			return qfalse;
		}
#else
		if ( ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_HEALTH] * 2 ) {
			return qfalse;
		}
#endif
		return qtrue;
#endif

	case IT_HEALTH:
		// small and mega healths will go over the max, otherwise
		// don't pick up if already at max
#ifdef MISSIONPACK
		if( BG_ItemForItemNum(ps->stats[STAT_PERSISTANT_POWERUP])->giTag == PW_GUARD ) {
			upperBound = ps->stats[STAT_MAX_HEALTH];
		}
		else
#endif
#ifndef TURTLEARENA // Treat all health items the same
		if ( item->quantity == 5 || item->quantity == 100 )
#endif
		{
			if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] * 2 ) {
				return qfalse;
			}
			return qtrue;
		}

		if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] ) {
			return qfalse;
		}
		return qtrue;

	case IT_POWERUP:
#if defined IOQ3ZTM && defined MISSIONPACK // Scout overrides haste, so don't pick it up.
		if (item->giTag == PW_HASTE && BG_ItemForItemNum(ps->stats[STAT_PERSISTANT_POWERUP])->giTag == PW_SCOUT ) {
			return qfalse;
		}
#endif
		return qtrue;	// powerups are always picked up

#ifdef MISSIONPACK
	case IT_PERSISTANT_POWERUP:
		// can only hold one item at a time
		if ( ps->stats[STAT_PERSISTANT_POWERUP] ) {
			return qfalse;
		}

		// check team only
		if( ent->team != 255 && ( ps->persistant[PERS_TEAM] != ent->team ) ) {
			return qfalse;
		}

		return qtrue;
#endif

	case IT_TEAM: // team items, such as flags
#ifdef MISSIONPACK		
		if( gametype == GT_1FCTF ) {
			// neutral flag can always be picked up
			if( item->giTag == PW_NEUTRALFLAG ) {
				return qtrue;
			}
			if (ps->persistant[PERS_TEAM] == TEAM_RED) {
				if (item->giTag == PW_BLUEFLAG  && ps->powerups[PW_NEUTRALFLAG] ) {
					return qtrue;
				}
			} else if (ps->persistant[PERS_TEAM] == TEAM_BLUE) {
				if (item->giTag == PW_REDFLAG  && ps->powerups[PW_NEUTRALFLAG] ) {
					return qtrue;
				}
			}
		}
#endif
		if( gametype == GT_CTF ) {
			// ent->modelindex2 is non-zero on items if they are dropped
			// we need to know this because we can pick up our dropped flag (and return it)
			// but we can't pick up our flag at base
			if (ps->persistant[PERS_TEAM] == TEAM_RED) {
				if (item->giTag == PW_BLUEFLAG ||
					(item->giTag == PW_REDFLAG && ent->modelindex2) ||
					(item->giTag == PW_REDFLAG && ps->powerups[PW_BLUEFLAG]) )
					return qtrue;
			} else if (ps->persistant[PERS_TEAM] == TEAM_BLUE) {
				if (item->giTag == PW_REDFLAG ||
					(item->giTag == PW_BLUEFLAG && ent->modelindex2) ||
					(item->giTag == PW_BLUEFLAG && ps->powerups[PW_REDFLAG]) )
					return qtrue;
			}
		}

#ifdef MISSIONPACK_HARVESTER
		if( gametype == GT_HARVESTER ) {
			return qtrue;
		}
#endif
		return qfalse;

	case IT_HOLDABLE:
#ifdef TA_HOLDSYS
		// Single use holdable item
		if (item->quantity == 0)
		{
			upperBound = 1;
		}
		else
		{
			// Multiple use holdable item
			upperBound = MAX_SHURIKENS;
		}

		// Check use limit.
		if (ps->holdable[item->giTag] >= upperBound)
		{
			return qfalse;
		}
#else
		// can only hold one item at a time
		if ( ps->stats[STAT_HOLDABLE_ITEM] ) {
			return qfalse;
		}
#endif
		return qtrue;

        case IT_BAD:
            Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD" );
        default:
#ifndef Q3_VM
#ifndef NDEBUG
          Com_Printf("BG_CanItemBeGrabbed: unknown enum %d\n", item->giType );
#endif
#endif
         break;
	}

	return qfalse;
}

//======================================================================

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result ) {
	float		deltaTime;
	float		phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorCopy( tr->trBase, result );
		break;
	case TR_LINEAR:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = sin( deltaTime * M_PI * 2 );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		if ( deltaTime < 0 ) {
			deltaTime = 0;
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;		// FIXME: local gravity...
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trType );
		break;
	}
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result ) {
	float	deltaTime;
	float	phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear( result );
		break;
	case TR_LINEAR:
		VectorCopy( tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = cos( deltaTime * M_PI * 2 );	// derivative of sin = cos
		phase *= 0.5;
		VectorScale( tr->trDelta, phase, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		VectorCopy( tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= DEFAULT_GRAVITY * deltaTime;		// FIXME: local gravity...
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trType );
		break;
	}
}

char *eventnames[] = {
	"EV_NONE",

	"EV_FOOTSTEP",
	"EV_FOOTSTEP_METAL",
	"EV_FOOTSPLASH",
	"EV_FOOTWADE",
	"EV_SWIM",

	"EV_STEP_4",
	"EV_STEP_8",
	"EV_STEP_12",
	"EV_STEP_16",

	"EV_FALL_SHORT",
	"EV_FALL_MEDIUM",
	"EV_FALL_FAR",

	"EV_JUMP_PAD",			// boing sound at origin", jump sound on player

	"EV_JUMP",
	"EV_WATER_TOUCH",	// foot touches
	"EV_WATER_LEAVE",	// foot leaves
	"EV_WATER_UNDER",	// head touches
	"EV_WATER_CLEAR",	// head leaves

	"EV_ITEM_PICKUP",			// normal item pickups are predictable
	"EV_GLOBAL_ITEM_PICKUP",	// powerup / team sounds are broadcast to everyone

#ifdef TA_WEAPSYS_EX
	"EV_DROP_WEAPON",
#else
	"EV_NOAMMO",
#endif
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",

	"EV_USE_ITEM0",
	"EV_USE_ITEM1",
	"EV_USE_ITEM2",
	"EV_USE_ITEM3",
	"EV_USE_ITEM4",
	"EV_USE_ITEM5",
	"EV_USE_ITEM6",
	"EV_USE_ITEM7",
	"EV_USE_ITEM8",
	"EV_USE_ITEM9",
	"EV_USE_ITEM10",
	"EV_USE_ITEM11",
	"EV_USE_ITEM12",
	"EV_USE_ITEM13",
	"EV_USE_ITEM14",
	"EV_USE_ITEM15",

	"EV_ITEM_RESPAWN",
	"EV_ITEM_POP",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",

#ifdef TA_WEAPSYS
	"EV_PROJECTILE_BOUNCE",
	"EV_PROJECTILE_STICK",
	"EV_PROJECTILE_TRIGGER",
#else
	"EV_GRENADE_BOUNCE",		// eventParm will be the soundindex
#endif

	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",		// no attenuation
	"EV_GLOBAL_TEAM_SOUND",

#ifndef TA_WEAPSYS
	"EV_BULLET_HIT_FLESH",
	"EV_BULLET_HIT_WALL",
#endif

	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_MISSILE_MISS_METAL",
	"EV_RAILTRAIL",
#ifndef TA_WEAPSYS
	"EV_SHOTGUN",
#endif

#ifdef TA_WEAPSYS
	"EV_WEAPON_HIT",
	"EV_WEAPON_MISS",
	"EV_WEAPON_MISS_METAL",
#endif

	"EV_PAIN",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_OBITUARY",

	"EV_POWERUP_QUAD",
	"EV_POWERUP_BATTLESUIT",
	"EV_POWERUP_REGEN",
#ifndef TURTLEARENA // POWERS
	"EV_POWERUP_INVUL",
#endif

#ifndef NOTRATEDM // No gibs.
	"EV_GIB_PLAYER",			// gib a previously living player
#endif
	"EV_SCOREPLUM",			// score plum
#ifdef TURTLEARENA // NIGHTS_ITEMS
	"EV_CHAINPLUM",
#endif
#ifdef TA_ENTSYS // BREAKABLE MISC_OBJECT
	"EV_SPAWN_DEBRIS",
	"EV_EXPLOSION",
#endif

//#ifdef MISSIONPACK
#ifndef TA_WEAPSYS
	"EV_PROXIMITY_MINE_STICK",
	"EV_PROXIMITY_MINE_TRIGGER",
#endif
#ifndef TURTLEARENA // NO_KAMIKAZE_ITEM
	"EV_KAMIKAZE",			// kamikaze explodes
#endif
	"EV_OBELISKEXPLODE",		// obelisk explodes
	"EV_OBELISKPAIN",		// obelisk pain
#ifndef TURTLEARENA // POWERS
	"EV_INVUL_IMPACT",		// invulnerability sphere impact
	"EV_JUICED",				// invulnerability juiced effect
	"EV_LIGHTNINGBOLT",		// lightning bolt bounced of invulnerability sphere
#endif
//#endif

	"EV_DEBUG_LINE",
#ifdef IOQ3ZTM // DEBUG_ORIGIN
	"EV_DEBUG_ORIGIN",
#endif
	"EV_STOPLOOPINGSOUND",
	"EV_TAUNT",
	"EV_TAUNT_YES",
	"EV_TAUNT_NO",
	"EV_TAUNT_FOLLOWME",
	"EV_TAUNT_GETFLAG",
	"EV_TAUNT_GUARDBASE",
	"EV_TAUNT_PATROL"

};

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void	trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps ) {

#ifdef _DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer("showevents", buf, sizeof(buf));
		if ( atof(buf) != 0 ) {
#ifdef GAME
			Com_Printf(" game event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#else
			Com_Printf("Cgame event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#endif
		}
	}
#endif
	ps->events[ps->eventSequence & (MAX_PS_EVENTS-1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS-1)] = eventParm;
	ps->eventSequence++;
}

/*
========================
BG_TouchJumpPad
========================
*/
void BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad ) {
	vec3_t	angles;
	float p;
	int effectNum;

	// spectators don't use jump pads
	if ( ps->pm_type != PM_NORMAL ) {
		return;
	}

	// flying characters don't hit bounce pads
	if ( ps->powerups[PW_FLIGHT] ) {
		return;
	}

	// if we didn't hit this same jumppad the previous frame
	// then don't play the event sound again if we are in a fat trigger
	if ( ps->jumppad_ent != jumppad->number ) {

		vectoangles( jumppad->origin2, angles);
		p = fabs( AngleNormalize180( angles[PITCH] ) );
		if( p < 45 ) {
			effectNum = 0;
		} else {
			effectNum = 1;
		}
		BG_AddPredictableEventToPlayerstate( EV_JUMP_PAD, effectNum, ps );
	}
	// remember hitting this jumppad this frame
	ps->jumppad_ent = jumppad->number;
	ps->jumppad_frame = ps->pmove_framecount;
	// give the player the velocity from the jumppad
	VectorCopy( jumppad->origin2, ps->velocity );
}

/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap ) {
	int		i;

	if ( !ps->linked ) {
		s->eType = ET_INVISIBLE;
#ifndef NOTRATEDM // No gibs.
	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
#endif
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;
	VectorCopy( ps->origin, s->pos.trBase );
	if ( snap ) {
		SnapVector( s->pos.trBase );
	}
	// set the trDelta for flag direction
	VectorCopy( ps->velocity, s->pos.trDelta );

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int		seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

	s->contents = ps->contents;
	s->loopSound = ps->loopSound;
	s->tokens = ps->tokens;
	s->team = ps->persistant[PERS_TEAM];

	s->bmodel = qfalse;
	s->capsule = ps->capsule;

	VectorCopy( ps->mins, s->mins );
	VectorCopy( ps->maxs, s->maxs );
	if ( snap ) {
		SnapVector( s->mins );
		SnapVector( s->maxs );
	}

#ifdef TA_WEAPSYS
	// cgame needs the weaponHands for all clients.
	s->weaponHands = ps->weaponHands;
#endif
#ifdef IOQ3ZTM_NO_COMPAT // DAMAGE_SKINS
	if (ps->stats[STAT_HEALTH] <= 0) {
		s->skinFraction = 1.0f;
	} else if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH]) {
		s->skinFraction = 0.0f;
	} else {
		s->skinFraction = 1.0f - ((float)ps->stats[STAT_HEALTH] / (float)ps->stats[STAT_MAX_HEALTH]);
	}
#endif
#ifdef TA_PLAYERSYS // LADDER
	VectorCopy( ps->origin2, s->origin2 );
#endif
}

/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap ) {
	int		i;

	if ( !ps->linked ) {
		s->eType = ET_INVISIBLE;
#ifndef NOTRATEDM // No gibs.
	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
#endif
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;
	VectorCopy( ps->origin, s->pos.trBase );
	if ( snap ) {
		SnapVector( s->pos.trBase );
	}
	// set the trDelta for flag direction and linear prediction
	VectorCopy( ps->velocity, s->pos.trDelta );
	// set the time for linear prediction
	s->pos.trTime = time;
	// set maximum extra polation time
	s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int		seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

	s->contents = ps->contents;
	s->loopSound = ps->loopSound;
	s->tokens = ps->tokens;
	s->team = ps->persistant[PERS_TEAM];

	s->bmodel = qfalse;
	s->capsule = ps->capsule;

	VectorCopy( ps->mins, s->mins );
	VectorCopy( ps->maxs, s->maxs );
	if ( snap ) {
		SnapVector( s->mins );
		SnapVector( s->maxs );
	}

#ifdef TA_WEAPSYS
	// cgame needs the weaponHands for all clients.
	s->weaponHands = ps->weaponHands;
#endif
#ifdef IOQ3ZTM_NO_COMPAT // DAMAGE_SKINS
	if (ps->stats[STAT_HEALTH] <= 0) {
		s->skinFraction = 1.0f;
	} else if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH]) {
		s->skinFraction = 0.0f;
	} else {
		s->skinFraction = 1.0f - ((float)ps->stats[STAT_HEALTH] / (float)ps->stats[STAT_MAX_HEALTH]);
	}
#endif
#ifdef TA_PLAYERSYS // LADDER
	VectorCopy( ps->origin2, s->origin2 );
#endif
}

/*
========================
BG_ComposeBits
========================
*/
void BG_ComposeBits( int *msg, int *bitsUsed, int value, int bits ) {
	*msg |= ( value & ( ( 1 << bits ) - 1 ) ) << *bitsUsed;
	*bitsUsed += bits;

	if ( *bitsUsed > 32 ) {
		Com_Error( ERR_DROP, "BG_ComposeBits exceeded 32 bits" );
	}
}

/*
========================
BG_DecomposeBits
========================
*/
void BG_DecomposeBits( int msg, int *bitsUsed, int *value, int bits ) {
	if ( value ) {
		*value = ( msg >> *bitsUsed ) & ( ( 1 << bits ) - 1 );
	}
	*bitsUsed += bits;

	if ( *bitsUsed > 32 ) {
		Com_Error( ERR_DROP, "BG_DecomposeBits exceeded 32 bits" );
	}
}

/*
========================
BG_ComposeUserCmdValue
========================
*/
int BG_ComposeUserCmdValue( int weapon, int holdable ) {
	int value = 0;
	int bitsUsed = 0;

#ifndef TA_WEAPSYS_EX
	BG_ComposeBits( &value, &bitsUsed, weapon, WEAPONNUM_BITS );
#endif

#ifdef TA_HOLDSYS/*2*/
	BG_ComposeBits( &value, &bitsUsed, holdable, HOLDABLENUM_BITS );
#endif

	return value;
}

/*
========================
BG_DecomposeUserCmdValue
========================
*/
void BG_DecomposeUserCmdValue( int value, int *weapon, int *holdable ) {
	int		bitsUsed = 0;

#ifndef TA_WEAPSYS_EX
	BG_DecomposeBits( value, &bitsUsed, weapon, WEAPONNUM_BITS );
#endif

#ifdef TA_HOLDSYS/*2*/
	BG_DecomposeBits( value, &bitsUsed, holdable, HOLDABLENUM_BITS );
#endif
}

int cmdcmp( const void *a, const void *b ) {
  return Q_stricmp( (const char *)a, ((dummyCmd_t *)b)->name );
}

#if defined CGAME || defined UI
/*
========================
BG_RegisterClientCvars

Init client-side cvars in cgame and ui.
========================
*/
void BG_RegisterClientCvars(int maxSplitview) {
	int i;
	const char *name;
	const int userInfo[MAX_SPLITVIEW] = { CVAR_USERINFO, CVAR_USERINFO2, CVAR_USERINFO3, CVAR_USERINFO4 };
	const char *modelNames[MAX_SPLITVIEW] = { DEFAULT_MODEL, DEFAULT_MODEL2, DEFAULT_MODEL3, DEFAULT_MODEL4 };
#ifdef IOQ3ZTM // BLANK_HEADMODEL
	const char *headModelNames[MAX_SPLITVIEW] = { "", "", "", "" };
#else
	const char *headModelNames[MAX_SPLITVIEW] = { DEFAULT_HEAD, DEFAULT_HEAD2, DEFAULT_HEAD3, DEFAULT_HEAD4 };
#endif
#ifndef IOQ3ZTM_NO_TEAM_MODEL
	const char *teamModelNames[MAX_SPLITVIEW] = { DEFAULT_TEAM_MODEL, DEFAULT_TEAM_MODEL2, DEFAULT_TEAM_MODEL3, DEFAULT_TEAM_MODEL4 };
	const char *teamHeadModelNames[MAX_SPLITVIEW] = { DEFAULT_TEAM_HEAD, DEFAULT_TEAM_HEAD2, DEFAULT_TEAM_HEAD3, DEFAULT_TEAM_HEAD4 };
#endif

	for (i = 0; i < maxSplitview; i++) {
		if (i == 0) {
			name = DEFAULT_CLIENT_NAME;
		} else {
			name = va("%s%d", DEFAULT_CLIENT_NAME, i + 1);
		}

		trap_Cvar_Register(NULL, Com_LocalClientCvarName(i, "name"), name, userInfo[i] | CVAR_ARCHIVE );

		trap_Cvar_Register(NULL, Com_LocalClientCvarName(i, "model"), modelNames[i], userInfo[i] | CVAR_ARCHIVE );
		trap_Cvar_Register(NULL, Com_LocalClientCvarName(i, "headmodel"), headModelNames[i], userInfo[i] | CVAR_ARCHIVE );

#ifndef IOQ3ZTM_NO_TEAM_MODEL
		trap_Cvar_Register(NULL, Com_LocalClientCvarName(i, "team_model"), teamModelNames[i], userInfo[i] | CVAR_ARCHIVE );
		trap_Cvar_Register(NULL, Com_LocalClientCvarName(i, "team_headmodel"), teamHeadModelNames[i], userInfo[i] | CVAR_ARCHIVE );
#endif

#ifdef TA_SP // SPMODEL
		trap_Cvar_Register(NULL, Com_LocalClientCvarName(i, "spmodel"), modelNames[i], userInfo[i] | CVAR_ROM );
		trap_Cvar_Register(NULL, Com_LocalClientCvarName(i, "spheadmodel"), headModelNames[i], userInfo[i] | CVAR_ROM );
#endif

		trap_Cvar_Register(NULL, Com_LocalClientCvarName(i, "color1"), va("%d", DEFAULT_CLIENT_COLOR1), userInfo[i] | CVAR_ARCHIVE );
		trap_Cvar_Register(NULL, Com_LocalClientCvarName(i, "color2"), va("%d", DEFAULT_CLIENT_COLOR2), userInfo[i] | CVAR_ARCHIVE );

		trap_Cvar_Register(NULL, Com_LocalClientCvarName(i, "handicap"), "100", userInfo[i] | CVAR_ARCHIVE );

		trap_Cvar_Register(NULL, Com_LocalClientCvarName(i, "teamtask"), "0", userInfo[i] );

		// set to in ui before starting server
		trap_Cvar_Register(NULL, Com_LocalClientCvarName(i, "teampref"), "", userInfo[i] );
		// clear team if was previously set (only want it used for one game)
		trap_Cvar_Set( Com_LocalClientCvarName(i, "teampref"), "" );

#ifndef TA_WEAPSYS_EX
		// init autoswitch so the ui will have it correctly even
		// if the cgame hasn't been started
		trap_Cvar_Register(NULL, Com_LocalClientCvarName(i, "cg_autoswitch"), "1", CVAR_ARCHIVE);
#endif
	}

	trap_Cvar_Register(NULL, "cg_predictItems", "1", CVAR_USERINFO_ALL | CVAR_ARCHIVE );

	// cgame might not be initialized before menu is used
	trap_Cvar_Register(NULL, "cg_viewsize", "100", CVAR_ARCHIVE );
	trap_Cvar_CheckRange("cg_viewsize", 30, 100, qtrue );
}
#endif
