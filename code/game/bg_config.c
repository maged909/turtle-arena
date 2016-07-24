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
// file parsing (for items, weapons, players, objects, unfinished NPCs) and
// a few general player and animation functions

#include "../qcommon/q_shared.h"
#include "bg_public.h"

#if defined TA_WEAPSYS || defined TA_PLAYERSYS || defined TA_ENTSYS
typedef struct
{
	int num;
	char *name;
} strAnimationDef_t;

#define ANIMDEF(x) {x, #x }
#endif

#if defined TA_WEAPSYS || defined TA_PLAYERSYS
strAnimationDef_t playerAnimationDefs[] = {
	// Quake3
	ANIMDEF(BOTH_DEATH1),
	ANIMDEF(BOTH_DEAD1),
	ANIMDEF(BOTH_DEATH2),
	ANIMDEF(BOTH_DEAD2),
	ANIMDEF(BOTH_DEATH3),
	ANIMDEF(BOTH_DEAD3),

	ANIMDEF(TORSO_GESTURE),

	ANIMDEF(TORSO_ATTACK),
	ANIMDEF(TORSO_ATTACK2),

	ANIMDEF(TORSO_DROP),
	ANIMDEF(TORSO_RAISE),

	ANIMDEF(TORSO_STAND),
	ANIMDEF(TORSO_STAND2),

	ANIMDEF(LEGS_WALKCR),
	ANIMDEF(LEGS_WALK),
	ANIMDEF(LEGS_RUN),
	ANIMDEF(LEGS_BACK),
	ANIMDEF(LEGS_SWIM),

	ANIMDEF(LEGS_JUMP),
	ANIMDEF(LEGS_LAND),

	ANIMDEF(LEGS_JUMPB),
	ANIMDEF(LEGS_LANDB),

	ANIMDEF(LEGS_IDLE),
	ANIMDEF(LEGS_IDLECR),

	ANIMDEF(LEGS_TURN),

	// Team Arena
	ANIMDEF(TORSO_GETFLAG),
	ANIMDEF(TORSO_GUARDBASE),
	ANIMDEF(TORSO_PATROL),
	ANIMDEF(TORSO_FOLLOWME),
	ANIMDEF(TORSO_AFFIRMATIVE),
	ANIMDEF(TORSO_NEGATIVE),

#ifdef TURTLEARENA // PLAYERS
	// Place default weapons somewhere on there person while there not used.
	ANIMDEF(TORSO_PUTDEFAULT_BOTH),
	ANIMDEF(TORSO_PUTDEFAULT_PRIMARY),
	ANIMDEF(TORSO_PUTDEFAULT_SECONDARY),
	ANIMDEF(TORSO_GETDEFAULT_BOTH),
	ANIMDEF(TORSO_GETDEFAULT_PRIMARY),
	ANIMDEF(TORSO_GETDEFAULT_SECONDARY),

	// Gun-type standing animations
    ANIMDEF(TORSO_STAND_GUN_PRIMARY),

	// Melee weapon standing animations
    ANIMDEF(TORSO_STAND_SWORD1_BOTH),
    ANIMDEF(TORSO_STAND_SWORD1_PRIMARY),

    ANIMDEF(TORSO_STAND_SWORD2),
    ANIMDEF(TORSO_STAND_DAISHO),
    ANIMDEF(TORSO_STAND_SAI2),
    ANIMDEF(TORSO_STAND_SAI1_PRIMARY),

    ANIMDEF(TORSO_STAND_BO),
    ANIMDEF(TORSO_STAND_BO_PRIMARY),

    ANIMDEF(TORSO_STAND_HAMMER),
    ANIMDEF(TORSO_STAND_HAMMER_PRIMARY),

    ANIMDEF(TORSO_STAND_NUNCHUCKS),
    ANIMDEF(TORSO_STAND_NUNCHUCKS1_PRIMARY),

	// Gun attacks
    ANIMDEF(TORSO_ATTACK_GUN_PRIMARY),

    // Melee Attacks
    ANIMDEF(TORSO_ATTACK_SWORD1_BOTH_A),
    ANIMDEF(TORSO_ATTACK_SWORD1_BOTH_B),
    ANIMDEF(TORSO_ATTACK_SWORD1_BOTH_C),

    ANIMDEF(TORSO_ATTACK_SWORD1_PRIMARY_A),
    ANIMDEF(TORSO_ATTACK_SWORD1_PRIMARY_B),
    ANIMDEF(TORSO_ATTACK_SWORD1_PRIMARY_C),

    ANIMDEF(TORSO_ATTACK_SWORD2_A),
    ANIMDEF(TORSO_ATTACK_SWORD2_B),
    ANIMDEF(TORSO_ATTACK_SWORD2_C),

    ANIMDEF(TORSO_ATTACK_DAISHO_A),
    ANIMDEF(TORSO_ATTACK_DAISHO_B),
    ANIMDEF(TORSO_ATTACK_DAISHO_C),

    ANIMDEF(TORSO_ATTACK_SAI2_A),
    ANIMDEF(TORSO_ATTACK_SAI2_B),
    ANIMDEF(TORSO_ATTACK_SAI2_C),

    ANIMDEF(TORSO_ATTACK_SAI1_PRIMARY_A),
    ANIMDEF(TORSO_ATTACK_SAI1_PRIMARY_C),
    ANIMDEF(TORSO_ATTACK_SAI1_PRIMARY_B),

    ANIMDEF(TORSO_ATTACK_BO_A),
    ANIMDEF(TORSO_ATTACK_BO_B),
    ANIMDEF(TORSO_ATTACK_BO_C),

    ANIMDEF(TORSO_ATTACK_BO_PRIMARY_A),
    ANIMDEF(TORSO_ATTACK_BO_PRIMARY_B),
    ANIMDEF(TORSO_ATTACK_BO_PRIMARY_C),

    ANIMDEF(TORSO_ATTACK_HAMMER_A),

    ANIMDEF(TORSO_ATTACK_HAMMER_PRIMARY_A),

    ANIMDEF(TORSO_ATTACK_NUNCHUCKS_A),
    ANIMDEF(TORSO_ATTACK_NUNCHUCKS_B),
    ANIMDEF(TORSO_ATTACK_NUNCHUCKS_C),
    ANIMDEF(TORSO_ATTACK_NUNCHUCKS1_PRIMARY_A),
    ANIMDEF(TORSO_ATTACK_NUNCHUCKS1_PRIMARY_B),
    ANIMDEF(TORSO_ATTACK_NUNCHUCKS1_PRIMARY_C),

	ANIMDEF(BOTH_LADDER_STAND),
	ANIMDEF(BOTH_LADDER_UP),
	ANIMDEF(BOTH_LADDER_DOWN),

	ANIMDEF(LEGS_JUMPB_LOCKON),
	ANIMDEF(LEGS_LANDB_LOCKON),

	ANIMDEF(BOTH_WAITING),
#endif

	// Quake3, not loaded from file
	ANIMDEF(LEGS_BACKCR),
	ANIMDEF(LEGS_BACKWALK),
#ifndef IOQ3ZTM // FLAG_ANIMATIONS
	ANIMDEF(FLAG_RUN),
	ANIMDEF(FLAG_STAND),
	ANIMDEF(FLAG_STAND2RUN),
#endif

	// Fake animations (Different names for real animations)
	{ TORSO_STAND, "TORSO_STAND_GUN" },
	{ TORSO_STAND2, "TORSO_STAND_GAUNTLET" },
	{ TORSO_ATTACK, "TORSO_ATTACK_GUN" },
	{ TORSO_ATTACK2, "TORSO_ATTACK_GAUNTLET" },

#ifdef TURTLEARENA // PLAYERS
	{ BOTH_LADDER_UP, "BOTH_LADDER" },
#endif

#if 0 // ZTM: NOTE: This is unneeded as we load undefined animations
	// Fake animations (BOTH_* version of TORSO_* animations)
	{ TORSO_ATTACK, "BOTH_ATTACK" },
	{ TORSO_ATTACK2, "BOTH_ATTACK2" },
	{ TORSO_STAND, "BOTH_STAND" },
	{ TORSO_STAND2, "BOTH_STAND2" },
	{ TORSO_STAND, "BOTH_STAND_GUN" },
	{ TORSO_STAND2, "BOTH_STAND_GAUNTLET" },
	{ TORSO_ATTACK, "BOTH_ATTACK_GUN" },
	{ TORSO_ATTACK2, "BOTH_ATTACK_GAUNTLET" },
#endif

	// End of List
	{ 0, NULL }
};
#endif

bg_commonInfo_t *bg_common = NULL;

int BG_ItemNumForItem( gitem_t *item )
{
	if (!item)
	{
		//Com_Printf("DEBUG: Returning type:NULL itemNum:0\n");
		return 0;
	}

	// If address is in bg_iteminfo
	if ((item - bg_iteminfo) < bg_numitems
		&& (item - bg_iteminfo) >= 0)
	{
		//Com_Printf("DEBUG: Returning type:gitem itemNum:%d\n", (item - bg_iteminfo));
		return (int)(item - bg_iteminfo);
	}

	// Failed
	//Com_Printf("DEBUG: Returning type:unknown itemNum:0\n");
	return 0;
}

gitem_t *BG_ItemForItemNum( int itemnum )
{
	if (itemnum >= 0 && itemnum < bg_numitems)
		return &bg_iteminfo[itemnum];

	return &bg_iteminfo[0]; // Can't return NULL.
}

int BG_ItemIndexForName(const char *classname)
{
	int i;
	for (i = 0; i < MAX_ITEMS; i++)
	{
		if ( !Q_stricmp( bg_iteminfo[i].classname, classname))
		{
			return i;
		}
	}

	return 0;
}

int BG_NumItems(void)
{
	if (bg_itemsys_init)
	{
		return bg_numitems;
	}
	else
	{
		// BG_GetFreeItemNum
		int i;
		for (i = 1; i < MAX_ITEMS; i++)
		{
			if ( bg_iteminfo[i].classname[0] != '\0' )
				continue;
			break;
		}
		bg_numitems = i+1;
		return i;
	}
}

int BG_NumHoldableItems(void)
{
	return bg_numholdables;
}

#ifdef TURTLEARENA // HOLD_SHURIKEN
int BG_ProjectileIndexForHoldable(int holdable)
{
	int projnum;

	switch (holdable)
	{
		case HI_SHURIKEN:
			projnum = BG_ProjectileIndexForName("p_shuriken");
			break;
		case HI_ELECTRICSHURIKEN:
			projnum = BG_ProjectileIndexForName("p_electricshuriken");
			break;
		case HI_FIRESHURIKEN:
			projnum = BG_ProjectileIndexForName("p_fireshuriken");
			break;
		case HI_LASERSHURIKEN:
			projnum = BG_ProjectileIndexForName("p_lasershuriken");
			break;
		default:
			projnum = BG_ProjectileIndexForName(va("p_holdable%d", holdable));
			break;
	}

	return projnum;
}
#endif

#ifdef TA_WEAPSYS
int BG_ProjectileIndexForName(const char *name)
{
	int i;
	for (i = 0; i < MAX_BG_PROJ; i++)
	{
		if ( !Q_stricmp( bg_projectileinfo[i].name, name))
		{
			return i;
		}
	}

	return 0;
}

int BG_WeaponIndexForName(const char *name)
{
	int i;
	for (i = 0; i < MAX_BG_WEAPONS; i++)
	{
		if ( !Q_stricmp( bg_weaponinfo[i].name, name))
		{
			return i;
		}
	}

	return 0;
}

int BG_WeaponGroupIndexForName(const char *name)
{
	int i;
	for (i = 0; i < MAX_BG_WEAPON_GROUPS; i++)
	{
		if ( !Q_stricmp( bg_weapongroupinfo[i].name, name))
		{
			return i;
		}
	}

	return 0;
}

int BG_NumProjectiles(void)
{
	if (bg_itemsys_init)
	{
		return bg_numprojectiles;
	}
	else
	{
		// BG_GetFreeProjectileNum
		int i;
		for (i = 0; i < MAX_BG_PROJ; i++)
		{
			if ( bg_projectileinfo[i].name[0] != '\0' )
				continue;
			break;
		}
		// projectile i is unused.
		bg_numprojectiles = i+1; // i is going to be used so count it anyway.
		return i;
	}
}

int BG_NumWeapons(void)
{
	if (bg_itemsys_init)
	{
		return bg_numweapons;
	}
	else
	{
		// BG_GetFreeWeaponNum
		int i;
		for (i = 0; i < MAX_BG_WEAPONS; i++)
		{
			if ( bg_weaponinfo[i].name[0] != '\0' )
				continue;
			break;
		}
		bg_numweapons = i+1;
		return i;
	}
}

int BG_NumWeaponGroups(void)
{
	if (bg_itemsys_init)
	{
		return bg_numweapongroups;
	}
	else
	{
		// BG_GetFreeWeaponGroupNum
		int i;
		for (i = 0; i < MAX_BG_WEAPON_GROUPS; i++)
		{
			if ( bg_weapongroupinfo[i].name[0] != '\0' )
				continue;
			break;
		}
		bg_numweapongroups = i+1;
		return i;
	}
}
#endif

#define PARSE_STRING(_token, _name, _out) \
		if ( !Q_stricmp( _token, _name ) ) { \
			_token = COM_Parse( p ); \
			if ( *_token ) { \
				Com_sprintf(_out, sizeof (_out), "%s", _token); \
			} else { \
				_out[0] = '\0'; \
			} \
			continue; \
		}

#define PARSE_INTEGER(_token, _name, _out) \
		if ( !Q_stricmp( _token, _name ) ) { \
			_token = COM_Parse( p ); \
			if ( *_token ) { \
				_out = atoi(_token); \
			} else { \
				Com_Printf("Missing token for %s\n", _name); \
			} \
			continue; \
		}

#define PARSE_FLOAT(_token, _name, _out) \
		if ( !Q_stricmp( _token, _name ) ) { \
			_token = COM_Parse( p ); \
			if ( *_token ) { \
				_out = atof(_token); \
			} else { \
				Com_Printf("Missing token for %s\n", _name); \
			} \
			continue; \
		}

// Some q3 code uses ( 1 1 1 ), so it is supported here too.
#define PARSE_FLOAT3(_token, _name, _out) \
		if ( !Q_stricmp( _token, _name ) ) { \
			for (i = 0; i < 3; i++) { \
				_token = COM_Parse( p ); \
				if ( _token[0] == '(' || _token[0] == ')') { \
					i--; \
					continue; \
				} \
				if ( *_token ) { \
					_out[i] = atof(_token); \
				} else { \
					Com_Printf("Missing token for %s\n", _name); \
					break; \
				} \
			} \
			continue; \
		}

typedef struct
{
	vec3_t value;
	const char *name;
} strVec3_t;

#define PARSE_FLOAT3_LIST(_token, _name, _out, _list) \
		if ( !Q_stricmp( _token, _name ) ) { \
			for (i = 0; i < 3; i++) { \
				_token = COM_Parse( p ); \
				if ( _token[0] == '(' || _token[0] == ')') { \
					i--; \
					continue; \
				} \
				if ( *_token ) { \
					int j; \
					for (j = 0; _list[j].name != NULL; j++) { \
						if ( !Q_stricmp( _token, _list[j].name )) { \
							VectorCopy(_list[j].value, _out); \
							break; \
						} \
					} \
					if (_list[j].name != NULL) \
						break; \
					_out[i] = atof(_token); \
				} else { \
					Com_Printf("Missing token for %s\n", _name); \
					break; \
				} \
			} \
			continue; \
		}

#define PARSE_LIST(_token, _name, _out, _list) \
		if ( !Q_stricmp( _token, _name ) ) { \
			_token = COM_Parse( p ); \
			if ( *_token ) { \
				for (i = 0; _list[i] != NULL; i++) { \
					if ( !Q_stricmp( _token, _list[i] ) ) { \
						_out = i; \
						break; \
					} \
				} \
				if (_list[i] == NULL) { \
					Com_Printf("Unknown token %s: valid options for \'%s\' are", _token, _name); \
					for (i = 0; _list[i] != NULL; i++) \
					{ \
						if (i == 0) Com_Printf(" %s", _list[i]); \
						else Com_Printf(", %s", _list[i]); \
					} \
					Com_Printf("\n"); \
					return qfalse; \
				} \
			} else { \
				Com_Printf("Missing token for %s\n", _name); \
			} \
			continue; \
		}

// Manual limit for when _list doesn't end with NULL.
#define PARSE_LIST2(_token, _name, _out, _list, _limit) \
		if ( !Q_stricmp( _token, _name ) ) { \
			_token = COM_Parse( p ); \
			if ( *_token ) { \
				for (i = 0; i < _limit; i++) { \
					if ( !Q_stricmp( _token, _list[i] ) ) { \
						_out = i; \
						break; \
					} \
				} \
				if (i == _limit) { \
					Com_Printf("Unknown token %s: valid options for \'%s\' are", _token, _name); \
					for (i = 0; _list[i] != NULL; i++) \
					{ \
						if (i == 0) Com_Printf(" %s", _list[i]); \
						else Com_Printf(", %s", _list[i]); \
					} \
					Com_Printf("\n"); \
					return qfalse; \
				} \
			} else { \
				Com_Printf("Missing token for %s\n", _name); \
			} \
			continue; \
		}

#define PARSE_BOOL(_token, _name, _out) \
		if ( !Q_stricmp( _token, _name ) ) { \
			_token = COM_Parse( p ); \
			if ( *_token ) { \
				if ( !Q_stricmp( _token, "true" ) || !Q_stricmp( _token, "qtrue" ) || !Q_stricmp( _token, "1" )) \
					_out = qtrue; \
				else if ( !Q_stricmp( _token, "false" ) || !Q_stricmp( _token, "qfalse" ) || !Q_stricmp( _token, "0" )) \
					_out = qfalse; \
				else { \
					Com_Printf("Unknown token %s: valid options for \'%s\' are true and false\n", _token, _name); \
					return qfalse; \
				} \
			} else { \
				Com_Printf("Missing token for %s\n", _name); \
			} \
			continue; \
		}

#define PARSE_BIT(_token, _name, _out, _bit) \
		if ( !Q_stricmp( _token, _name ) ) { \
			_token = COM_Parse( p ); \
			if ( *_token ) { \
				if ( !Q_stricmp( _token, "true" ) || !Q_stricmp( _token, "qtrue" ) || !Q_stricmp( _token, "1" )) \
					_out |= _bit; \
				else if ( !Q_stricmp( _token, "false" ) || !Q_stricmp( _token, "qfalse" ) || !Q_stricmp( _token, "0" )) \
					_out &= ~_bit; \
				else { \
					Com_Printf("Unknown token %s: valid options for \'%s\' are true and false\n", _token, _name); \
					return qfalse; \
				} \
			} else { \
				Com_Printf("Missing token for %s\n", _name); \
			} \
			continue; \
		}

const char *it_names[] =
{
	"IT_BAD",
	"IT_WEAPON",
	"IT_AMMO",
#ifdef TURTLEARENA // NIGHTS_ITEMS
	"IT_SCORE",
#endif
#ifndef TURTLEARENA // NOARMOR
	"IT_ARMOR",
#endif
	"IT_HEALTH",
	"IT_POWERUP",
	"IT_HOLDABLE",
	"IT_PERSISTANT_POWERUP",
	"IT_TEAM",
	NULL
};

#ifndef TA_WEAPSYS
const char *weapon_names[WP_NUM_WEAPONS+1] =
{
	"WP_NONE",

	"WP_GAUNTLET",
	"WP_MACHINEGUN",
	"WP_SHOTGUN",
	"WP_GRENADE_LAUNCHER",
	"WP_ROCKET_LAUNCHER",
	"WP_LIGHTNING",
	"WP_RAILGUN",
	"WP_PLASMAGUN",
	"WP_BFG",
	"WP_GRAPPLING_HOOK",
#ifdef MISSIONPACK
	"WP_NAILGUN",
	"WP_PROX_LAUNCHER",
	"WP_CHAINGUN",
#endif

	NULL
};
#endif

// ZTM: Hmm... I believe this shows my insanity quite well.
const char *holdable_names[HI_NUM_HOLDABLE+1] =
{
	"HI_NONE",
#ifndef TURTLEARENA // no q3 teleprter
	"HI_TELEPORTER",
#elif !defined TA_HOLDSYS
	"HI_TELEPORTER_REMOVED", // do not use
#endif
	"HI_MEDKIT",
#ifndef TURTLEARENA // NO_KAMIKAZE_ITEM
	"HI_KAMIKAZE",
#elif !defined TA_HOLDSYS
	"HI_KAMIKAZE_REMOVED", // do not use
#endif
#ifndef TA_HOLDSYS
	"HI_PORTAL",
#endif
#ifndef TURTLEARENA // POWERS
	"HI_INVULNERABILITY",
#elif !defined TA_HOLDSYS
	"HI_INVULNERABILITY_REMOVED", // do not use
#endif
#ifdef TURTLEARENA // HOLD_SHURIKEN
	// Shurikens
	"HI_SHURIKEN",
	"HI_ELECTRICSHURIKEN",
	"HI_FIRESHURIKEN",
	"HI_LASERSHURIKEN",
#endif
#ifdef TA_HOLDSYS // ZTM: Moved "out of the way"
	"HI_PORTAL",
#endif

	NULL
};

const char *powerup_names[PW_NUM_POWERUPS+1] =
{
	"PW_NONE",
#ifdef TURTLEARENA // POWERS
	"PW_QUAD",
	"PW_BATTLESUIT",
	"PW_HASTE",
	"PW_INVIS",

	"PW_FLIGHT",

	"PW_INVUL",
	"PW_FLASHING",

	"PW_REDFLAG",
	"PW_BLUEFLAG",
	"PW_NEUTRALFLAG",

	"PW_SCOUT",
	"PW_GUARD",
	"PW_DOUBLER",
	"PW_AMMOREGEN",
#else
	"PW_QUAD",
	"PW_BATTLESUIT",
	"PW_HASTE",
	"PW_INVIS",
	"PW_REGEN",
	"PW_FLIGHT",

	"PW_REDFLAG",
	"PW_BLUEFLAG",
	"PW_NEUTRALFLAG",

	"PW_SCOUT",
	"PW_GUARD",
	"PW_DOUBLER",
	"PW_AMMOREGEN",
	"PW_INVULNERABILITY",
#endif
	NULL
};

static qboolean ItemInfo_Parse(char **p) {
	char *token;
	gitem_t item;
	int i;
	int num;

	memset(&item, 0, sizeof(item));

	token = COM_ParseExt(p, qtrue);
	if ( !*token ) {
		Com_Printf("Warning: item missing classname\n");
		return qfalse;
	}
	Com_sprintf(item.classname, sizeof (item.classname), "%s", token);

	//Com_Printf("Loading item [%s] ...\n", item.classname);

	token = COM_ParseExt(p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {
		token = COM_ParseExt(p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			num = BG_ItemIndexForName(item.classname);
			if (!num)
			{
				// Use free slot
				num = BG_NumItems();
			}

			Com_Memcpy(&bg_iteminfo[num], &item, sizeof (item));
			//Com_Printf("Loaded item [%s]\n", item.classname);
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		if ( !Q_stricmp( token, "clone" ) ) {
			char classname[MAX_QPATH]; // Save name
			int num;
			token = COM_Parse( p );
			if ( !*token ) {
				break;
			}
			num = BG_ItemIndexForName(token);
			if (num) {
				Q_strncpyz(classname, item.classname, sizeof (classname)); // backup name
				memcpy(&item, &bg_iteminfo[num], sizeof (item));
				Q_strncpyz(item.classname, classname, sizeof (classname)); // use backed up name
			}
			continue;
		}
		else PARSE_STRING(token, "model", item.world_model[0])
		else PARSE_STRING(token, "pickupModel", item.world_model[0]) // Be compatible with weaponGroup
		else PARSE_STRING(token, "model0", item.world_model[0])
		else PARSE_STRING(token, "model1", item.world_model[1])
		else PARSE_STRING(token, "model2", item.world_model[2])
		else PARSE_STRING(token, "model3", item.world_model[3])
		else PARSE_STRING(token, "pickupSound", item.pickup_sound)
		else PARSE_STRING(token, "iconName", item.icon)
		else PARSE_STRING(token, "pickupName", item.pickup_name)
		else PARSE_INTEGER(token, "quantity", item.quantity)
		else PARSE_INTEGER(token, "pickupAmmo", item.quantity) // Be compatible with weaponGroup
		else PARSE_LIST(token, "type", item.giType, it_names)
		else if ( !Q_stricmp( token, "tag" ) ) {
			token = COM_Parse( p );
			if ( !*token ) {
				break;
			}

			switch (item.giType)
			{
				case IT_WEAPON:
				case IT_AMMO:
#ifdef TA_WEAPSYS
					num = BG_WeaponGroupIndexForName(token);
					if (num) {
						item.giTag = num;
						continue;
					}
#else
					num = -1;
					for (i = 0; weapon_names[i] != NULL; i++)
					{
						if ( !Q_stricmp( token, weapon_names[i] ) ) {
							num = i;
							break;
						}
					}
					if (num != -1) {
						item.giTag = num;
						continue;
					}
#endif
					break;

				case IT_HOLDABLE:
					num = -1;
					for (i = 0; holdable_names[i] != NULL; i++)
					{
						if ( !Q_stricmp( token, holdable_names[i] ) ) {
							num = i;
							break;
						}
					}
					if (num != -1) {
						item.giTag = num;
						continue;
					}
					break;

				case IT_POWERUP:
				case IT_PERSISTANT_POWERUP:
				case IT_TEAM:
					num = -1;
					for (i = 0; powerup_names[i] != NULL; i++)
					{
						if ( !Q_stricmp( token, powerup_names[i] ) ) {
							num = i;
							break;
						}
					}
					if (num != -1) {
						item.giTag = num;
						continue;
					}
					break;

				default:
					// Just use integer
					break;
			}

			item.giTag = atoi(token);
			continue;
		}
		else PARSE_STRING(token, "skin", item.skin)
		//else PARSE_STRING(token, "precaches", item.precaches)
		else PARSE_STRING(token, "sounds", item.sounds)

		Com_Printf( "unknown token '%s' in item %s\n", token, item.classname );
	}
	return qfalse;
}

#ifdef TA_WEAPSYS
//projectile trail types
const char *pt_names[] =
{
	"PT_NONE",
	"PT_PLASMA",
	"PT_ROCKET",
	"PT_GRENADE",
	"PT_GRAPPLE",
	"PT_NAIL",
	"PT_LIGHTNING",
	"PT_RAIL",
	"PT_BULLET",
	"PT_SPARKS",
	NULL
};

//projectile death types
const char *pd_names[] =
{
	"PD_NONE",
	"PD_PLASMA",
	"PD_ROCKET",
	"PD_GRENADE",
	"PD_BULLET",
	"PD_RAIL",
	"PD_BFG",
	"PD_LIGHTNING",
	"PD_ROCKET_SMALL",
	"PD_BULLET_COLORIZE",
	"PD_NONE_EXP_PLAYER",
	NULL
};

//projectile explosion types
const char *pe_names[] =
{
	"PE_NORMAL",
	"PE_NONE",
	"PE_PROX",
	NULL
};

//projectile spin types
const char *ps_names[] =
{
	"PS_ROLL",
	"PS_NONE",
	"PS_PITCH",
	"PD_YAW",
	NULL
};

//projectile bounce types
const char *pb_names[] =
{
	"PB_NONE",
	"PB_HALF",
	"PB_FULL",
	NULL
};

//projectile stickOnImpact
const char *psoi_names[] =
{
	"PSOI_NONE",
	"PSOI_KEEP_ANGLES",
	"PSOI_ANGLE_180",
	"PSOI_ANGLE_90",
	"PSOI_ANGLE_0",
	"PSOI_ANGLE_270",
	NULL
};

static qboolean Projectile_Parse(char **p) {
	char *token;
	bg_projectileinfo_t projectile;
	int i;

	memset(&projectile, 0, sizeof(projectile));
	projectile.numProjectiles = 1;
	projectile.maxHits = 1;
	projectile.damageAttacker = qtrue;

	token = COM_ParseExt(p, qtrue);
	if ( !*token ) {
		Com_Printf("Warning: projectile missing name\n");
		return qfalse;
	}
	Com_sprintf(projectile.name, sizeof (projectile.name), "%s", token);

	//Com_Printf("Loading projectile [%s] ...\n", projectile.name);

	token = COM_ParseExt(p, qtrue);

	if (token[0] != '{') {
	return qfalse;
	}

	while ( 1 ) {
		token = COM_ParseExt(p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			int num = BG_ProjectileIndexForName(projectile.name);
			if (!num)
			{
				// Use free slot
				num = BG_NumProjectiles();
			}

			Com_Memcpy(&bg_projectileinfo[num], &projectile, sizeof (projectile));
			//Com_Printf("Loaded projectile [%s]\n", projectile.name);
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		if ( !Q_stricmp( token, "clone" ) ) {
			char name[MAX_QPATH]; // Save name
			int num;
			token = COM_Parse( p );
			if ( !*token ) {
				break;
			}
			num = BG_ProjectileIndexForName(token);
			if (num) {
				Q_strncpyz(name, projectile.name, MAX_QPATH); // backup name
				memcpy(&projectile, &bg_projectileinfo[num], sizeof (projectile));
				Q_strncpyz(projectile.name, name, MAX_QPATH); // use backed up name
			}
			continue;
		}
		else PARSE_STRING(token, "model", projectile.model)
		else PARSE_STRING(token, "modelBlue", projectile.modelBlue)
		else PARSE_STRING(token, "modelRed", projectile.modelRed)
		else PARSE_INTEGER(token, "damageAttacker", projectile.damageAttacker)
		else PARSE_INTEGER(token, "damage", projectile.damage)
		else PARSE_INTEGER(token, "splashDamage", projectile.splashDamage)
		else PARSE_FLOAT(token, "splashRadius", projectile.splashRadius)
		else PARSE_INTEGER(token, "speed", projectile.speed)
		else PARSE_INTEGER(token, "timetolive", projectile.timetolive)
		else PARSE_BOOL(token, "shootable", projectile.shootable)
		else PARSE_LIST(token, "trailType", projectile.trailType, pt_names)
		else PARSE_STRING(token, "trailShader0", projectile.trailShaderName[0])
		else PARSE_STRING(token, "trailShader1", projectile.trailShaderName[1])
		else PARSE_INTEGER(token, "trailRadius", projectile.trailRadius)
		else PARSE_INTEGER(token, "trailTime", projectile.trailTime)
		else PARSE_LIST(token, "deathType", projectile.deathType, pd_names)
		else PARSE_LIST(token, "explosionType", projectile.explosionType, pe_names)
		else PARSE_INTEGER(token, "missileDlight", projectile.missileDlight)
		else PARSE_STRING(token, "missileSound", projectile.missileSoundName)
		else PARSE_FLOAT3(token, "missileDlightColor", projectile.missileDlightColor)
		else PARSE_STRING(token, "sprite", projectile.sprite)
		else PARSE_INTEGER(token, "spriteRadius", projectile.spriteRadius)
		else PARSE_LIST(token, "spinType", projectile.spinType, ps_names)
		else PARSE_LIST(token, "bounceType", projectile.bounceType, pb_names)
		else PARSE_INTEGER(token, "maxBounces", projectile.maxBounces)
		else PARSE_BIT(token, "useGravity", projectile.flags, PF_USE_GRAVITY)
		else PARSE_LIST(token, "stickOnImpact", projectile.stickOnImpact, psoi_names)
		else PARSE_BOOL(token, "fallToGround", projectile.fallToGround)
		else PARSE_INTEGER(token, "spdRndAdd", projectile.spdRndAdd)
		else PARSE_INTEGER(token, "spread", projectile.spread)
		else PARSE_INTEGER(token, "numProjectiles", projectile.numProjectiles)
		else PARSE_INTEGER(token, "homing", projectile.homing)
		else PARSE_BOOL(token, "grappling", projectile.grappling)
		else PARSE_BOOL(token, "instantDamage", projectile.instantDamage)
		else PARSE_INTEGER(token, "maxHits", projectile.maxHits)
		else PARSE_BIT(token, "hitMarkFadeAlpha", projectile.flags, PF_HITMARK_FADE_ALPHA)
		else PARSE_BIT(token, "hitMarkColorize", projectile.flags, PF_HITMARK_COLORIZE)
		else PARSE_STRING(token, "hitMarkName", projectile.hitMarkName)
		else PARSE_INTEGER(token, "hitMarkRadius", projectile.hitMarkRadius)
		else PARSE_STRING(token, "hitSound0", projectile.hitSoundName[0])
		else PARSE_STRING(token, "hitSound1", projectile.hitSoundName[1])
		else PARSE_STRING(token, "hitSound2", projectile.hitSoundName[2])
		else PARSE_STRING(token, "hitPlayerSound", projectile.hitPlayerSoundName)
		else PARSE_STRING(token, "hitMetalSound", projectile.hitMetalSoundName)
		else PARSE_BIT(token, "impactMarkFadeAlpha", projectile.flags, PF_IMPACTMARK_FADE_ALPHA)
		else PARSE_BIT(token, "impactMarkColorize", projectile.flags, PF_IMPACTMARK_COLORIZE)
		else PARSE_STRING(token, "impactMarkName", projectile.impactMarkName)
		else PARSE_INTEGER(token, "impactMarkRadius", projectile.impactMarkRadius)
		else PARSE_STRING(token, "impactSound0", projectile.impactSoundName[0])
		else PARSE_STRING(token, "impactSound1", projectile.impactSoundName[1])
		else PARSE_STRING(token, "impactSound2", projectile.impactSoundName[2])
		else PARSE_STRING(token, "impactPlayerSound", projectile.impactPlayerSoundName)
		else PARSE_STRING(token, "impactMetalSound", projectile.impactMetalSoundName)
		else PARSE_STRING(token, "triggerSound", projectile.triggerSoundName)
		else PARSE_STRING(token, "tickSound", projectile.tickSoundName)

		Com_Printf( "unknown token '%s' in projectile %s\n", token, projectile.name );
	}
	return qfalse;
}

const char *trailtype_names[] = {
	"none", // TRAIL_NONE
	"default", // TRAIL_DEFAULT
	NULL
};

static qboolean WeaponBlade_Parse(char **p, bg_weaponinfo_t *weapon) {
	char *token;
	int bladeNum = 0;
	bg_bladeinfo_t *blade = NULL;
	qboolean validBlade = qtrue;
	int i;

	token = COM_ParseExt(p, qtrue);

	if (token[0] >= '0' && token[0] <= '9')
	{
		bladeNum = atoi(token);
		token = COM_ParseExt(p, qtrue);
	}
	else
	{
		for (i=0; i < MAX_WEAPON_BLADES; i++)
		{
			if (weapon->blades[i].damage == 0)
			{
				// Make sure it is a clean blade.
				Com_Memset(&weapon->blades[i], 0, sizeof (bg_bladeinfo_t));
				break;
			}
		}
		bladeNum = i;
	}

	if (token[0] != '{') {
		return qfalse;
	}

	validBlade = (bladeNum < MAX_WEAPON_BLADES);
	if (!validBlade)
	{
		Com_Printf("Weapon [%s]: Invalid blade number %d\n", weapon->name, bladeNum);
	}
	else
	{
		blade = &weapon->blades[bladeNum];
	}

	while ( 1 ) {
		token = COM_ParseExt(p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		// Just skipping this blade.
		if (!validBlade)
		{
			continue;
		}

		PARSE_FLOAT3(token, "origin", blade->origin)
		else PARSE_FLOAT3(token, "tip", blade->tip)
		else PARSE_INTEGER(token, "damage", blade->damage)
		else PARSE_LIST(token, "trailStyle", blade->trailStyle, trailtype_names)

		//Com_Printf( "unknown token '%s' [in blade] in %s\n", token, filename );
		Com_Printf( "unknown token '%s' [in blade]\n", token );
	}
	return qfalse;
}

const char *weapontypeNames[WT_MAX+1] = {
	"WT_NONE",
	"WT_GAUNTLET",
	"WT_GUN",
	"WT_MELEE",
	NULL
};

const char *barrelSpinNames[BS_MAX+1] = {
	"BS_PITCH",
	"BS_YAW",
	"BS_ROLL",
	"BS_NONE",
	NULL
};

const strVec3_t flashColor_list[] = {
	{{0,0,0}, "none"},
	{{(float)'c',1.0f,0}, "color1"},
	{{(float)'c',2.0f,0}, "color2"},

	{{0,0,0}, NULL},
};

static qboolean Weapon_Parse(char **p) {
	char *token;
	bg_weaponinfo_t weapon;
	int i;

	memset(&weapon, 0, sizeof(weapon));

	weapon.attackDelay = 400;
	weapon.flashColor[0] = 1.0f;
	weapon.flashColor[1] = 1.0f;
	weapon.flashColor[2] = 1.0f;
	weapon.impactMarkRadius = 16;
	weapon.barrelSpin = BS_ROLL;
	// fire_projectile and G_MeleeDamageSingle will use the
	//     correct default mod (MOD_PORJECTILE or MOD_WEAPON_*)
	weapon.mod = MOD_UNKNOWN;
	weapon.splashMod = MOD_UNKNOWN;

	// Set projectile pointer
	weapon.projnum = 0;
	weapon.proj = &bg_projectileinfo[0];

	token = COM_ParseExt(p, qtrue);
	if ( !*token ) {
		Com_Printf("Warning: weapon missing name\n");
		return qfalse;
	}
	Com_sprintf(weapon.name, sizeof (weapon.name), "%s", token);

	//Com_Printf("Loading weapon [%s] ...\n", weapon.name);

	token = COM_ParseExt(p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {
		token = COM_ParseExt(p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			int num = BG_WeaponIndexForName(weapon.name);
			if (!num)
			{
				// Use free slot
				num = BG_NumWeapons();
			}

			Com_Memcpy(&bg_weaponinfo[num], &weapon, sizeof (weapon));
			//Com_Printf("Loaded weapon [%s]\n", weapon.name);
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		if (Q_stricmp(token, "blade") == 0) {
			if (WeaponBlade_Parse(p, &weapon))
				continue;
			else
				break;
		} else if ( !Q_stricmp( token, "projectile" ) ) {
			token = COM_Parse( p );
			if ( !*token ) {
				weapon.projnum = 0;
				weapon.proj = &bg_projectileinfo[weapon.projnum];
				continue;
			}
			weapon.projnum = BG_ProjectileIndexForName(token);
			weapon.proj = &bg_projectileinfo[weapon.projnum];
			continue;
		} else if ( !Q_stricmp( token, "clone" ) ) {
			char name[MAX_QPATH]; // Save name
			int num;
			token = COM_Parse( p );
			if ( !*token ) {
				break;
			}
			num = BG_WeaponIndexForName(token);
			if (num) {
				Q_strncpyz(name, weapon.name, MAX_QPATH); // backup name
				memcpy(&weapon, &bg_weaponinfo[num], sizeof (weapon));
				Q_strncpyz(weapon.name, name, MAX_QPATH); // use backed up name
			}
			continue;
		}
		else PARSE_STRING(token, "model", weapon.model)
		else PARSE_LIST(token, "weapontype", weapon.weapontype, weapontypeNames)
		else PARSE_INTEGER(token, "attackDelay", weapon.attackDelay)
		else PARSE_STRING(token, "impactMarkName", weapon.impactMarkName)
		else PARSE_INTEGER(token, "impactMarkRadius", weapon.impactMarkRadius)
		else PARSE_BIT(token, "impactMarkFadeAlpha", weapon.flags, WIF_IMPACTMARK_FADE_ALPHA)
		else PARSE_BIT(token, "impactMarkColorize", weapon.flags, WIF_IMPACTMARK_COLORIZE)
		else PARSE_BIT(token, "alwaysDamage", weapon.flags, WIF_ALWAYS_DAMAGE)
		else PARSE_BIT(token, "cuts", weapon.flags, WIF_CUTS)
		else PARSE_BIT(token, "continuousFlash", weapon.flags, WIF_CONTINUOUS_FLASH)
		else PARSE_BIT(token, "ejectBrass", weapon.flags, WIF_EJECT_BRASS)
		else PARSE_BIT(token, "ejectBrass2", weapon.flags, WIF_EJECT_BRASS2)
		else PARSE_BIT(token, "ejectSmoke", weapon.flags, WIF_EJECT_SMOKE)
		else PARSE_BIT(token, "ejectSmoke2", weapon.flags, WIF_EJECT_SMOKE2)
		else PARSE_LIST2(token, "mod", weapon.mod, modNames, MOD_MAX)
		else PARSE_LIST2(token, "splashMod", weapon.splashMod, modNames, MOD_MAX)
		else PARSE_FLOAT3_LIST(token, "flashColor", weapon.flashColor, flashColor_list)
		else PARSE_STRING(token, "flashSound0", weapon.flashSoundName[0])
		else PARSE_STRING(token, "flashSound1", weapon.flashSoundName[1])
		else PARSE_STRING(token, "flashSound2", weapon.flashSoundName[2])
		else PARSE_STRING(token, "flashSound3", weapon.flashSoundName[3])
		else PARSE_LIST(token, "barrelSpin", weapon.barrelSpin, barrelSpinNames)
		else PARSE_BIT(token, "barrelIdleUseGravity", weapon.flags, WIF_BARREL_IDLE_USE_GRAVITY)
		else PARSE_BIT(token, "initialEffectOnly", weapon.flags, WIF_INITIAL_EFFECT_ONLY)
		else PARSE_STRING(token, "impactSound0", weapon.impactSoundName[0])
		else PARSE_STRING(token, "impactSound1", weapon.impactSoundName[1])
		else PARSE_STRING(token, "impactSound2", weapon.impactSoundName[2])
		else PARSE_STRING(token, "impactPlayerSound", weapon.impactPlayerSoundName)
		else PARSE_STRING(token, "impactMetalSound", weapon.impactMetalSoundName)

		Com_Printf( "unknown token '%s' in weapon %s\n", token, weapon.name );
	}
	return qfalse;
}

static qboolean WeaponGroupAnims_Parse(char **p, bg_weapongroup_anims_t *anims) {
	char *token;
	int i;

	token = COM_ParseExt(p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {
		token = COM_ParseExt(p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			// Count the number of valid attack animations
			for (anims->numAttackAnims = 0; anims->numAttackAnims < MAX_WG_ATK_ANIMS; anims->numAttackAnims++) {
				if (anims->attackAnim[anims->numAttackAnims] == 0) {
					break;
				}
			}
			if (anims->numAttackAnims < 1) {
				anims->attackAnim[0] = TORSO_ATTACK;
				anims->numAttackAnims = 1;
			}

			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		if (Q_stricmp(token, "standAnim") == 0) {
			token = COM_Parse( p );
			if ( !*token ) {
				break;
			}
			for (i = 0; playerAnimationDefs[i].name != NULL; i++)
			{
				if (Q_stricmp(token, playerAnimationDefs[i].name) == 0)
				{
					anims->standAnim = playerAnimationDefs[i].num;
					break;
				}
			}
			if (!playerAnimationDefs[i].name)
			{
				// failed to find token.
				Com_Printf("unknown animation name %s in weapon group anims\n", token);
			}
			continue;
		} else if (Q_stricmpn(token, "attackAnim", 9) == 0) {
			int atkIndex = atoi(&token[10]);

			// if attackAnim1, attackAnim2, attackAnim3, ...
			if (atkIndex > 0) {
				atkIndex--;
			}
			if (atkIndex >= MAX_WG_ATK_ANIMS) {
				atkIndex = MAX_WG_ATK_ANIMS-1;
			}

			token = COM_Parse( p );
			if ( !*token ) {
				break;
			}
			for (i = 0; playerAnimationDefs[i].name != NULL; i++)
			{
				if (Q_stricmp(token, playerAnimationDefs[i].name) == 0)
				{
					anims->attackAnim[atkIndex] = playerAnimationDefs[i].num;
					break;
				}
			}
			if (!playerAnimationDefs[i].name)
			{
				// failed to find token.
				Com_Printf("unknown animation name %s in weapon group anims\n", token);
			}
			continue;
		}
		Com_Printf( "unknown token '%s' [in animation block]\n", token );
	}
	return qfalse;
}

static void BG_SetupWeaponGroup(bg_weapongroupinfo_t *weaponGroup, gitem_t *weaponItem, const char *name, int num)
{
	if (weaponGroup) {
		// Set name
		Com_sprintf(weaponGroup->name, sizeof (weaponGroup->name), "%s", name);

		// Set randomSpawn
		weaponGroup->randomSpawn = qtrue;

		// Setup Animations
		weaponGroup->normalAnims.standAnim = TORSO_STAND;
		weaponGroup->normalAnims.attackAnim[0] = TORSO_ATTACK;
		weaponGroup->normalAnims.numAttackAnims = 1;
		weaponGroup->primaryAnims.standAnim = TORSO_STAND2;
		weaponGroup->primaryAnims.attackAnim[0] = TORSO_ATTACK2;
		weaponGroup->primaryAnims.numAttackAnims = 1;

		// Set item pointer to non-NULL
		weaponGroup->item = &bg_iteminfo[0];

		// Set weapon pointers (so I don't always have to check if it is NULL).
		weaponGroup->weapon[0] = &bg_weaponinfo[0];
		weaponGroup->weapon[1] = &bg_weaponinfo[0];
	}

	if (weaponItem) {
		// Setup item
		Com_sprintf(weaponItem->pickup_sound, sizeof (weaponItem->pickup_sound), "sound/misc/w_pkup.wav");
		weaponItem->quantity = 0; // ammo
		weaponItem->giType = IT_WEAPON;
		weaponItem->giTag = num;
	}
}

static qboolean WeaponGroup_Parse(char **p) {
	char *token;
	bg_weapongroupinfo_t weaponGroup;
	gitem_t weaponItem;

	token = COM_ParseExt(p, qtrue);
	if ( !*token ) {
		Com_Printf("Warning: weapongroup missing name\n");
		return qfalse;
	}

	Com_Memset(&weaponGroup, 0, sizeof (weaponGroup));
	Com_Memset(&weaponItem, 0, sizeof (weaponItem));

	BG_SetupWeaponGroup(&weaponGroup, &weaponItem, token, -1);

	token = COM_ParseExt(p, qtrue);
	if (token[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {
		token = COM_ParseExt(p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			int num = BG_WeaponGroupIndexForName(weaponGroup.name);
			if (!num)
			{
				// Use free slot.
				num = BG_NumWeaponGroups();
			}

			// Copy to weapon group info
			Com_Memcpy(&bg_weapongroupinfo[num], &weaponGroup, sizeof (weaponGroup));

			// Set weapon group number
			weaponItem.giTag = num;

			// Setup pickup item
			num = BG_ItemIndexForName(weaponItem.classname);
			if (!num)
			{
				// Use free slot.
				num = BG_NumItems();
			}

			// Save pointer to item
			bg_weapongroupinfo[weaponItem.giTag].item = &bg_iteminfo[num];

			// Copy to weapon group info
			Com_Memcpy(&bg_iteminfo[num], &weaponItem, sizeof (weaponItem));

			//Com_Printf("Loaded weapon group [%s]\n", weaponGroup.name);
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		if (Q_stricmp(token, "normalAnims") == 0) {
			if (WeaponGroupAnims_Parse(p, &weaponGroup.normalAnims))
				continue;
			else
				break;
		} else if (Q_stricmp(token, "primaryAnims") == 0) {
			if (WeaponGroupAnims_Parse(p, &weaponGroup.primaryAnims))
				continue;
			else
				break;
		} else if ( !Q_stricmp( token, "clone" ) ) {
			char name[MAX_QPATH]; // Save name
			int num;
			token = COM_Parse( p );
			if ( !*token ) {
				break;
			}
			num = BG_WeaponGroupIndexForName(token);
			if (num) {
				Q_strncpyz(name, weaponGroup.name, MAX_QPATH); // backup name
				memcpy(&weaponGroup, &bg_weapongroupinfo[num], sizeof (weaponGroup));
				Q_strncpyz(weaponGroup.name, name, MAX_QPATH); // use backed up name
			}
			continue;
		}
		// ITEM START
		else PARSE_STRING(token, "itemName", weaponItem.classname)
		else PARSE_STRING(token, "pickupSound", weaponItem.pickup_sound)
		else PARSE_STRING(token, "pickupModel", weaponItem.world_model[0])
		else PARSE_STRING(token, "iconName", weaponItem.icon)
		else PARSE_STRING(token, "pickupName", weaponItem.pickup_name)
		else PARSE_INTEGER(token, "pickupAmmo", weaponItem.quantity)
		// ITEM END
		else PARSE_INTEGER(token, "randomSpawn", weaponGroup.randomSpawn)
		else PARSE_STRING(token, "handsModel", weaponGroup.handsModelName)
		else PARSE_STRING(token, "readySound", weaponGroup.readySoundName)
		else PARSE_STRING(token, "firingSound", weaponGroup.firingSoundName)
		else PARSE_STRING(token, "firingStoppedSound", weaponGroup.firingStoppedSoundName)
		else if ( !Q_stricmp( token, "weapon_primary" )
			|| !Q_stricmp( token, "weapon_secondary" ) )
		{
			int hand = !Q_stricmp( token, "weapon_secondary" ) ? HAND_SECONDARY : HAND_PRIMARY;
			token = COM_Parse( p );
			if ( *token ) {
				int w = BG_WeaponIndexForName(token);
				weaponGroup.weapon[hand] = &bg_weaponinfo[w];
				weaponGroup.weaponnum[hand] = w;

				if (!w) {
					Com_Printf("Can't find weapon [%s] for [%s]\n", token, weaponGroup.name);
				}
			} else {
				Com_Printf("Missing token for %s\n", hand == HAND_SECONDARY ? "weapon_secondary" : "weapon_primary");
			}
			continue;
		}
		Com_Printf( "unknown token '%s' in weapongroup %s\n", token, weaponGroup.name );
	}
	return qfalse;
}
#endif

/*
======================
BG_ParseItemInfoFile
======================
*/
qboolean BG_ParseItemInfoFile( const char *filename ) {
	char		*text_p;
	int			len;
	char		*token;
	char		text[30000]; // ZTM: NOTE: Was 20000, but the file became too long...
	fileHandle_t	f;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}
	if ( (unsigned)len >= sizeof( text ) - 1 ) {
		Com_Printf( "File %s too long\n", filename );
		trap_FS_FCloseFile( f );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;

	// read optional parameters
	while ( 1 ) {
		token = COM_Parse( &text_p );

		if ( !*token ) {
			break;
		}

		if ( Q_stricmp( token, "item" ) == 0 ) {
			if (!ItemInfo_Parse(&text_p))
			{
				break;
			}
#ifdef TA_WEAPSYS
		} else if ( Q_stricmp( token, "projectile" ) == 0 ) {
			if (!Projectile_Parse(&text_p))
			{
				break;
			}
		} else if ( Q_stricmp( token, "weapon" ) == 0 ) {
			if (!Weapon_Parse(&text_p))
			{
				break;
			}
		} else if ( Q_stricmp( token, "weapongroup" ) == 0 ) {
			if (!WeaponGroup_Parse(&text_p))
			{
				break;
			}
#endif
		} else {
			Com_Printf( "unknown token '%s' in %s\n", token, filename );
		}
	}

	return qtrue;
}

#if 0 // TA_WEAPSYS
// ZTM: Weapon info debuging tool
void BG_DumpWeaponInfo(void)
{
	int i, j;
	bg_projectileinfo_t *projectile;
	bg_weaponinfo_t *weapon;
	bg_bladeinfo_t *blade;
	bg_weapongroupinfo_t *weaponGroup;
	char text[256] = { 0 };
	fileHandle_t f;

	Com_Printf("Dumping weapon info\n");

	if (trap_FS_FOpenFile("weapondump.txt", &f, FS_WRITE) < 0) {
		Com_Printf("  Failed to open file for write.\n");
		return;
	}

#define FS_Printf1(x) { text[0] = 0; trap_FS_Write(x, strlen(x)+1, f); }
#define FS_Printf2(x, y) { text[0] = 0; Com_sprintf(text, sizeof (text), x, y); trap_FS_Write(text, strlen(text)+1, f); }
#define FS_Printf3(x, y, z) { text[0] = 0; Com_sprintf(text, sizeof (text), x, y, z); trap_FS_Write(text, strlen(text)+1, f); }
#define FS_Printf4(w, x, y, z) { text[0] = 0; Com_sprintf(text, sizeof (text), w, x, y, z); trap_FS_Write(text, strlen(text)+1, f); }

	FS_Printf1("// ZTM: TODO: Dump items\n\n");

	// Dump projectiles
	for (i = 0; i < MAX_BG_PROJ; i++)
	{
		projectile = &bg_projectileinfo[i];
		if (projectile->name[0] == '\0')
			continue;
		FS_Printf2("projectile \"%s\"\r\n", projectile->name);
		FS_Printf1("{\r\n");
		FS_Printf2("\tmodel \"%s\"\r\n", projectile->model);
		FS_Printf2("\tdamageAttacker %d\r\n", projectile->damageAttacker);
		FS_Printf2("\tdamage %d\r\n", projectile->damage);
		FS_Printf2("\tsplashdamage %d\r\n", projectile->splashDamage);
		FS_Printf2("\tsplashRadius %d\r\n", projectile->splashRadius);
		FS_Printf2("\tspeed %d\r\n", projectile->speed);
		FS_Printf2("\ttimetolive %d\r\n", projectile->timetolive);
		FS_Printf2("\tshootable %d\r\n", projectile->shootable);
		FS_Printf2("\ttrailType %d\r\n", projectile->trailType);
		FS_Printf2("\ttrailShader0 \"%s\"\r\n", projectile->trailShaderName[0]);
		FS_Printf2("\ttrailShader1 \"%s\"\r\n", projectile->trailShaderName[1]);
		FS_Printf2("\ttrailRadius %d\r\n", projectile->trailRadius);
		FS_Printf2("\ttrailTime %d\r\n", projectile->trailTime);
		FS_Printf2("\tdeathType %d\r\n", projectile->deathType);
		FS_Printf2("\texplosionType %d\r\n", projectile->explosionType);
		FS_Printf2("\tmissileDlight %d\r\n", projectile->missileDlight);
		FS_Printf4("\tmissileDlightColor %f %f %f\r\n", projectile->missileDlightColor[0], projectile->missileDlightColor[1], projectile->missileDlightColor[2]);
		FS_Printf2("\tmissileSound \"%s\"\r\n", projectile->missileSoundName);
		FS_Printf2("\tsprite \"%s\"\r\n", projectile->sprite);
		FS_Printf2("\tspriteRadius %d\r\n", projectile->spriteRadius);
		FS_Printf2("\tspinType %d\r\n", projectile->spinType);
		FS_Printf2("\tbounceType %d\r\n", projectile->bounceType);
		FS_Printf2("\tmaxBounces %d\r\n", projectile->maxBounces);
		FS_Printf2("\tuseGravity %d\r\n", (projectile->flags & PF_USE_GRAVITY));

		FS_Printf2("\tstickOnImpact %d\r\n", projectile->stickOnImpact);
		FS_Printf2("\tfallToGround %d\r\n", projectile->fallToGround);
		FS_Printf2("\tspdRndAdd %d\r\n", projectile->spdRndAdd);
		FS_Printf2("\tspread %d\r\n", projectile->spread);
		FS_Printf2("\tnumProjectiles %d\r\n", projectile->numProjectiles);

		FS_Printf2("\thoming %d\r\n", projectile->homing);
		FS_Printf2("\tgrappling %d\r\n", projectile->grappling);


		FS_Printf2("\tinstantDamage %d\r\n", projectile->instantDamage);
		FS_Printf2("\tmaxHits %d\r\n", projectile->maxHits);

		FS_Printf2("\thitMarkFadeAlpha %d\r\n", (projectile->flags & PF_HITMARK_FADE_ALPHA));
		FS_Printf2("\thitMarkColorize %d\r\n", (projectile->flags & PF_HITMARK_COLORIZE));
		FS_Printf2("\thitMarkName \"%s\"\r\n", projectile->hitMarkName);
		FS_Printf2("\thitMarkRadius %d\r\n", projectile->hitMarkRadius);
		FS_Printf2("\thitSound0 \"%s\"\r\n", projectile->hitSoundName[0]);
		FS_Printf2("\thitSound1 \"%s\"\r\n", projectile->hitSoundName[1]);
		FS_Printf2("\thitSound2 \"%s\"\r\n", projectile->hitSoundName[2]);
		FS_Printf2("\thitPlayerSound \"%s\"\r\n", projectile->hitPlayerSoundName);
		FS_Printf2("\thitMetalSound \"%s\"\r\n", projectile->hitMetalSoundName);

		FS_Printf2("\timpactMarkFadeAlpha %d\r\n", (projectile->flags & PF_IMPACTMARK_FADE_ALPHA));
		FS_Printf2("\timpactMarkColorize %d\r\n", (projectile->flags & PF_IMPACTMARK_COLORIZE));
		FS_Printf2("\timpactMarkName \"%s\"\r\n", projectile->impactMarkName);
		FS_Printf2("\timpactMarkRadius %d\r\n", projectile->impactMarkRadius);
		FS_Printf2("\timpactSound0 \"%s\"\r\n", projectile->impactSoundName[0]);
		FS_Printf2("\timpactSound1 \"%s\"\r\n", projectile->impactSoundName[1]);
		FS_Printf2("\timpactSound2 \"%s\"\r\n", projectile->impactSoundName[2]);
		FS_Printf2("\timpactPlayerSound \"%s\"\r\n", projectile->impactPlayerSoundName);
		FS_Printf2("\timpactMetalSound \"%s\"\r\n", projectile->impactMetalSoundName);

		FS_Printf2("\ttriggerSound \"%s\"\r\n", projectile->triggerSoundName);
		FS_Printf2("\ttickSound \"%s\"\r\n", projectile->tickSoundName);

		FS_Printf1("}\r\n\r\n");
	}
	projectile = NULL;

	// Dump weapons
	for (i = 0; i < MAX_BG_WEAPONS; i++)
	{
		weapon = &bg_weaponinfo[i];
		if (weapon->name[0] == '\0')
			continue;
		FS_Printf2("weapon \"%s\"\r\n", weapon->name);
		FS_Printf1("{\r\n");
		FS_Printf2("\tmodel \"%s\"\r\n", weapon->model);
		FS_Printf2("\tweapontype %s\r\n", weapontypeNames[weapon->weapontype]);
		FS_Printf2("\tmod %s\r\n", modNames[weapon->mod]);
		FS_Printf2("\tattackDelay %d\r\n", weapon->attackDelay);
		FS_Printf2("\timpactMarkName \"%s\"\r\n", weapon->impactMarkName);
		FS_Printf2("\timpactMarkRadius %d\r\n", weapon->impactMarkRadius);

		FS_Printf2("\timpactMarkFadeAlpha %d\r\n", (weapon->flags & WIF_IMPACTMARK_FADE_ALPHA));
		FS_Printf2("\timpactMarkColorize %d\r\n", (weapon->flags & WIF_IMPACTMARK_COLORIZE));
		FS_Printf2("\talwaysDamage %d\r\n", (weapon->flags & WIF_ALWAYS_DAMAGE));
		FS_Printf2("\tcuts %d\r\n", (weapon->flags & WIF_CUTS));
		FS_Printf2("\tcontinuousFlash %d\r\n", (weapon->flags & WIF_CONTINUOUS_FLASH));
		FS_Printf2("\tejectBrass %d\r\n", (weapon->flags & WIF_EJECT_BRASS));
		FS_Printf2("\tejectBrass2 %d\r\n", (weapon->flags & WIF_EJECT_BRASS2));
		FS_Printf2("\tejectSmoke %d\r\n", (weapon->flags & WIF_EJECT_SMOKE));
		FS_Printf2("\tejectSmoke2 %d\r\n", (weapon->flags & WIF_EJECT_SMOKE2));
		FS_Printf2("\tbarrelIdleUseGravity %d\r\n", (weapon->flags & WIF_BARREL_IDLE_USE_GRAVITY));
		FS_Printf2("\tinitialEffectOnly %d\r\n", (weapon->flags & WIF_INITIAL_EFFECT_ONLY));

		FS_Printf2("\tsplashMod %s\r\n", modNames[weapon->splashMod]);
		if (weapon->flashColor[0] == (float)'c' && weapon->flashColor[1] == 1.0f && weapon->flashColor[2] == 0)
			FS_Printf1("\tflashColor color1\r\n");
		else if (weapon->flashColor[0] == (float)'c' && weapon->flashColor[1] == 2.0f && weapon->flashColor[2] == 0)
			FS_Printf1("\tflashColor color2\r\n");
		else if (weapon->flashColor[0] == (0 && weapon->flashColor[1] == 0 && weapon->flashColor[2] == 0)
			FS_Printf1("\tflashColor none\r\n");
		else
			FS_Printf4("\tflashColor %f %f %f\r\n", weapon->flashColor[0], weapon->flashColor[1], weapon->flashColor[2]);
		FS_Printf2("\tflashSound0 \"%s\"\r\n", weapon->flashSoundName[0]);
		FS_Printf2("\tflashSound1 \"%s\"\r\n", weapon->flashSoundName[1]);
		FS_Printf2("\tflashSound2 \"%s\"\r\n", weapon->flashSoundName[2]);
		FS_Printf2("\tflashSound3 \"%s\"\r\n", weapon->flashSoundName[3]);
		FS_Printf2("\timpactSound0 \"%s\"\r\n", weapon->impactSoundName[0]);
		FS_Printf2("\timpactSound1 \"%s\"\r\n", weapon->impactSoundName[1]);
		FS_Printf2("\timpactSound2 \"%s\"\r\n", weapon->impactSoundName[2]);
		FS_Printf2("\timpactPlayerSound \"%s\"\r\n", weapon->impactPlayerSoundName);
		FS_Printf2("\timpactMetalSound \"%s\"\r\n", weapon->impactMetalSoundName);
		FS_Printf2("\tprojectile \"%s\"\r\n", weapon->proj->name);

		// Dump blades
		for (j = 0; j < MAX_WEAPON_BLADES; j++)
		{
			blade = &weapon->blades[j];
			FS_Printf1("\t{\r\n");
			FS_Printf2("\t\tdamage %d\r\n", blade->damage);
			FS_Printf4("\t\torigin %f %f %f\r\n", blade->origin[0], blade->origin[1], blade->origin[2]);
			FS_Printf4("\t\ttip %f %f %f\r\n", blade->tip[0], blade->tip[1], blade->tip[2]);
			FS_Printf2("\t\ttrailStyle %d\r\n", blade->trailStyle);
			FS_Printf1("\t}\r\n");
		}
		FS_Printf1("}\r\n\r\n");
	}
	weapon = NULL;

	// Dump weapon groups
	for (i = 0; i < MAX_BG_WEAPON_GROUPS; i++)
	{
		weaponGroup = &bg_weapongroupinfo[i];
		if (weaponGroup->name[0] == '\0')
			break;
		FS_Printf2("weapongroup \"%s\"\r\n", weaponGroup->name);
		FS_Printf1("{\r\n");
		FS_Printf2("\trandomSpawn %d\r\n", weaponGroup->randomSpawn);

		// item settings
		FS_Printf2("\titemName \"%s\"\r\n", weaponGroup->item->classame);
		FS_Printf2("\tpickupSound \"%s\"\r\n", weaponGroup->item->pickup_sound);
		FS_Printf2("\tpickupModel \"%s\"\r\n", weaponGroup->item->world_model[0]);
		FS_Printf2("\ticonName \"%s\"\r\n", weaponGroup->item->icon);
		FS_Printf2("\tpickupName \"%s\"\r\n", weaponGroup->item->pickup_name);
		FS_Printf2("\tpickupAmmo %i\r\n", weaponGroup->item->quantity);

		FS_Printf2("\thandsModel \"%s\"\r\n", weaponGroup->handsModelName);
		FS_Printf2("\treadySound \"%s\"\r\n", weaponGroup->readySoundName);
		FS_Printf2("\tfiringSound \"%s\"\r\n", weaponGroup->firingSoundName);
		FS_Printf2("\tfiringStoppedSound \"%s\"\r\n", weaponGroup->firingStoppedSoundName);

		if (weaponGroup->weaponnum[0]) {
			FS_Printf2("\tweapon_primary \"%s\"\r\n", weaponGroup->weapon[0]->name);
		}
		if (weaponGroup->weaponnum[1]) {
			FS_Printf2("\tweapon_secondary \"%s\"\r\n", weaponGroup->weapon[1]->name);
		}

		// Animations
		FS_Printf1("\r\n");
		FS_Printf1("\tnormalAnims\r\n");
		FS_Printf1("\t{\r\n");
		FS_Printf2("\t\tstandAnim \"%s\"\r\n", playerAnimationDefs[weaponGroup->normalAnims.standAnim].name);
		for (j = 0; j < MAX_WG_ATK_ANIMS; j++)
		{
			if (weaponGroup->normalAnims.attackAnim[j] == 0)
				continue;
			FS_Printf3("\t\tattackAnim%s \"%s\"\r\n", j > 0? va("%d", i+1): "", playerAnimationDefs[weaponGroup->normalAnims.attackAnim[j]].name);
		}
		FS_Printf1("\t}\r\n");
		FS_Printf1("\tprimaryAnims\r\n");
		FS_Printf1("\t{\r\n");
		FS_Printf2("\t\tstandAnim \"%s\"\r\n", playerAnimationDefs[weaponGroup->primaryAnims.standAnim].name);
		for (j = 0; j < MAX_WG_ATK_ANIMS; j++)
		{
			if (weaponGroup->primaryAnims.attackAnim[j] == 0)
				continue;
			FS_Printf3("\t\tattackAnim%s \"%s\"\r\n", j > 0? va("%d", i+1): "", playerAnimationDefs[weaponGroup->primaryAnims.attackAnim[j]].name);
		}
		FS_Printf1("\t}\r\n");

		FS_Printf1("}\r\n\r\n");
	}
	weaponGroup = NULL;

	trap_FS_FCloseFile(f);
}
#endif

/*
=========
BG_InitItemInfo
=========
*/
void BG_InitItemInfo(void)
{
	int			numdirs;
	char		filename[128];
	char		dirlist[1024];
	char*		dirptr;
	int			i;
	int			dirlen;

	// ZTM: NOTE: In the past game/cgame/ui DLLs shared the same memory and only parsed files once.
	//            QVMs did not support it and it was removed from the engine in Spearmint 0.4.
	//            Since CGame/UI were merged it still makes sense to check if already inited -- though, could probably remove call from UI?
	bg_common = trap_HeapMalloc( sizeof ( bg_commonInfo_t ) );

	if (bg_itemsys_init)
		return;

#ifdef TA_WEAPSYS
	// Setup dummys (0 means not valid or not found)
	strcpy(bg_projectileinfo[0].name, "p_none");
	bg_projectileinfo[0].numProjectiles = 1;
	bg_projectileinfo[0].maxHits = 1;

	strcpy(bg_weaponinfo[0].name, "w_none");
	bg_weaponinfo[0].weapontype = WT_NONE;
	bg_weaponinfo[0].attackDelay = 400;
	bg_weaponinfo[0].flashColor[0] = 1.0f;
	bg_weaponinfo[0].flashColor[1] = 1.0f;
	bg_weaponinfo[0].flashColor[2] = 1.0f;
	bg_weaponinfo[0].impactMarkRadius = 16;
	bg_weaponinfo[0].projnum = 0;
	bg_weaponinfo[0].proj = &bg_projectileinfo[0];

	// WP_NONE
	BG_SetupWeaponGroup(&bg_weapongroupinfo[0], &bg_iteminfo[0], "wp_none", 0);
	strcpy(bg_iteminfo[0].pickup_name, "None");
#endif

	bg_common->numitems = 1;

	// Load main data files
	BG_ParseItemInfoFile("scripts/iteminfo.txt");
#ifdef TA_WEAPSYS
	BG_ParseItemInfoFile("scripts/weaponinfo.txt");
#endif

	// Load all weapons from .item files
	numdirs = trap_FS_GetFileList("scripts", ".item", dirlist, 1024 );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		BG_ParseItemInfoFile(filename);
	}

#ifdef TA_WEAPSYS
	// Load all weapons from .weap files
	numdirs = trap_FS_GetFileList("scripts", ".weap", dirlist, 1024 );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		BG_ParseItemInfoFile(filename);
	}

	// Missing weapon info, avoid crashing or other errors.
	if (!bg_projectileinfo[1].name[0])
	{
		strcpy(bg_projectileinfo[1].name, "p_gun");
		bg_projectileinfo[1].numProjectiles = 1;
		bg_projectileinfo[1].maxHits = 1;
		bg_projectileinfo[1].instantDamage = qtrue;
		bg_projectileinfo[1].damage = 10;
		bg_numprojectiles = 2;
	}

	if (!bg_weaponinfo[1].name[0])
	{
		strcpy(bg_weaponinfo[1].name, "w_gun");
		bg_weaponinfo[1].weapontype = WT_GUN;
		bg_weaponinfo[1].attackDelay = 400;
		bg_weaponinfo[1].flashColor[0] = 1.0f;
		bg_weaponinfo[1].flashColor[1] = 1.0f;
		bg_weaponinfo[1].flashColor[2] = 1.0f;
		bg_weaponinfo[1].projnum = 1;
		bg_weaponinfo[1].proj = &bg_projectileinfo[1];
		bg_numweapons = 2;
	}

	if (!bg_weapongroupinfo[1].name[0])
	{
		BG_SetupWeaponGroup(&bg_weapongroupinfo[1], &bg_iteminfo[bg_numitems], "wp_gun", 1);
		bg_numitems++;
		strcpy(bg_weapongroupinfo[1].item->pickup_name, "Dummy Weapon");
		strcpy(bg_weapongroupinfo[1].item->classname, "weapon_gun");
		bg_weapongroupinfo[1].item->quantity = 10; // ammo
		bg_weapongroupinfo[1].weaponnum[0] = 1;
		bg_weapongroupinfo[1].weapon[0] = &bg_weaponinfo[1];
		bg_numweapongroups = 2;
	}
#endif

	// Done setting up the item system.
	bg_itemsys_init = qtrue;

#if 0 // TA_WEAPSYS
	// So I can see if it is loading correctly.
	BG_DumpWeaponInfo();
#endif

	// Count holdable items
	for (i = 1; i < BG_NumItems(); i++)
	{
		if (bg_iteminfo[i].giType == IT_HOLDABLE) {
			bg_numholdables++;
		}
	}
}

#ifdef TA_WEAPSYS
/*
=================
BG_PlayerRunning

Returns qtrue if player is not standing.
=================
*/
qboolean BG_PlayerRunning(vec3_t velocity)
{
	vec_t xyspeed = sqrt( velocity[0] * velocity[0] +
		velocity[1] * velocity[1] );

	return (xyspeed > 200);
}

/*
=================
BG_MaxAttackIndex
=================
*/
int BG_MaxAttackIndex(playerState_t *ps)
{
	bg_weapongroup_anims_t *anims;

	// Select animations to count
	if (ps->eFlags & EF_PRIMARY_HAND) {
		anims = &bg_weapongroupinfo[ps->weapon].primaryAnims;
	} else {
		anims = &bg_weapongroupinfo[ps->weapon].normalAnims;
	}

	return anims->numAttackAnims;
}

/*
=================
BG_AttackIndexForPlayerState

Check if running or jumping and return proper attack animation index.
=================
*/
int BG_AttackIndexForPlayerState(playerState_t *ps)
{
	int max_combo = BG_MaxAttackIndex(ps);
	int atkIndex;

	if (max_combo <= 1) {
		return 0;
	}

	if (ps->groundEntityNum == ENTITYNUM_NONE) {
		// Jump attack only uses last attack animation
		atkIndex = max_combo-1;
	} else {
		if (BG_PlayerRunning(ps->velocity)) {
			// ZTM: if running "max_combo /= 2"; like in LoZ:TP
			// Run attack only uses the first half of the attack animations (usually 2)
			max_combo /= 2;
		}
		atkIndex = ps->meleeAttack % max_combo;
	}

	return atkIndex;
}

/*
==============
BG_TorsoStandForPlayerState
==============
*/
animNumber_t BG_TorsoStandForPlayerState(playerState_t *ps, bg_playercfg_t *playercfg)
{
	if (!ps || ps->weapon < 0 || ps->weapon >= BG_NumWeaponGroups() || !playercfg)
	{
		return TORSO_STAND;
	}

#ifdef TURTLEARENA // PLAYERS
	if ((ps->eFlags & EF_PLAYER_WAITING) && (playercfg->animations[BOTH_WAITING].prefixType & AP_TORSO)) {
		return BOTH_WAITING;
	}
#endif

	if (ps->eFlags & EF_PRIMARY_HAND)
	{
		return (animNumber_t)bg_weapongroupinfo[ps->weapon].primaryAnims.standAnim;
	}
	return (animNumber_t)bg_weapongroupinfo[ps->weapon].normalAnims.standAnim;
}

/*
==============
BG_TorsoAttackForPlayerState
==============
*/
animNumber_t BG_TorsoAttackForPlayerState(playerState_t *ps)
{
	int atkIndex;

	if (!ps || ps->weapon < 0 || ps->weapon >= BG_NumWeaponGroups())
	{
		return TORSO_ATTACK;
	}

	atkIndex = BG_AttackIndexForPlayerState(ps);

	if (ps->eFlags & EF_PRIMARY_HAND)
	{
		return (animNumber_t)bg_weapongroupinfo[ps->weapon].primaryAnims.attackAnim[atkIndex];
	}
	return (animNumber_t)bg_weapongroupinfo[ps->weapon].normalAnims.attackAnim[atkIndex];
}

/*
==============
BG_LegsStandForPlayerState
==============
*/
animNumber_t BG_LegsStandForPlayerState(playerState_t *ps, bg_playercfg_t *playercfg)
{
	animNumber_t anim;

	if (!ps) {
		return LEGS_IDLE;
	}

	if (ps->pm_flags & PMF_DUCKED)
	{
		return LEGS_IDLECR;
	}

	if ((ps->weapon < 0 || ps->weapon >= BG_NumWeaponGroups()) || !playercfg)
	{
		return LEGS_IDLE;
	}

#ifdef TURTLEARENA // PLAYERS
	if ((ps->eFlags & EF_PLAYER_WAITING) && (playercfg->animations[BOTH_WAITING].prefixType & AP_LEGS)) {
		return BOTH_WAITING;
	}
#endif

	if (ps->eFlags & EF_PRIMARY_HAND) {
		anim = bg_weapongroupinfo[ps->weapon].primaryAnims.standAnim;
	} else {
		anim = bg_weapongroupinfo[ps->weapon].normalAnims.standAnim;
	}

	if (playercfg->animations[anim].prefixType & AP_LEGS) {
		return anim;
	} else {
		return LEGS_IDLE;
	}
}

/*
==============
BG_LegsAttackForPlayerState
==============
*/
animNumber_t BG_LegsAttackForPlayerState(playerState_t *ps, bg_playercfg_t *playercfg)
{
	animNumber_t anim;
	int atkIndex;

	if (!ps || !playercfg || ps->weapon < 0 || ps->weapon >= BG_NumWeaponGroups()
		|| (ps->pm_flags & PMF_DUCKED) || (ps->groundEntityNum == ENTITYNUM_NONE)
		|| BG_PlayerRunning(ps->velocity))
	{
		return -1; // no change
	}

	atkIndex = BG_AttackIndexForPlayerState(ps);

	if (ps->eFlags & EF_PRIMARY_HAND) {
		anim = bg_weapongroupinfo[ps->weapon].primaryAnims.attackAnim[atkIndex];
	} else {
		anim = bg_weapongroupinfo[ps->weapon].normalAnims.attackAnim[atkIndex];
	}

	if (playercfg->animations[anim].prefixType & AP_LEGS) {
		return anim;
	} else {
		return -1;
	}
}

/*
==============
BG_TorsoStandForWeapon

For ui/q3_ui
==============
*/
animNumber_t BG_TorsoStandForWeapon(weapon_t weaponnum)
{
	return (animNumber_t)bg_weapongroupinfo[weaponnum].normalAnims.standAnim;
}

/*
==============
BG_TorsoAttackForWeapon

For ui/q3_ui
==============
*/
animNumber_t BG_TorsoAttackForWeapon(weapon_t weaponnum, unsigned int atkIndex)
{
	atkIndex = atkIndex % bg_weapongroupinfo[weaponnum].normalAnims.numAttackAnims;
	return (animNumber_t)bg_weapongroupinfo[weaponnum].normalAnims.attackAnim[atkIndex];
}

/*
==============
BG_LegsStandForWeapon

For ui/q3_ui
==============
*/
animNumber_t BG_LegsStandForWeapon(bg_playercfg_t *playercfg, weapon_t weaponnum)
{
	if (playercfg && playercfg->animations[bg_weapongroupinfo[weaponnum].normalAnims.standAnim].prefixType & AP_LEGS) {
		return (animNumber_t)bg_weapongroupinfo[weaponnum].normalAnims.standAnim;
	}

	return LEGS_IDLE;
}

/*
==============
BG_LegsAttackForWeapon

For ui/q3_ui
==============
*/
animNumber_t BG_LegsAttackForWeapon(bg_playercfg_t *playercfg, weapon_t weaponnum, unsigned int atkIndex)
{
	atkIndex = atkIndex % bg_weapongroupinfo[weaponnum].normalAnims.numAttackAnims;
	if (playercfg && playercfg->animations[bg_weapongroupinfo[weaponnum].normalAnims.attackAnim[atkIndex]].prefixType & AP_LEGS) {
		return (animNumber_t)bg_weapongroupinfo[weaponnum].normalAnims.attackAnim[atkIndex];
	}

	return -1;
}

qboolean BG_PlayerAttackAnim(animNumber_t aa)
{
	animNumber_t a = (aa & ~ANIM_TOGGLEBIT);
#if 0
	int i;
	for (i = 0; i < WT_MAX; i++)
	{
		if (bg_weapontypeinfo[i].attackAnim == a)
		{
			return qtrue;
		}
	}
	return qfalse;
#else
	return (a == TORSO_ATTACK || a == TORSO_ATTACK2
#ifdef TURTLEARENA // PLAYERS
	|| (a >= TORSO_ATTACK_GUN_PRIMARY && a <= TORSO_ATTACK_NUNCHUCKS1_PRIMARY_C)
#endif
	);
#endif
}

/*
==============
BG_PlayerStandAnim

For bg/game/game and ui/q3_ui

Returns true if aa is a valid standing/idle animation based on playercfg and prefixBit
==============
*/
qboolean BG_PlayerStandAnim(bg_playercfg_t *playercfg, int prefixBit, animNumber_t aa)
{
	animNumber_t a = (aa & ~ANIM_TOGGLEBIT);
#if 0
	int i;

	for (i = 0; i < WT_MAX; i++) {
		if (bg_weapontypeinfo[i].standAnim == a) {
			return (playercfg->animations[a].prefixType & prefixBit);
		}
	}

	if (prefixBit & AP_LEGS) {
		return (a == LEGS_IDLE);
	}

	return (a == TORSO_STAND || a == TORSO_STAND2);
#else
	return (((((prefixBit & AP_TORSO) && (a == TORSO_STAND || a == TORSO_STAND2))
			|| ((prefixBit & AP_LEGS) && a == LEGS_IDLE))
#ifdef TURTLEARENA // PLAYERS
		|| (a >= TORSO_STAND_GUN_PRIMARY && a <= TORSO_STAND_NUNCHUCKS1_PRIMARY)
#endif
		) && (playercfg->animations[a].prefixType & prefixBit));
#endif
}

/*
==============
BG_WeaponHandsForPlayerState
==============
*/
int BG_WeaponHandsForPlayerState(playerState_t *ps)
{
#ifdef TA_PLAYERSYS // LADDER
	if ((ps->eFlags & EF_LADDER) && ps->weapon == ps->stats[STAT_DEFAULTWEAPON]) {
		return HB_NONE;
	}
#endif
	if (ps->eFlags & EF_PRIMARY_HAND) {
		return HB_PRIMARY;
	}

	return BG_WeaponHandsForWeaponNum(ps->weapon);
}

/*
==============
BG_WeaponHandsForWeaponNum
==============
*/
int BG_WeaponHandsForWeaponNum(weapon_t weaponnum)
{
	int hands = 0; // HB_NONE
	int i;

	for (i = 0; i < MAX_HANDS; i++)
	{
		if (bg_weapongroupinfo[weaponnum].weaponnum[i])
		{
			hands |= HAND_TO_HB(i);
		}
	}

	return hands;
}


qboolean BG_WeapTypeIsMelee(weapontype_t wt)
{
	return (wt == WT_MELEE || wt == WT_GAUNTLET);
}

qboolean BG_WeaponHasMelee(weapon_t weaponnum)
{
	if (weaponnum <= 0 || weaponnum >= BG_NumWeaponGroups())
		return qfalse;

	return (BG_WeapTypeIsMelee(bg_weapongroupinfo[weaponnum].weapon[0]->weapontype) ||
		BG_WeapTypeIsMelee(bg_weapongroupinfo[weaponnum].weapon[1]->weapontype));
}

qboolean BG_WeaponHasType(weapon_t weaponnum, weapontype_t wt)
{
	if (weaponnum <= 0 || weaponnum >= BG_NumWeaponGroups())
		return qfalse;

	return (bg_weapongroupinfo[weaponnum].weapon[0]->weapontype == wt ||
		bg_weapongroupinfo[weaponnum].weapon[1]->weapontype == wt);
}

qboolean BG_WeapUseAmmo(weapon_t w)
{
	// Check if the weapon group uses ammo
	return (bg_weapongroupinfo[w].item && bg_weapongroupinfo[w].item->quantity > 0);
}

/*
==============
BG_WeaponGroupTotalDamage

Returns the total damage the weaponGroup can do in one frame.
==============
*/
int BG_WeaponGroupTotalDamage(int weaponGroup)
{
	int damage = 0;
	int i, j;

	if (weaponGroup <= 1 || weaponGroup >= BG_NumWeaponGroups()) {
		return 0;
	}

	for (i = 0; i < MAX_HANDS; i++) {
		if (bg_weapongroupinfo[weaponGroup].weapon[i]->projnum) {
			damage += bg_weapongroupinfo[weaponGroup].weapon[i]->proj->damage*bg_weapongroupinfo[weaponGroup].weapon[i]->proj->numProjectiles;
		} else {
			for (j = 0; j < MAX_WEAPON_BLADES; j++) {
				damage += bg_weapongroupinfo[weaponGroup].weapon[i]->blades[j].damage;
			}
		}
	}

	return damage;
}
#endif // TA_WEAPSYS

#ifdef IOQ3ZTM // LERP_FRAME_CLIENT_LESS
/*
===============
BG_SetLerpFrameAnimation

Based on ioquake3's CG_SetLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
void BG_SetLerpFrameAnimation( lerpFrame_t *lf, animation_t *animations, int newAnimation ) {
	animation_t	*anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if ( newAnimation < 0 ) {
		Com_Error( ERR_DROP, "Bad animation number: %i", newAnimation );
	}

	anim = &animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	//if ( cg_debugAnim.integer ) {
	//	CG_Printf( "Anim: %i\n", newAnimation );
	//}
}

/*
===============
BG_RunLerpFrame


Based on CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
void BG_RunLerpFrame( lerpFrame_t *lf, animation_t *animations, int newAnimation, int time, float speedScale ) {
	int			f, numFrames;
	animation_t	*anim;

	// debugging tool to get no animations
	//if ( cg_animSpeed.integer == 0 ) {
	//	lf->oldFrame = lf->frame = lf->backlerp = 0;
	//	return;
	//}

	// see if the animation sequence is switching
	if ( newAnimation != lf->animationNumber || !lf->animation ) {
		BG_SetLerpFrameAnimation( lf, animations, newAnimation );
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( time >= lf->frameTime ) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if ( !anim->frameLerp ) {
			return;		// shouldn't happen
		}
		if ( time < lf->animationTime ) {
			lf->frameTime = lf->animationTime;		// initial lerp
		} else {
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
		f *= speedScale;		// adjust for haste, etc

		numFrames = anim->numFrames;
		if (anim->flipflop) {
			numFrames *= 2;
		}
		if ( f >= numFrames ) {
			f -= numFrames;
			if ( anim->loopFrames ) {
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			} else {
				f = numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = time;
			}
		}
		if ( anim->reversed ) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
		}
		else if (anim->flipflop && f >= anim->numFrames) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - (f%anim->numFrames);
		}
		else {
			lf->frame = anim->firstFrame + f;
		}
		if ( time > lf->frameTime ) {
			lf->frameTime = time;
			//if ( cg_debugAnim.integer ) {
			//	CG_Printf( "Clamp lf->frameTime\n");
			//}
		}
	}

	if ( lf->frameTime > time + 200 ) {
		lf->frameTime = time;
	}

	if ( lf->oldFrameTime > time ) {
		lf->oldFrameTime = time;
	}
	// calculate current lerp value
	if ( lf->frameTime == lf->oldFrameTime ) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)( time - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
}


/*
===============
BG_ClearLerpFrame
Based on CG_ClearLerpFrame
===============
*/
void BG_ClearLerpFrame( lerpFrame_t *lf, animation_t *animations, int animationNumber, int time ) {
	lf->frameTime = lf->oldFrameTime = time; // time was cg.time
	BG_SetLerpFrameAnimation( lf, animations, animationNumber );
	lf->oldFrame = lf->frame = lf->animation->firstFrame;
}
#endif

#ifdef IOQ3ZTM // BG_SWING_ANGLES
/*
==================
BG_SwingAngles

Based on Q3's CG_SwingAngles

* frametime should be;
** game - (level.time - level.previousTime)
** q3_ui - uis.frametime
** ui - uiInfo.uiDC.frameTime
** cgame - cg.frametime
==================
*/
void BG_SwingAngles( float destination, float swingTolerance, float clampTolerance,
					float speed, float *angle, qboolean *swinging, int frametime ) {
	float	swing;
	float	move;
	float	scale;

	if ( !*swinging ) {
		// see if a swing should be started
		swing = AngleSubtract( *angle, destination );
		if ( swing > swingTolerance || swing < -swingTolerance ) {
			*swinging = qtrue;
		}
	}

	if ( !*swinging ) {
		return;
	}

	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract( destination, *angle );
	scale = fabs( swing );
	if ( scale < swingTolerance * 0.5 ) {
		scale = 0.5;
	} else if ( scale < swingTolerance ) {
		scale = 1.0;
	} else {
		scale = 2.0;
	}

	// swing towards the destination angle
	if ( swing >= 0 ) {
		move = frametime * scale * speed;
		if ( move >= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	} else if ( swing < 0 ) {
		move = frametime * scale * -speed;
		if ( move <= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	}

	// clamp to no more than tolerance
	swing = AngleSubtract( destination, *angle );
	if ( swing > clampTolerance ) {
		*angle = AngleMod( destination - (clampTolerance - 1) );
	} else if ( swing < -clampTolerance ) {
		*angle = AngleMod( destination + (clampTolerance - 1) );
	}
}
#endif

#ifdef TA_PLAYERSYS
/*
===============
BG_AnimationTime

Returns the msec to play the whole animation once.
===============
*/
int BG_AnimationTime(animation_t *anim)
{
	int time = 100 * 10; // 10 fps
	if (!anim)
		return time;

	// Based on SMOKIN_GUNS' PM_AnimLength(int anim)
	time = anim->frameLerp * anim->numFrames-1 + anim->initialLerp;
	if (anim->flipflop)
	{
		time *= 2;
		time -= anim->initialLerp;
	}

	return time;
}



#ifndef TA_WEAPSYS
// These are in game, cgame, and ui, but not in bg - so its okay to use here...
int		trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void	trap_FS_Read( void *buffer, int len, fileHandle_t f );
void	trap_FS_FCloseFile( fileHandle_t f );
#endif

/*
===============
BG_SetDefaultAnimation

Players only "need" four animations
	(BOTH_DEATH1, BOTH_DEAD1, TORSO_STAND, and LEGS_IDLE)

Returns qtrue if the animation has a default which could be set.
===============
*/
qboolean BG_SetDefaultAnimation(qboolean loadedAnim[], int index, animation_t *animations)
{
#define MAX_DEFAULT_ANIMATIONS 4
	int anim[MAX_DEFAULT_ANIMATIONS]; // Animations to use, best to default.
	qboolean reversed;
	//qboolean flipflop;
	int i;

	if (loadedAnim == NULL)
	{
		Com_Printf("Error: loadedAnim is NULL in BG_SetDefaultAnimation\n");
		return qfalse;
	}

	memset(anim, -1, MAX_DEFAULT_ANIMATIONS * sizeof (int));
	reversed = qfalse;
	//flipflop = qfalse;

	// Set defaults for quake3 and missionpack animations
	switch (index)
	{
		default:
			// no defaults.
			break;

		// Set defaults for quake3 animations
		case BOTH_DEATH1:
		case BOTH_DEAD1:
			// no defaults.
			break;

		case BOTH_DEATH2:
			anim[0] = BOTH_DEATH1;
			break;
		case BOTH_DEAD2:
			anim[0] = BOTH_DEAD1;
			break;
		case BOTH_DEATH3:
			anim[0] = BOTH_DEATH1;
			anim[1] = BOTH_DEATH2;
			break;
		case BOTH_DEAD3:
			anim[0] = BOTH_DEAD1;
			anim[1] = BOTH_DEAD2;
			break;

		case TORSO_GESTURE:
		case TORSO_ATTACK:
			anim[0] = TORSO_STAND;
			break;

		case TORSO_ATTACK2:
		case TORSO_DROP:
		case TORSO_RAISE:
			anim[0] = TORSO_STAND2;
			anim[1] = TORSO_STAND;
			break;

		case TORSO_STAND:
			// no default.
			break;

		case TORSO_STAND2:
			anim[0] = TORSO_STAND;
			break;

		case LEGS_WALKCR:
			anim[0] = LEGS_IDLECR;
			anim[1] = LEGS_IDLE;
			break;

		case LEGS_WALK:
			anim[0] = LEGS_IDLE;
			break;

		case LEGS_RUN:
			anim[0] = LEGS_WALK;
			anim[1] = LEGS_IDLE;
			break;
		case LEGS_BACK: // LEGS_BACKRUN
			anim[0] = LEGS_RUN;
			anim[1] = LEGS_WALK;
			anim[2] = LEGS_IDLE;
			reversed = qtrue;
			break;
		case LEGS_SWIM:
			anim[0] = LEGS_WALK;
			anim[1] = LEGS_IDLE;
			break;

		case LEGS_JUMP:
		case LEGS_LAND:
			anim[0] = LEGS_IDLE;
			break;

		case LEGS_JUMPB:
			anim[0] = LEGS_JUMP;
			anim[1] = LEGS_IDLE;
			break;
		case LEGS_LANDB:
			anim[0] = LEGS_LAND;
			anim[1] = LEGS_IDLE;
			break;

		case LEGS_IDLE:
			// no default.
			break;

		case LEGS_IDLECR:
		case LEGS_TURN:
			anim[0] = LEGS_IDLE;
			break;

		case LEGS_BACKCR:
			anim[0] = LEGS_WALKCR;
			anim[1] = LEGS_IDLECR;
			anim[2] = LEGS_IDLE;
			reversed = qtrue;
			break;
		case LEGS_BACKWALK:
			anim[0] = LEGS_WALK;
			anim[1] = LEGS_IDLE;
			reversed = qtrue;
			break;

		// Set defaults for missionpack animations
		case TORSO_GETFLAG:
		case TORSO_GUARDBASE:
		case TORSO_PATROL:
		case TORSO_FOLLOWME:
		case TORSO_AFFIRMATIVE:
		case TORSO_NEGATIVE:
			anim[0] = TORSO_GESTURE;
			anim[1] = TORSO_STAND2;
			anim[2] = TORSO_STAND;
			//flipflop = qtrue;
			break;

#ifdef TURTLEARENA // PLAYERS
		case BOTH_LADDER_STAND:
			anim[0] = LEGS_IDLE;
			break;

		case BOTH_LADDER_UP:
			anim[0] = LEGS_WALK;
			break;

		case BOTH_LADDER_DOWN:
			anim[0] = BOTH_LADDER_UP;
			reversed = qtrue;
			break;

		case LEGS_JUMPB_LOCKON:
			anim[0] = LEGS_JUMPB;
			anim[1] = LEGS_JUMP;
			anim[2] = LEGS_IDLE;
			break;
		case LEGS_LANDB_LOCKON:
			anim[0] = LEGS_LANDB;
			anim[1] = LEGS_LAND;
			anim[2] = LEGS_IDLE;
			break;
#endif
	}


#ifdef TURTLEARENA // PLAYERS
	// Set defaults for Turtle Arena animations
	// default weapon, put away
	if (index >= TORSO_PUTDEFAULT_BOTH && index <= TORSO_PUTDEFAULT_SECONDARY)
	{
		reversed = qtrue;
		anim[0] = TORSO_GETDEFAULT_BOTH+(index-TORSO_PUTDEFAULT_BOTH);
		anim[1] = TORSO_RAISE; // correct would be TORSO_DROP, but must reverse GETDEFAULT
		anim[2] = TORSO_STAND2;
		anim[3] = TORSO_STAND;
	}

	// default weapon, get out
	if (index >= TORSO_GETDEFAULT_BOTH && index <= TORSO_GETDEFAULT_SECONDARY)
	{
		reversed = qtrue;
		anim[0] = TORSO_PUTDEFAULT_BOTH+(index-TORSO_GETDEFAULT_BOTH);
		anim[1] = TORSO_DROP; // correct would be TORSO_RAISE, but must reverse PUTDEFAULT
		anim[2] = TORSO_STAND2;
		anim[3] = TORSO_STAND;
	}


	// standing defaults
	if (index == TORSO_STAND_GUN_PRIMARY)
	{
		anim[0] = TORSO_STAND2;
		anim[1] = TORSO_STAND;
	}

	if (index == TORSO_STAND_SWORD1_BOTH)
	{
		anim[0] = TORSO_STAND;
	}

	if (index == TORSO_STAND_SWORD1_PRIMARY)
	{
		anim[0] = TORSO_STAND_SAI1_PRIMARY;
		anim[1] = TORSO_STAND2;
		anim[2] = TORSO_STAND;
	}

	if (index == TORSO_STAND_SAI1_PRIMARY)
	{
		anim[0] = TORSO_STAND_SWORD1_PRIMARY;
		anim[1] = TORSO_STAND2;
		anim[2] = TORSO_STAND;
	}

	if (index == TORSO_STAND_SWORD2)
	{
		anim[0] = TORSO_STAND_SAI2;
		anim[1] = TORSO_STAND2;
		anim[2] = TORSO_STAND;
	}

	if (index == TORSO_STAND_DAISHO || index == TORSO_STAND_SAI2)
	{
		anim[0] = TORSO_STAND_SWORD2;
		anim[1] = TORSO_STAND2;
		anim[2] = TORSO_STAND;
	}

	if (index == TORSO_STAND_BO)
	{
		anim[0] = TORSO_STAND_SWORD1_BOTH;
		anim[1] = TORSO_STAND;
	}

	if (index == TORSO_STAND_BO_PRIMARY)
	{
		anim[0] = TORSO_STAND_SWORD1_PRIMARY;
		anim[1] = TORSO_STAND_SAI1_PRIMARY;
		anim[2] = TORSO_STAND2;
		anim[3] = TORSO_STAND;
	}

	// default to TORSO_STAND_SWORD1_*
	if (index == TORSO_STAND_HAMMER || index == TORSO_STAND_HAMMER_PRIMARY)
	{
		anim[0] = TORSO_STAND_SWORD1_BOTH+(index-TORSO_STAND_HAMMER);
		anim[1] = TORSO_STAND2;
		anim[2] = TORSO_STAND;
	}

	// default to TORSO_STAND_SAI*
	if (index == TORSO_STAND_NUNCHUCKS || index == TORSO_STAND_NUNCHUCKS1_PRIMARY)
	{
		anim[0] = TORSO_STAND_SAI2+(index-TORSO_STAND_NUNCHUCKS);
		anim[1] = TORSO_STAND2;
		anim[2] = TORSO_STAND;
	}


	// attacking defaults
	if (index == TORSO_ATTACK_HAMMER_A) {
		anim[0] = TORSO_ATTACK_SWORD1_BOTH_A;
		anim[1] = TORSO_ATTACK_HAMMER_PRIMARY_A;
		anim[2] = TORSO_ATTACK2;
		anim[3] = TORSO_ATTACK;
	}
	if (index == TORSO_ATTACK_HAMMER_PRIMARY_A) {
		anim[0] = TORSO_ATTACK_SWORD1_PRIMARY_A;
		anim[1] = TORSO_ATTACK2;
		anim[2] = TORSO_ATTACK;
	}

	if (index >= TORSO_ATTACK_BO_A && index <= TORSO_ATTACK_BO_C) {
		anim[0] = TORSO_ATTACK_SWORD1_BOTH_A + index - TORSO_ATTACK_BO_A;
		anim[1] = index+3;
		anim[2] = TORSO_ATTACK2;
		anim[3] = TORSO_ATTACK;
	}
	if (index >= TORSO_ATTACK_BO_PRIMARY_A && index <= TORSO_ATTACK_BO_PRIMARY_C) {
		anim[0] = TORSO_ATTACK_SWORD1_PRIMARY_A + index - TORSO_ATTACK_BO_PRIMARY_A;
		anim[1] = TORSO_ATTACK2;
		anim[2] = TORSO_ATTACK;
	}

	if (index >= TORSO_ATTACK_NUNCHUCKS_A && index <= TORSO_ATTACK_NUNCHUCKS_C) {
		anim[0] = TORSO_ATTACK_SAI2_A + index - TORSO_ATTACK_NUNCHUCKS_A;
		anim[1] = index+3;
		anim[2] = TORSO_ATTACK2;
		anim[3] = TORSO_ATTACK;
	}
	if (index >= TORSO_ATTACK_NUNCHUCKS1_PRIMARY_A && index <= TORSO_ATTACK_NUNCHUCKS1_PRIMARY_C) {
		anim[0] = TORSO_ATTACK_SAI1_PRIMARY_A + index - TORSO_ATTACK_NUNCHUCKS1_PRIMARY_A;
		anim[1] = TORSO_ATTACK2;
		anim[2] = TORSO_ATTACK;
	}

	// BOTH can default to PRIMARY.
	if (   (index >= TORSO_ATTACK_SWORD1_BOTH_A &&
			index <= TORSO_ATTACK_SWORD1_BOTH_C)
		|| (index >= TORSO_ATTACK_SAI2_A &&
			index <= TORSO_ATTACK_SAI2_C)
		// SWORD2 defaults to Daisho
		|| (index >= TORSO_ATTACK_SWORD2_A &&
			index <= TORSO_ATTACK_SWORD2_C)
		)
	{
		anim[0] = index+3;
		anim[1] = TORSO_ATTACK2;
		anim[2] = TORSO_ATTACK;
	}

	// PRIMARY can default to BOTH.
	if (   (index >= TORSO_ATTACK_SWORD1_PRIMARY_A &&
			index <= TORSO_ATTACK_SWORD1_PRIMARY_C)
		|| (index >= TORSO_ATTACK_SAI1_PRIMARY_A &&
			index <= TORSO_ATTACK_SAI1_PRIMARY_C)
		// SWORD2 defaults to SWORD1_PRIMARY
		|| (index >= TORSO_ATTACK_SWORD2_A &&
			index <= TORSO_ATTACK_SWORD2_C)
		// Daisho defaults to SWORD2
		|| (index >= TORSO_ATTACK_DAISHO_A &&
			index <= TORSO_ATTACK_DAISHO_C)
		)
	{
		anim[0] = index-3;
		anim[1] = TORSO_ATTACK2;
		anim[2] = TORSO_ATTACK;
	}
#endif

	for (i = 0; i < MAX_DEFAULT_ANIMATIONS; i++)
	{
		if (anim[i] == -1)
			break;

		// Check if the animation has been loaded
		if (loadedAnim[anim[i]] == qfalse) {
			continue;
		}

		memcpy(&animations[index], &animations[anim[i]], sizeof (animation_t));
		if (reversed) {
			animations[index].reversed = !animations[index].reversed;
		}
		//animations[i].flipflop = flipflop;

		// Animation is now valid
		loadedAnim[index] = qtrue;

		return qtrue;
	}

	//Com_Printf("BG_SetDefaultAnimation: Failed to set default animation for index %i.\n", index);
	return qfalse;
#undef MAX_DEFAULT_ANIMATIONS
}

/*
===============
BG_LoadAnimation

Returns qtrue if the animation loaded with out error.
===============
*/
int BG_LoadAnimation(char **text_p, int i, animation_t *animations, frameSkip_t *skip, int prefixType)
{
	char		*token;
	float		fps;

	animations[i].prefixType = prefixType;

	token = COM_Parse( text_p );
	if ( !*token ) {
		//Com_Printf("DEBUG: Animtion missing first frame.\n");
		return -1;
	}
	animations[i].firstFrame = atoi( token );

	if (skip != NULL)
	{
		// leg only frames must be adjusted to not count the upper body only frames
		if (prefixType == AP_LEGS) {
			if (skip->legSkip == -1) {
				skip->legSkip = animations[i].firstFrame - skip->firstTorsoFrame;
			}
			animations[i].firstFrame -= skip->legSkip;
		}
		else if (prefixType == AP_TORSO)
		{
			if (skip->firstTorsoFrame == -1) {
				skip->firstTorsoFrame = animations[i].firstFrame;
			}
		}
	}

	token = COM_Parse( text_p );
	if ( !*token ) {
		//Com_Printf("DEBUG: Animtion missing numFrame.\n");
		return 0;
	}
	animations[i].numFrames = atoi( token );

	animations[i].reversed = qfalse;
	animations[i].flipflop = qfalse;
	// if numFrames is negative the animation is reversed
	if (animations[i].numFrames < 0) {
		animations[i].numFrames = -animations[i].numFrames;
		animations[i].reversed = qtrue;
	}

	token = COM_Parse( text_p );
	if ( !*token ) {
		//Com_Printf("DEBUG: Animtion missing loopFrame.\n");
		return 0;
	}

	// ZTM: NOTE: After loading all frames we check if Elite Force style,
	//                     and convert it to Quake3 style loopFrames if needed.
	animations[i].loopFrames = atoi( token );

	token = COM_Parse( text_p );
	if ( !*token ) {
		//Com_Printf("DEBUG: Animtion missing fps.\n");
		return 0;
	}
	fps = atof( token );
	if ( fps == 0 ) {
		fps = 1;
	}
	animations[i].frameLerp = 1000 / fps;
	animations[i].initialLerp = 1000 / fps;

	return 1;
}

/*
===============
BG_ConvertEFAnimationsToQ3

Convert STV: Elite Force loopFrames to Quake3 loopFrames
===============
*/
static void BG_ConvertEFAnimationsToQ3(animation_t *animations, int numanims)
{
	int			i;
	qboolean	ef_style = qfalse;

	// Check if ef style
	for ( i = 0 ; i < numanims ; i++ )
	{
		if (animations[i].loopFrames == -1)
		{
			// No Quake3 players should have this, only Elite Force.
			ef_style = qtrue;
		}
		else if (animations[i].loopFrames == 0)
		{
			// Shared by q3 and ef.
		}
		else
		{
			// EF only uses -1 and 0.
			ef_style = qfalse;
			break;
		}
	}

	if (ef_style)
	{
		// Convert to q3 style.
		for ( i = 0 ; i < MAX_ANIMATIONS ; i++ )
		{
			if (animations[i].loopFrames == -1)
			{
				animations[i].loopFrames = 0;
				break;
			}
			else if (animations[i].loopFrames == 0)
			{
				animations[i].loopFrames = animations[i].numFrames;
			}
		}
	}
}

#ifdef TA_PLAYERSYS
/*
======================
BG_AnimPrefixTypeForAnimIndex

Only supports player animations, not NPCs or misc_objects
======================
*/
int BG_AnimPrefixTypeForAnimIndex(animNumber_t anim)
{
	// ZTM: NOTE: anim is unsigned, don't check for >= 0
	if (/*anim >= BOTH_DEATH1 && */anim <= BOTH_DEAD3)
		return AP_BOTH;
	else if (anim >= TORSO_GESTURE && anim <= TORSO_STAND2)
		return AP_TORSO;
	else if (anim >= LEGS_WALKCR && anim <= LEGS_TURN)
		return AP_LEGS;
	else if (anim >= TORSO_GETFLAG && anim <= TORSO_NEGATIVE)
		return AP_TORSO;

	return 0;
}

/*
======================
BG_AnimPrefixTypeForName

Returns the prefixType for the animation name.
======================
*/
int BG_AnimPrefixTypeForName(const char *name)
{
	if (Q_stricmpn(name, "TORSO_", 6) == 0)
		return AP_TORSO;
	else if (Q_stricmpn(name, "LEGS_", 5) == 0)
		return AP_LEGS;
	else if (Q_stricmpn(name, "BOTH_", 5) == 0)
		return AP_BOTH;
	else if (Q_stricmpn(name, "OBJECT_", 7) == 0)
		return AP_OBJECT;

	return 0;
}

/*
======================
BG_AnimName

Returns a pointer to the animations name, skipping the prefix (BOTH_, TORSO_, LEGS_, or ANIM_)
======================
*/
const char *BG_AnimName(const char *nameWithPrefix)
{
	char *r;

	r = strstr(nameWithPrefix, "_");
	if (r != NULL) {
		r++;
	}

	return r;
}
#endif

/*
======================
BG_ParsePlayerCFGFile

Based on Quake3's CG_ParseAnimationFile
  This was been moved to BG because its used in game, cgame, and ui,
  This way it is a lot easier to make changes to animation.cfg loading...

Read a configuration file containing animation and game config for the player.
models/players/[model name]/animation.cfg, models/players/heads/[head name]/animation.cfg

Supports Quake3 and Elite Force animation styles.

Quake3 style (also used by Elite Force MP): (TA_SUPPORTQ3 must be defined)
0	31	0	20		// BOTH_DEATH1
30	1	0	20		// BOTH_DEAD1
31	31	0	20		// BOTH_DEATH2
61	1	0	20		// BOTH_DEAD2
62	33	0	20		// BOTH_DEATH3
94	1	0	20		// BOTH_DEAD3

Elite Force (SP) style:
BOTH_DEATH1		0	31	0	20
BOTH_DEAD1		30	1	0	20
BOTH_DEATH2		31	31	0	20
BOTH_DEAD2		61	1	0	20
BOTH_DEATH3		62	33	0	20
BOTH_DEAD3		94	1	0	20

Both styles also support Elite Force (SP/MP) "loopFrames" (-1 = no loop, 0 = loop all frames.)
  NOTE: q3 style "loopFrames" (0 = no loop, # = loop num frames, normally equal to numframes.)

ZTM: NOTE: I changed it to parse animation name instead of having a defined order
			(and a comment with the name.) and then found out it was the same
			in Elite Force Single Player, so I added support for elite force "loopFrames"
======================
*/
qboolean BG_ParsePlayerCFGFile(const char *filename, bg_playercfg_t *playercfg, qboolean headConfig ) {
	char		*text_p, *prev;
	int			len;
	int			i;
	char		*token;
	frameSkip_t	skip;
	char		text[20000];
	fileHandle_t	f;
	animation_t *animations, dummyAnimation;
	qboolean foundAnim;
	qboolean loadedAnim[MAX_TOTALANIMATIONS];
	int rtn = 0;
	int prefixType;

	animations = playercfg->animations;
	foundAnim = qfalse;
	memset(loadedAnim, qfalse, MAX_TOTALANIMATIONS * sizeof (qboolean));

	// Defaults moved to BG_LoadPlayerCFGFile

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}
	if ( (unsigned)len >= sizeof( text ) - 1 ) {
		Com_Printf( "File %s too long\n", filename );
		trap_FS_FCloseFile( f );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;
	skip.legSkip = skip.firstTorsoFrame = -1;

	// read optional parameters
	while ( 1 ) {
		prev = text_p;	// so we can unget
		token = COM_Parse( &text_p );

		// ZTM: NOTE: Must check for (!*token) because (!token) is ALWAYS false.
		//       This is a bug in the Quake3 source.
		//       It doesn't cause a problem in the animation.cfg parse
		//          due to special checking/breaking for animations.
		if ( !*token ) {
			break;
		}

		// Skip animation keywords.
		if ( !Q_stricmp( token, "footsteps" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			if (headConfig) // skip footsteps
				continue;
			if ( !Q_stricmp( token, "default" ) || !Q_stricmp( token, "normal" ) ) {
				playercfg->footsteps = FOOTSTEP_NORMAL;
			} else if ( !Q_stricmp( token, "boot" ) ) {
				playercfg->footsteps = FOOTSTEP_BOOT;
			} else if ( !Q_stricmp( token, "flesh" ) ) {
				playercfg->footsteps = FOOTSTEP_FLESH;
			} else if ( !Q_stricmp( token, "mech" ) ) {
				playercfg->footsteps = FOOTSTEP_MECH;
			} else if ( !Q_stricmp( token, "energy" ) ) {
				playercfg->footsteps = FOOTSTEP_ENERGY;
			// ZTM: Hey why not, its valid. (Not in Q3)
			} else if ( !Q_stricmp( token, "splash" ) ) {
				playercfg->footsteps = FOOTSTEP_SPLASH;
			} else {
				Com_Printf( "Bad footsteps parm (using default) in %s: %s\n", filename, token );
				playercfg->footsteps = FOOTSTEP_NORMAL;
			}
			continue;
		} else if ( !Q_stricmp( token, "headoffset" ) ) {
			for ( i = 0 ; i < 3 ; i++ ) {
				token = COM_Parse( &text_p );
				if ( !*token ) {
					break;
				}
				playercfg->headOffset[i] = atof( token );
			}
			continue;
		} else if ( !Q_stricmp( token, "sex" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			if ( token[0] == 'f' || token[0] == 'F' ) {
				playercfg->gender = GENDER_FEMALE;
			} else if ( token[0] == 'n' || token[0] == 'N' ) {
				playercfg->gender = GENDER_NEUTER;
			} else {
				playercfg->gender = GENDER_MALE;
			}
			continue;
		} else if ( !Q_stricmp( token, "fixedlegs" ) ) {
			token = COM_Parse( &text_p );
#ifndef TA_SUPPORTQ3
			if ( !*token ) {
				break;
			}
#endif
			if (headConfig) // skip fixedlegs
				continue;
			if (!Q_stricmp( token, "false" ) || !Q_stricmp( token, "0" )) {
				playercfg->fixedlegs = qfalse;
			} else if (!Q_stricmp( token, "true" ) || !Q_stricmp( token, "1" )) {
				playercfg->fixedlegs = qtrue;
			} else {
#ifdef TA_SUPPORTQ3
				if ( *token ) {
					Com_Printf( "Bad fixedlegs parm in %s: %s\n", filename, token );
				}
				playercfg->fixedlegs = qtrue;
#else
				Com_Printf( "Bad fixedlegs parm in %s: %s\n", filename, token );
#endif
			}
			continue;
		} else if ( !Q_stricmp( token, "fixedtorso" ) ) {
			token = COM_Parse( &text_p );
#ifndef TA_SUPPORTQ3
			if ( !*token ) {
				break;
			}
#endif
			if (headConfig) // skip fixedtorso
				continue;
			if (!Q_stricmp( token, "false" ) || !Q_stricmp( token, "0" )) {
				playercfg->fixedtorso = qfalse;
			} else if (!Q_stricmp( token, "true" ) || !Q_stricmp( token, "1" )) {
				playercfg->fixedtorso = qtrue;
			} else {
#ifdef TA_SUPPORTQ3
				if ( *token ) {
					Com_Printf( "Bad fixedtorso parm in %s: %s\n", filename, token );
				}
				playercfg->fixedtorso = qtrue;
#else
				Com_Printf( "Bad fixedtorso parm in %s: %s\n", filename, token );
#endif
			}
			continue;
		}
#ifdef TA_WEAPSYS
		else if ( !Q_stricmp( token, "primaryHand" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			if (headConfig) // skip primaryHand
				continue;
			if ( !Q_stricmp( token, "left" ) ) {
				playercfg->handSide[HAND_PRIMARY] = HS_LEFT;
				playercfg->handSide[HAND_SECONDARY] = HS_RIGHT;
			} else if ( !Q_stricmp( token, "right" ) ) {
				playercfg->handSide[HAND_PRIMARY] = HS_RIGHT;
				playercfg->handSide[HAND_SECONDARY] = HS_LEFT;
			} else {
				Com_Printf( "Bad primaryHand parm in %s: %s\n", filename, token );
			}
			continue;
		}
#endif

		//
		// Support Elite Force soundpath keyword
		//
		else if ( !Q_stricmp( token, "soundpath" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			Com_sprintf(playercfg->soundpath, sizeof (playercfg->soundpath), "%s", token);
			continue;
		}

		//
		// New Turtle Arena stuff
		//

		// boundingbox -15 -15 -24 15 15 32
		else if ( !Q_stricmp( token, "boundingbox" ) ) {
			for ( i = 0 ; i < 3 ; i++ ) {
				token = COM_Parse( &text_p );
				if ( !*token ) {
					break;
				}
				if (!headConfig) { // skip boundingbox
					playercfg->bbmins[i] = atof( token );
				}
			}
			if (i == 3) // found all tokens
			{
				for ( i = 0 ; i < 3 ; i++ ) {
					token = COM_Parse( &text_p );
					if ( !*token ) {
						break;
					}
					if (!headConfig) { // skip boundingbox
						playercfg->bbmaxs[i] = atof( token );
					}
				}
			}
			continue;
		}
		else if ( !Q_stricmp( token, "deadmax" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			if (headConfig) // skip deadmax
				continue;
			playercfg->deadmax = atoi( token );
			continue;
		}

		// Jump velocity multiplier
		else if ( !Q_stricmp( token, "jumpMult" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			if (headConfig) // skip jumpMult
				continue;
			playercfg->jumpMult = atof( token );
			continue;
		}
		// prefcolor1/prefcolor2 are used as color1/color2 in single player
		else if ( !Q_stricmp( token, "prefcolor1" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			playercfg->prefcolor1 = atoi( token );
			continue;
		}
		else if ( !Q_stricmp( token, "prefcolor2" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			playercfg->prefcolor2 = atoi( token );
			continue;
		}
		else if ( !Q_stricmp( token, "ability" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			if (headConfig) // skip ability
				continue;
			if ( !Q_stricmp( token, "none" ) ) {
				playercfg->ability = ABILITY_NONE;
			} else if ( !Q_stricmp( token, "tech" ) ) {
				playercfg->ability = ABILITY_TECH;
			} else if ( !Q_stricmp( token, "strength" ) ) {
				playercfg->ability = ABILITY_STRENGTH;
			} else {
				Com_Printf( "Bad ability parm in %s: %s\n", filename, token );
			}
			continue;
		}
		// "speed" is the max speed that the
		// player runs without powerups.
		// Based on SRB2's "normalspeed"
		else if ( !Q_stricmp( token, "speed" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			if (headConfig) // skip speed
				continue;
			playercfg->max_speed = atoi( token );
			continue;
		}
		// Speed to start run animtion.
		//else if ( !Q_stricmp( token, "runspeed" ) ) {
		//}
		// Accel when move starts.
		else if ( !Q_stricmp( token, "accelstart" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			if (headConfig) // skip accelstart
				continue;
			playercfg->accelstart = atof( token );
			continue;
		}
		else if ( !Q_stricmp( token, "acceleration" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			if (headConfig) // skip acceleration
				continue;
			playercfg->accelerate_speed = atof( token );
			continue;
		}

		//else if ( !Q_stricmp( token, "thrustfactor" ) ) {
		//}

#ifdef TA_WEAPSYS // ZTM: DEFAULT_WEAPON
		else if ( !Q_stricmp( token, "default_weapon" ) ) {
			int j;
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			if (headConfig) // skip default_weapon
				continue;
			j = BG_WeaponGroupIndexForName(token);
			if (j) {
				playercfg->default_weapon = j;
			} else {
				Com_Printf( "Bad default_weapon parm in %s: %s\n", filename, token );
			}
			continue;
		}
#endif

		if ((prefixType = BG_AnimPrefixTypeForName(token)))
		{
			qboolean animName = qfalse;
			const char *name;

			// Load animations by name.
			for (i = 0; playerAnimationDefs[i].name != NULL; i++)
			{
				if ( !Q_stricmp( token, playerAnimationDefs[i].name ) ) {
					animName = foundAnim = qtrue;
					if (headConfig) {
						// head config skips body animations
						rtn = BG_LoadAnimation(&text_p, 0, &dummyAnimation, &skip, prefixType);
					} else {
						rtn = BG_LoadAnimation(&text_p, playerAnimationDefs[i].num, animations, &skip, prefixType);
						if (rtn == -1) {
							BG_SetDefaultAnimation(loadedAnim, playerAnimationDefs[i].num, animations);
						} else if (rtn == 0) {
							Com_Printf("BG_ParsePlayerCFGFile: Anim %s: Failed loading.\n", token);
						} else {
							loadedAnim[playerAnimationDefs[i].num] = qtrue;
						}
					}
					break;
				}
			}

			if (animName) {
				continue;
			}

			// Check undefined animations (some are actually supported...)
			name = BG_AnimName(token);

			for (i = 0; playerAnimationDefs[i].name != NULL; i++)
			{
				// if the token and animation i have the same base name,
				//   but with different prefixes, load it but use token's prefix...
				if (!Q_stricmp( name, BG_AnimName(playerAnimationDefs[i].name) ) )
				{
					animName = foundAnim = qtrue;
					if (headConfig) {
						// head config skips body animations
						rtn = BG_LoadAnimation(&text_p, 0, &dummyAnimation, &skip, prefixType);
					} else {
						rtn = BG_LoadAnimation(&text_p, playerAnimationDefs[i].num, animations, &skip, prefixType);
						if (rtn == -1) {
							BG_SetDefaultAnimation(loadedAnim, playerAnimationDefs[i].num, animations);
						} else if (rtn == 0) {
							Com_Printf("BG_ParsePlayerCFGFile: Anim %s: Failed loading.\n", token);
						} else {
							loadedAnim[playerAnimationDefs[i].num] = qtrue;
						}
					}
					break;
				}
			}

			if (animName) {
				continue;
			}

			Com_Printf( "unknown animation '%s' in %s\n", token, filename );
			continue;
		}

#if !defined TURTLEARENA || defined TA_SUPPORTQ3 // animation.cfg
		// if it is a number, start parsing animations
		if ( token[0] >= '0' && token[0] <= '9' ) {
			text_p = prev;	// unget the token
			break;
		}
#endif

		Com_Printf( "unknown token '%s' in %s\n", token, filename );
	}

	if (headConfig) // skip animations
	{
		return qtrue;
	}

	// Didn't find an Elite Force (SP) style animation.
	if (!foundAnim)
	{
#if !defined TURTLEARENA || defined TA_SUPPORTQ3 // animation.cfg
		// Assume Quake3 (or Elite Force MP) player.
		// read information for each frame
		for ( i = 0 ; i < MAX_ANIMATIONS ; i++ )
		{
			rtn = BG_LoadAnimation(&text_p, i, animations, &skip, BG_AnimPrefixTypeForAnimIndex(i));
			if (rtn == -1) {
				BG_SetDefaultAnimation(loadedAnim, i, animations);
			} else if (rtn == 0) {
				Com_Printf("BG_ParsePlayerCFGFile: Animation %d failed loading.\n", i);
				break;
			} else {
				loadedAnim[i] = qtrue;
			}
		}

		if (i != MAX_ANIMATIONS)
		{
			Com_Printf( "Error parsing animation file: %s", filename );
			return qfalse;
		}
#else
		// Didn't find any animations.
		Com_Printf( "Error: No animations in file: %s", filename );
		return qfalse;
#endif
	}

	// Check for missing animations and load there defaults.
	for ( i = 0 ; i < MAX_TOTALANIMATIONS ; i++ )
	{
		if (loadedAnim[i] == qfalse)
		{
			// Load the default for this animation.
			BG_SetDefaultAnimation(loadedAnim, i, animations);
		}
	}

	BG_ConvertEFAnimationsToQ3(animations, MAX_TOTALANIMATIONS);

	return qtrue;
}

/*
===============
BG_LoadPlayerCFGFile

Load animation.cfg for model into playercfg
===============
*/
qboolean BG_LoadPlayerCFGFile(bg_playercfg_t *playercfg, const char *model, const char *headModel)
{
	char filename[MAX_QPATH];
	int i;

	if (!model)
	{
		model = DEFAULT_MODEL;
	}

	if (!headModel)
	{
		headModel = model;
	}

	Q_strncpyz(playercfg->model, model, MAX_QPATH);

	// Setup defaults
	playercfg->footsteps = FOOTSTEP_NORMAL;
	VectorClear( playercfg->headOffset );
	playercfg->gender = GENDER_MALE;
	playercfg->fixedlegs = qfalse;
	playercfg->fixedtorso = qfalse;
	playercfg->prefcolor1 = 2; // green
	playercfg->prefcolor2 = 9; // lime

	// Use the model name for the default soundpath.
	Q_strncpyz(playercfg->soundpath, model, sizeof (playercfg->soundpath));

#ifdef TA_WEAPSYS
	playercfg->handSide[HAND_PRIMARY] = HS_RIGHT;
	playercfg->handSide[HAND_SECONDARY] = HS_LEFT;
	playercfg->default_weapon = BG_WeaponGroupIndexForName(DEFAULT_DEFAULT_WEAPON);

	// If the default weapon wasn't found.
	if (!playercfg->default_weapon)
	{
		int max = BG_NumWeaponGroups();

		// Look for a melee weapon, like Q3 gauntlet.
		for (i = 1; i < max; i++)
		{
			if (BG_WeaponHasType(i, WT_GUN))
			{
				continue;
			}
			playercfg->default_weapon = i;
			break;
		}
		if (i == max && max > 1)
		{
			// No melee weapon so fall back to first weapon.
			playercfg->default_weapon = 1;
		}
	}
#endif

	// Default to Q3 bounding box (If changed update game's SpotWouldTelefrag)
	VectorSet(playercfg->bbmins,-15, -15, -24); // playerMins
	VectorSet(playercfg->bbmaxs, 15,  15,  32); // playerMaxs
	playercfg->deadmax = -8;

	playercfg->max_speed = 320;
	playercfg->accelerate_speed = 10.0f;
	playercfg->accelstart = 50.0f;

	playercfg->jumpMult = 1.0f;

	playercfg->ability = ABILITY_NONE;

	// Use first frame for all animations.
	for (i = 0; i < MAX_TOTALANIMATIONS; i++)
	{
		playercfg->animations[i].firstFrame = 0;
		playercfg->animations[i].numFrames = 1;
		playercfg->animations[i].loopFrames = 0;
		playercfg->animations[i].frameLerp = 100; // 10 fps
		playercfg->animations[i].initialLerp = 100; // 10 fps
		playercfg->animations[i].reversed = 0;
		playercfg->animations[i].flipflop = 0;
	}

#ifndef IOQ3ZTM // FLAG_ANIMATIONS
	//
	// Setup flag animations. For the flag not player.
	//
	// flag moving fast
	animations[FLAG_RUN].firstFrame = 0;
	animations[FLAG_RUN].numFrames = 16;
	animations[FLAG_RUN].loopFrames = 16;
	animations[FLAG_RUN].frameLerp = 1000 / 15;
	animations[FLAG_RUN].initialLerp = 1000 / 15;
	animations[FLAG_RUN].reversed = qfalse;
	// flag not moving or moving slowly
	animations[FLAG_STAND].firstFrame = 16;
	animations[FLAG_STAND].numFrames = 5;
	animations[FLAG_STAND].loopFrames = 0;
	animations[FLAG_STAND].frameLerp = 1000 / 20;
	animations[FLAG_STAND].initialLerp = 1000 / 20;
	animations[FLAG_STAND].reversed = qfalse;
	// flag speeding up
	animations[FLAG_STAND2RUN].firstFrame = 16;
	animations[FLAG_STAND2RUN].numFrames = 5;
	animations[FLAG_STAND2RUN].loopFrames = 1;
	animations[FLAG_STAND2RUN].frameLerp = 1000 / 15;
	animations[FLAG_STAND2RUN].initialLerp = 1000 / 15;
	animations[FLAG_STAND2RUN].reversed = qtrue;
#endif

	// load animation.cfg
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/animation.cfg", model );
	if ( !BG_ParsePlayerCFGFile(filename, playercfg, qfalse) ) {
		Com_Printf( "Failed to load animation.cfg for %s\n", model );
		return qfalse;
	}

	// If using a different head model, load the correct head settings.
	if (Q_stricmp(model, headModel) != 0)
	{
		qboolean foundConfig;

		if (headModel[0] == '*') {
			Com_sprintf( filename, sizeof( filename ), "models/players/heads/%s/animation.cfg", headModel );
			foundConfig = BG_ParsePlayerCFGFile(filename, playercfg, qtrue);
		} else {
			foundConfig = qfalse;
		}

		if (!foundConfig)
		{
			char oldSoundPath[MAX_QPATH];
			const char *head;

			Q_strncpyz(oldSoundPath, playercfg->soundpath, sizeof (playercfg->soundpath));
			Q_strncpyz(playercfg->soundpath, "", sizeof (playercfg->soundpath));

			if (headModel[0] == '*')
				head = &headModel[1];
			else
				head = headModel;

			Com_sprintf( filename, sizeof( filename ), "models/players/%s/animation.cfg", head );
			foundConfig = BG_ParsePlayerCFGFile(filename, playercfg, qtrue);

			// If found animation.cfg and didn't set soundpath
			if (foundConfig && !strlen(playercfg->soundpath))
			{
				// Use the implied soundpath
				Q_strncpyz(playercfg->soundpath, head, sizeof (playercfg->soundpath));
			}
			else if (!foundConfig)
			{
				// Restore soundpath
				Q_strncpyz(playercfg->soundpath, oldSoundPath, sizeof (playercfg->soundpath));
			}

			if (!foundConfig && headModel[0] != '*')
			{
				Com_sprintf( filename, sizeof( filename ), "models/players/heads/%s/animation.cfg", headModel );
				foundConfig = BG_ParsePlayerCFGFile(filename, playercfg, qtrue);
			}
		}
	}

	return qtrue;
}
#endif

#ifdef TA_ENTSYS // MISC_OBJECT
#define MAX_LEVEL_OBJECT_TYPES 128 // currently there is like 10, so 128 per level should be more than enough
bg_objectcfg_t	bg_objectcfgs[MAX_LEVEL_OBJECT_TYPES];
static int		numObjectConfigs;

bg_objectcfg_t *BG_NewObjectCfg(void) {
	bg_objectcfg_t *objectcfg;
	animation_t *animations;
	int i;

	objectcfg = &bg_objectcfgs[numObjectConfigs];
	numObjectConfigs++;

	// Default boundingbox
	objectcfg->bbmins[0] = objectcfg->bbmins[1] = -5.0f;
	objectcfg->bbmins[2] = 0.0f;

	objectcfg->bbmaxs[0] = objectcfg->bbmaxs[1] = 5.0f;
	objectcfg->bbmaxs[2] = 10.0f;

	objectcfg->health = 50;
	objectcfg->wait = 25; // Wait 25 seconds before respawning
	objectcfg->speed = 1.0f;
	objectcfg->knockback = qfalse;
	objectcfg->pushable = qfalse;
	objectcfg->heavy = qfalse;

	objectcfg->unsolidOnDeath = qtrue;
	objectcfg->invisibleUnsolidDeath = qfalse;
	objectcfg->lerpframes = qfalse;
	objectcfg->scale = 1.0f;

	objectcfg->explosionDamage = 0;
	objectcfg->explosionRadius = 300;
	objectcfg->deathDelay = 0;

	objectcfg->skin[0] = '\0';

	animations = objectcfg->animations;

	// Use first frame for all animations.
	for (i = 0; i < MAX_MISC_OBJECT_ANIMATIONS; i++)
	{
		animations[i].firstFrame = 0;
		animations[i].numFrames = 1;
		animations[i].loopFrames = 0;
		animations[i].frameLerp = 100; // 10 fps
		animations[i].initialLerp = 100; // 10 fps
		animations[i].reversed = 0;
		animations[i].flipflop = 0;
	}

	return objectcfg;
}

void BG_InitObjectConfig(void)
{
	Com_Memset(&bg_objectcfgs, 0, sizeof (bg_objectcfgs));
	numObjectConfigs = 0;
	BG_NewObjectCfg(); // Reserve bg_objectcfgs[0]
}

bg_objectcfg_t *BG_DefaultObjectCFG(void)
{
	return &bg_objectcfgs[0];
}

strAnimationDef_t objectAnimationDefs[] = {
	ANIMDEF(OBJECT_IDLE),
	ANIMDEF(OBJECT_DEATH1),
	ANIMDEF(OBJECT_DEATH2),
	ANIMDEF(OBJECT_DEATH3),
	ANIMDEF(OBJECT_DEAD1),
	ANIMDEF(OBJECT_DEAD2),
	ANIMDEF(OBJECT_DEAD3),
	ANIMDEF(OBJECT_LAND),
	ANIMDEF(OBJECT_PAIN),

	// End of List
	{ 0, NULL }
};

#ifndef GAME
sfxHandle_t	trap_S_RegisterSound( const char *sample, qboolean compressed );
#endif

/* Load sounds example:
{
	OBJECT_IDLE 3 sounds/object/idle%d.wav 1 3 30
	OBJECT_PAIN 0 sounds/object/pain.wav 0 0 0
}
*/
static qboolean Sounds_Parse(char **p, const char *name, bg_sounds_t *sounds, prefixType_e prefixType, strAnimationDef_t *animDefs) {
	bg_sounddef_t *sounddef;
	char *token;
	int i;
#ifndef GAME
	int j;
#endif

	token = COM_ParseExt(p, qtrue);
	if (token[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {
		token = COM_ParseExt(p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		for (i = 0; animDefs[i].name != NULL; i++) {
			if (Q_stricmp(token, animDefs[i].name) == 0) {
				// Found anim, load sound info
				sounddef = &sounds->sounddefs[sounds->numSounds];
				sounddef->prefixType = prefixType;
				sounddef->anim = animDefs[i].num;

				token = COM_Parse(p);
				if ( !token || token[0] == 0 ) {
					i = -1;
					break;
				}
				sounddef->frame = atoi(token);

				token = COM_Parse(p);
				if ( !token || token[0] == 0 ) {
					i = -1;
					break;
				}
				Q_strncpyz(sounddef->name, token, MAX_QPATH);

				token = COM_Parse(p);
				if ( !token || token[0] == 0 ) {
					i = -1;
					break;
				}
				sounddef->start = atoi(token);

				token = COM_Parse(p);
				if ( !token || token[0] == 0 ) {
					i = -1;
					break;
				}
				sounddef->end = atoi(token);

				token = COM_Parse(p);
				if ( !token || token[0] == 0 ) {
					i = -1;
					break;
				}
				sounddef->chance = atoi(token);

				if (sounddef->end - sounddef->start > MAX_RAND_SOUNDS)
				{
					Com_Printf("Warning: Too many random sound indexes in sounddef, only using first %d\n", MAX_RAND_SOUNDS);
					sounddef->end = sounddef->start + MAX_RAND_SOUNDS;
				}

#ifndef GAME
				// Load sounds
				for (j = sounddef->start; j < sounddef->end; j++) {
					sounddef->sounds[j-sounddef->start] = trap_S_RegisterSound(va(token, j), qfalse);
				}
#endif

				// Check if there is a open slot
				if (sounds->numSounds == MAX_BG_SOUNDS) {
					Com_Printf("Warning out of free slots for new sounddef, over writing last. (In %s)\n", name);
				} else {
					sounds->numSounds++;
				}
				break;
			}
		}
		if (i == -1) {
			Com_Printf( "skipping malformed sounddef line in sounds (In %s)\n", name );
			continue;
		}
		// Found anim
		if (animDefs[i].name != NULL) {
			continue;
		}

		Com_Printf( "unknown token '%s' in sounds %s\n", token, name );
	}
	Com_Printf( "sounds block not closed in %s\n", name );
	return qfalse;
}

/*
// example file for misc_object
boundingbox -16 -16 0 16 16 32
health 15
wait 0
speed 1
lerpframes 1
OBJECT_IDLE 0 10 10 10
OBJECT_DEATH1 20 10 0 10
OBJECT_DEATH2 30 10 0 10
OBJECT_DEATH3 40 10 0 10
OBJECT_DEAD1 50 10 0 10
*/

/*
===============
BG_ParseObjectCFGFile
===============
*/
bg_objectcfg_t *BG_ParseObjectCFGFile(const char *filename)
{
	char		*text_p;
	int			len;
	int			i;
	char		*token;
	char		text[20000];
	fileHandle_t	f;
	bg_objectcfg_t	*objectcfg;
	animation_t	*animations;

	if (filename[0] == '\0') {
		Com_Printf("DEBUG: Missing config filename for misc_object!\n");
		return NULL;
	}

	// Check file was already loaded
	for (i = 1; i < numObjectConfigs; i++) {
		if (Q_stricmp(bg_objectcfgs[i].filename, filename) == 0) {
			//Com_Printf("DEBUG: Reusing objectcfg %s\n", bg_objectcfgs[i].filename);
			return &bg_objectcfgs[i];
		}
	}

	if (numObjectConfigs >= MAX_LEVEL_OBJECT_TYPES) {
		Com_Printf("Warning: BG_ParseObjectCFGFile reached MAX_LEVEL_OBJECT_TYPES (%d)\n", MAX_LEVEL_OBJECT_TYPES);
		return NULL;
	}

	objectcfg = BG_NewObjectCfg();
	Q_strncpyz(objectcfg->filename, filename, MAX_QPATH);
	animations = objectcfg->animations;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return NULL;
	}
	if ( (unsigned)len >= sizeof( text ) - 1 ) {
		Com_Printf( "File %s too long\n", filename );
		trap_FS_FCloseFile( f );
		return NULL;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;

	// read optional parameters
	while ( 1 ) {
		token = COM_Parse( &text_p );
		if ( !*token ) {
			break;
		}
		// boundingbox -15 -15 -24 15 15 32
		else if ( !Q_stricmp( token, "boundingbox" ) ) {
			for ( i = 0 ; i < 3 ; i++ ) {
				token = COM_Parse( &text_p );
				if ( !*token ) {
					break;
				}
				objectcfg->bbmins[i] = atof( token );
			}
			if (i == 3) // found all tokens
			{
				for ( i = 0 ; i < 3 ; i++ ) {
					token = COM_Parse( &text_p );
					if ( !*token ) {
						break;
					}
					objectcfg->bbmaxs[i] = atof( token );
				}
			}
			continue;
		}
		// spawnhealth
		else if ( !Q_stricmp( token, "health" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			objectcfg->health = atoi(token);
			continue;
		}
		else if ( !Q_stricmp( token, "wait" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			objectcfg->wait = atoi(token);
			continue;
		}
		else if ( !Q_stricmp( token, "speed" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			objectcfg->speed = atof(token);
			continue;
		}
		else if ( !Q_stricmp( token, "knockback" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			objectcfg->knockback = atoi(token);
			continue;
		}
		else if ( !Q_stricmp( token, "pushable" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			objectcfg->pushable = atoi(token);
			continue;
		}
		else if ( !Q_stricmp( token, "heavy" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			objectcfg->heavy = atoi(token);
			continue;
		}
		else if ( !Q_stricmp( token, "unsolidOnDeath" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			objectcfg->unsolidOnDeath = atoi(token);
			continue;
		}
		else if ( !Q_stricmp( token, "invisibleUnsolidDeath" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			objectcfg->invisibleUnsolidDeath = atoi(token);
			continue;
		}
		else if ( !Q_stricmp( token, "lerpframes" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			objectcfg->lerpframes = atoi(token);
			continue;
		}
		else if ( !Q_stricmp( token, "scale" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			objectcfg->scale = atof(token);
			continue;
		}
		else if ( Q_stricmp( token, "sounds" ) == 0 ) {
			if (Sounds_Parse(&text_p, filename, &objectcfg->sounds, AP_OBJECT, objectAnimationDefs)) {
				continue;
			}
		}
		else if ( Q_stricmp( token, "explosionDamage" ) == 0 ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			objectcfg->explosionDamage = atoi(token);
			continue;
		}
		else if ( Q_stricmp( token, "explosionRadius" ) == 0 ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			objectcfg->explosionRadius = atof(token);
			continue;
		}
		else if ( Q_stricmp( token, "deathDelay" ) == 0 ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			objectcfg->deathDelay = atoi(token);
			continue;
		}
		else if ( Q_stricmp( token, "skin" ) == 0 ) {
			token = COM_Parse( &text_p );
			Q_strncpyz(objectcfg->skin, token, MAX_QPATH);
			continue;
		}
		else
		{
			qboolean animName = qfalse;

			// Load animations by name.
			for (i = 0; objectAnimationDefs[i].name != NULL; i++)
			{
				if ( !Q_stricmp( token, objectAnimationDefs[i].name ) )
				{
					animName = qtrue;
					if (BG_LoadAnimation(&text_p, objectAnimationDefs[i].num, animations, NULL, AP_OBJECT) != 1) {
						Com_Printf("BG_ParseObjectCFGFile: Anim %s: Failed loading.\n", objectAnimationDefs[i].name);
					}
					break;
				}
			}
			if (animName)
				continue;
		}

		Com_Printf( "unknown token '%s' in %s\n", token, filename );
	}

	return objectcfg;
}
#endif

