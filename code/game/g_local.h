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
// g_local.h -- local definitions for game module

#include "../qcommon/q_shared.h"
#include "bg_misc.h"
#include "g_public.h"
#include "ai_ea.h"

//==================================================================

// the "gameversion" client command will print this plus compile date
#define	GAMEVERSION	MODDIR

#define BODY_QUEUE_SIZE		8

#define INFINITE			1000000

#define	FRAMETIME			100					// msec
#define	CARNAGE_REWARD_TIME	3000
#define REWARD_SPRITE_TIME	2000

#ifdef TURTLEARENA // PLAYERS
#define TIME_BEFORE_WAITING_ANIMATION 7000
#endif

#define	INTERMISSION_DELAY_TIME	1000
#define	SP_INTERMISSION_DELAY_TIME	5000

// gentity->flags
#ifdef TURTLEARENA // DROWNING
#define	FL_DROWNING_WARNING		0x00000001
#endif
#ifdef IOQ3ZTM
#define	FL_FIRST_TIME			0x00000002
#endif
#define	FL_GODMODE				0x00000010
#define	FL_NOTARGET				0x00000020
#define	FL_TEAMSLAVE			0x00000400	// not the first on the team
#define FL_NO_KNOCKBACK			0x00000800
#define FL_DROPPED_ITEM			0x00001000
#define FL_NO_BOTS				0x00002000	// spawn point not for bot use
#define FL_NO_HUMANS			0x00004000	// spawn point just for bots
#define FL_FORCE_GESTURE		0x00008000	// force gesture on client
#ifdef TA_ENTSYS // MISC_OBJECT
#define FL_PUSHABLE				0x00010000	// make misc_object pushable
#define FL_HEAVY				0x00020000  // only pushable by characters with ABILITY_STRENGTH
#endif
#ifdef TA_WEAPSYS
#define FL_MISSILE_NO_DAMAGE_PARENT	0x00040000
#endif

// movers are things like doors, plats, buttons, etc
typedef enum {
	MOVER_POS1,
	MOVER_POS2,
	MOVER_1TO2,
	MOVER_2TO1
} moverState_t;

#ifndef TA_SP
#define SP_PODIUM_MODEL		"models/mapobjects/podium/podium4.md3"
#endif

//============================================================================

typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;

struct gentity_s {
	entityShared_t	r;				// shared by both the server system and game
	entityState_t	s;				// communicated by server to clients

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	struct gclient_s	*client;			// NULL if not a client

	qboolean	inuse;

	char		*classname;			// set in QuakeEd
	int			spawnflags;			// set in QuakeEd

	qboolean	neverFree;			// if true, FreeEntity will only unlink
									// bodyque uses this

	int			flags;				// FL_* variables

	char		*model;
	char		*model2;

#ifdef TA_WEAPSYS // XREAL
	int			spawnTime;			// level.time when the object was spawned
#endif
	int			freetime;			// level.time when the object was freed
	
	int			eventTime;			// events will be cleared EVENT_VALID_MSEC after set
	qboolean	freeAfterEvent;
	qboolean	unlinkAfterEvent;

	qboolean	physicsObject;		// if true, it can be pushed by movers and fall off edges
									// all game items are physicsObjects, 
	float		physicsBounce;		// 1.0 = continuous bounce, 0.0 = no bounce
	int			clipmask;			// brushes with this content value will be collided against
									// when moving.  items and corpses do not collide against
									// players, for instance

	// movers
	moverState_t moverState;
	int			soundPos1;
	int			sound1to2;
	int			sound2to1;
	int			soundPos2;
	int			soundLoop;
	gentity_t	*parent;
	gentity_t	*nextTrain;
	gentity_t	*prevTrain;
#ifdef TA_PATHSYS
	int			pathflags;
#endif
	vec3_t		pos1, pos2;

	char		*message;

	int			timestamp;		// body queue sinking, etc

	char		*target;
#ifdef TA_ENTSYS
	char		*paintarget;
#endif
	char		*targetname;
	char		*team;
	char		*targetShaderName;
	char		*targetShaderNewName;
	gentity_t	*target_ent;

	float		speed;
	vec3_t		movedir;

	int			nextthink;
	void		(*think)(gentity_t *self);
	void		(*reached)(gentity_t *self);	// movers call this when hitting endpoint
	void		(*blocked)(gentity_t *self, gentity_t *other);
	void		(*touch)(gentity_t *self, gentity_t *other, trace_t *trace);
	void		(*use)(gentity_t *self, gentity_t *other, gentity_t *activator);
	void		(*pain)(gentity_t *self, gentity_t *attacker, int damage);
	void		(*die)(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);
	qboolean	(*snapshotCallback)(gentity_t *self, gentity_t *client);

	int			pain_debounce_time;
	int			fly_sound_debounce_time;	// wind tunnel
	int			last_move_time;

	int			health;

	qboolean	takedamage;

	int			damage;
	int			splashDamage;	// quad will increase this without increasing radius
	int			splashRadius;
	int			methodOfDeath;
	int			splashMethodOfDeath;

	int			count;

	gentity_t	*chain;
	gentity_t	*enemy;
	gentity_t	*activator;
	gentity_t	*teamchain;		// next entity in team
	gentity_t	*teammaster;	// master of the team

#if defined MISSIONPACK && !defined TURTLEARENA // NO_KAMIKAZE_ITEM
	int			kamikazeTime;
	int			kamikazeShockTime;
#endif

#ifdef TA_MISC // MATERIALS
	int			deathMaterial; // death material
	int			damageMaterial; // damage material
#endif

	int			watertype;
	int			waterlevel;

	int			noise_index;

	// timing variables
	float		wait;
	float		random;

	bg_iteminfo_t	*item;			// for bonus items

#ifdef TA_ENTSYS // MISC_OBJECT
	bg_objectcfg_t	*objectcfg;
#endif
#ifdef TA_NPCSYS
	bg_npc_t	bgNPC;
#endif
#ifdef TA_WEAPSYS
	int			mustcut; // Only takes damage from WIF_CUTS weapons
#endif

	// dlights
	vec3_t		dl_color;
	char		*dl_stylestring;
	char		*dl_shader;
	int			dl_atten;

	// info for bots
	qboolean	botvalid;
	float		update_time;
	float		ltime;
	vec3_t		lastvisorigin;
	vec3_t		lastAngles;
	vec3_t		lastMins;
	vec3_t		lastMaxs;
	int			areanum;
};


typedef enum {
	CON_DISCONNECTED,
	CON_CONNECTING,
	CON_CONNECTED
} clientConnected_t;

typedef enum {
	SPECTATOR_NOT,
	SPECTATOR_FREE,
	SPECTATOR_FOLLOW,
	SPECTATOR_SCOREBOARD
} spectatorState_t;

typedef enum {
	TEAM_BEGIN,		// Beginning a team game, spawn at base
	TEAM_ACTIVE		// Now actively playing
} playerTeamStateState_t;

typedef struct {
	playerTeamStateState_t	state;

	int			location;

	int			captures;
	int			basedefense;
	int			carrierdefense;
	int			flagrecovery;
	int			fragcarrier;
	int			assists;

	float		lasthurtcarrier;
	float		lastreturnedflag;
	float		flagsince;
	float		lastfraggedcarrier;
} playerTeamState_t;

// client data that stays across multiple levels or tournament restarts
// this is achieved by writing all the data to cvar strings at game shutdown
// time and reading them back at connection time.  Anything added here
// MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
typedef struct {
	team_t		sessionTeam;
	int			spectatorNum;		// for determining next-in-line to play
	spectatorState_t	spectatorState;
	int			spectatorClient;	// for chasecam and follow mode
	int			wins, losses;		// tournament stats
	qboolean	teamLeader;			// true when this client is a team leader
} clientSession_t;

//
#define MAX_NETNAME			36
#define	MAX_VOTE_COUNT		3

// client data that stays across multiple respawns, but is cleared
// on each level change or team change at ClientBegin()
typedef struct {
	int			connectionNum;		// index in level.connections
	int			localPlayerNum;		// client's local player number in range of 0 to MAX_SPLITVIEW-1
	clientConnected_t	connected;	
	usercmd_t	cmd;				// we would lose angles if not persistant
	qboolean	localClient;		// true if "ip" info key is "localhost"
	qboolean	initialSpawn;		// the first spawn should be at a cool location
	qboolean	predictItemPickup;	// based on cg_predictItems userinfo
	qboolean	pmoveFixed;			//
	char		netname[MAX_NETNAME];
	int			maxHealth;			// for handicapping
	int			enterTime;			// level.time the client entered the game
	playerTeamState_t teamState;	// status in teamplay games
	int			voteCount;			// to prevent people from constantly calling votes
	int			teamVoteCount;		// to prevent people from constantly calling votes
	qboolean	teamInfo;			// send team overlay updates?
#ifdef TA_PLAYERSYS
    bg_playercfg_t playercfg;        // data loaded from animation.cfg
#endif
#ifdef TA_GAME_MODELS
	// loaded using trap_R_RegisterModel, used in trap_R_LerpTag
	qhandle_t	torsoModel;
	qhandle_t	legsModel;

	// Used with ps.torsoAnim/legsAnim to find tag locations
	lerpFrame_t torso, legs;

	// Player axis, setup using G_PlayerAngles
	vec3_t legsAxis[3], torsoAxis[3], headAxis[3];
#endif
} clientPersistant_t;


// this structure is cleared on each ClientSpawn(),
// except for 'client->pers' and 'client->sess'
struct gclient_s {
	// ps MUST be the first element, because the server expects it
	playerState_t	ps;				// communicated by server to clients

#ifdef IOQ3ZTM // PEAKING
	playerState_t savedPS; // Saved ps when following other clients when not a spectator
#endif

	// the rest of the structure is private to game
	clientPersistant_t	pers;
	clientSession_t		sess;

	qboolean	readyToExit;		// wishes to leave the intermission

	qboolean	noclip;

	int			lastCmdTime;		// level.time of last usercmd_t, for EF_CONNECTION
									// we can't just use pers.lastCommand.time, because
									// of the g_sycronousclients case
	int			buttons;
	int			oldbuttons;
	int			latched_buttons;

	vec3_t		oldOrigin;
#if 0 //#ifdef TA_ENTSYS // PUSHABLE
	float		oldYaw;
#endif

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
#ifndef TURTLEARENA // NOARMOR
	int			damage_armor;		// damage absorbed by armor
#endif
	int			damage_blood;		// damage taken out of health
	int			damage_knockback;	// impact damage
	vec3_t		damage_from;		// origin for vector calculation
	qboolean	damage_fromWorld;	// if true, don't use the damage_from vector

#ifndef TURTLEARENA // AWARDS
	int			accurateCount;		// for "impressive" reward sound
#endif

	int			accuracy_shots;		// total number of shots
	int			accuracy_hits;		// total number of hits

	//
	int			lastkilled_client;	// last client that this client killed
	int			lasthurt_client;	// last client that damaged this client
	int			lasthurt_mod;		// type of damage the client did
#ifdef TA_WEAPSYS
	int			lasthurt_weapon;	// weapon/projectile for mod
#endif

#ifdef NIGHTSMODE
	int			mare;				// Client's current mare for NiGHTS mode. 0 means not on a mare.
#endif

	// timers
	int			respawnTime;		// can respawn when time > this, force after g_forcerespwan
	int			inactivityTime;		// kick players when time > this
	qboolean	inactivityWarning;	// qtrue if the five seoond warning has been given
	int			rewardTime;			// clear the EF_AWARD_IMPRESSIVE, etc when time > this

#ifdef TURTLEARENA // PLAYERS
	int			idleTime;			// swich to BOTH_WAITING animation after awhile
#endif

#ifndef TURTLEARENA // DROWNING
	int			airOutTime;
#endif

#ifndef TURTLEARENA // AWARDS
	int			lastKillTime;		// for multiple kill rewards
#endif

#ifdef TA_SP
	int			finishTime; // when the client finished the level.
#endif

#ifndef IOQ3ZTM
	qboolean	fireHeld;			// used for hook
#endif
	gentity_t	*hook;				// grapple hook if out

	int			switchTeamTime;		// time the player switched teams

	// timeResidual is used to handle events that happen every second
	// like health / armor countdowns and regeneration
	int			timeResidual;

#ifdef MISSIONPACK
	gentity_t	*persistantPowerup;
	int			portalID;
#ifdef TA_WEAPSYS
	int			ammoTimes[MAX_BG_WEAPON_GROUPS];
#else
	int			ammoTimes[WP_NUM_WEAPONS];
#endif
#ifdef TURTLEARENA // REGEN_SHURIKENS
	int			holdableTimes[MAX_HOLDABLE];
#endif
#ifndef TURTLEARENA // POWERS
	int			invulnerabilityTime;
#endif
#endif
#ifdef TA_WEAPSYS
	int melee_debounce; // Delay doing damage for primary weapon
	int melee_debounce2; // delay doing damage for secondary weapon
#endif
#ifdef TURTLEARENA // LOCKON
	int lockonTime;
	qboolean hadLockon;
#endif

	char		*areabits;
};


// A single client can have multiple players, for splitscreen.
typedef struct gconnection_s {
	int			numLocalPlayers;				// for quick access, the players could be any indexes in localPlayers[].
	int			localPlayerNums[MAX_SPLITVIEW];
} gconnection_t;


//
// this structure is cleared as each map is entered
//
#define	MAX_SPAWN_VARS			64
#define	MAX_SPAWN_VARS_CHARS	4096

typedef struct {
	struct gclient_s	*clients;		// [maxclients]

	struct gentity_s	*gentities;
	int			gentitySize;
	int			num_entities;		// MAX_CLIENTS <= num_entities <= ENTITYNUM_MAX_NORMAL

	gconnection_t	*connections;

	int			warmupTime;			// restart match at this time

	fileHandle_t	logFile;

	// store latched cvars here that we want to get at often
	int			maxclients;
	int			maxconnections;

	int			framenum;
	int			time;					// in msec
	int			previousTime;			// so movers can back up when blocked

	int			startTime;				// level.time the map was started

	int			teamScores[TEAM_NUM_TEAMS];
	int			lastTeamLocationTime;		// last time of client team location update

	qboolean	newSession;				// don't use any old session data, because
										// we changed gametype

	qboolean	restarted;				// waiting for a map_restart to fire

	int			numConnectedClients;
	int			numNonSpectatorClients;	// includes connecting clients
	int			numPlayingClients;		// connected, non-spectators
	int			sortedClients[MAX_CLIENTS];		// sorted by score
	int			follow1, follow2;		// clientNums for auto-follow spectators

	int			snd_fry;				// sound index for standing in lava

	int			warmupModificationCount;	// for detecting if g_warmup is changed

	// voting state
	char		voteString[MAX_STRING_CHARS];
	char		voteDisplayString[MAX_STRING_CHARS];
	int			voteTime;				// level.time vote was called
	int			voteExecuteTime;		// time the vote is executed
	int			voteYes;
	int			voteNo;
	int			numVotingClients;		// set by CalculateRanks

	// team voting state
	char		teamVoteString[2][MAX_STRING_CHARS];
	int			teamVoteTime[2];		// level.time vote was called
	int			teamVoteYes[2];
	int			teamVoteNo[2];
	int			numteamVotingClients[2];// set by CalculateRanks

	// spawn variables
	qboolean	spawning;				// the G_Spawn*() functions are valid
	int			numSpawnVars;
	char		*spawnVars[MAX_SPAWN_VARS][2];	// key / value pairs
	int			numSpawnVarChars;
	char		spawnVarChars[MAX_SPAWN_VARS_CHARS];

	// intermission state
	int			intermissionQueued;		// intermission was qualified, but
										// wait INTERMISSION_DELAY_TIME before
										// actually going there so the last
										// frag can be watched.  Disable future
										// kills during this delay
	int			intermissiontime;		// time the intermission was started
	char		*changemap;
	qboolean	readyToExit;			// at least one client wants to exit
	int			exitTime;
	vec3_t		intermission_origin;	// also used for spectator spawns
	vec3_t		intermission_angle;

	qboolean	locationLinked;			// target_locations get linked
	gentity_t	*locationHead;			// head of the location list
	int			bodyQueIndex;			// dead bodies
	gentity_t	*bodyQue[BODY_QUEUE_SIZE];
#ifdef MISSIONPACK
	int			portalSequence;
#endif
} level_locals_t;


//
// g_spawn.c
//
qboolean	G_SpawnString( const char *key, const char *defaultString, char **out );
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean	G_SpawnFloat( const char *key, const char *defaultString, float *out );
qboolean	G_SpawnInt( const char *key, const char *defaultString, int *out );
qboolean	G_SpawnVector( const char *key, const char *defaultString, float *out );
void		G_SpawnEntitiesFromString( void );
char *G_NewString( const char *string );

//
// g_cmds.c
//
void Cmd_Score_f (gentity_t *ent);
void StopFollowing( gentity_t *ent );
void BroadcastTeamChange( gclient_t *client, int oldTeam );
void SetTeam( gentity_t *ent, const char *s );
void Cmd_FollowCycle_f( gentity_t *ent, int dir );
#ifdef IOQ3ZTM // PEAKING
qboolean G_AllowPeaking(void);
#endif

//
// g_items.c
//
#define	RESPAWN_ARMOR		25
#define	RESPAWN_HEALTH		35
#ifdef TURTLEARENA // NIGHTS_ITEMS
#define	RESPAWN_SCORE		25
#endif
#define	RESPAWN_AMMO		40
#ifdef TURTLEARENA // HOLDABLE
#define	RESPAWN_HOLDABLE	35
#else
#define	RESPAWN_HOLDABLE	60
#endif
#define	RESPAWN_MEGAHEALTH	35//120
#define	RESPAWN_POWERUP		120

void G_CheckTeamItems( void );
void G_RunItem( gentity_t *ent );
void RespawnItem( gentity_t *ent );

void UseHoldableItem( gentity_t *ent );
void PrecacheItem (bg_iteminfo_t *it);
gentity_t *Drop_Item( gentity_t *ent, bg_iteminfo_t *item, float angle );
gentity_t *LaunchItem( bg_iteminfo_t *item, vec3_t origin, vec3_t velocity );
void SetRespawn (gentity_t *ent, float delay);
void G_SpawnItem (gentity_t *ent, bg_iteminfo_t *item);
void FinishSpawningItem( gentity_t *ent );
void Think_Weapon (gentity_t *ent);
int ArmorIndex (gentity_t *ent);
void	Add_Ammo (gentity_t *ent, int weapon, int count);
void Touch_Item (gentity_t *ent, gentity_t *other, trace_t *trace);

void ClearRegisteredItems( void );
void RegisterItem( bg_iteminfo_t *item );
void SaveRegisteredItems( void );

#ifdef TA_WEAPSYS // weapon_random
bg_iteminfo_t *G_RandomWeaponItem( gentity_t *ent, int flags );
#endif

//
// g_utils.c
//
int		G_FindConfigstringIndex( char *name, int start, int max, qboolean create );
int		G_ModelIndex( char *name );
int		G_SoundIndex( char *name );
#ifdef IOQ3ZTM // Particles
int		G_ParticleAreaIndex( char *str );
#endif
#ifdef TA_ENTSYS // MISC_OBJECT
int		G_StringIndex( char *name );
#endif
void	trap_SendServerCommand( int clientNum, char *cmd );
void	G_TeamCommand( team_t team, char *cmd );
void	G_KillBox (gentity_t *ent);
gentity_t *G_Find (gentity_t *from, int fieldofs, const char *match);
gentity_t *G_PickTarget (char *targetname);
void	G_UseTargets (gentity_t *ent, gentity_t *activator);
#ifdef TA_ENTSYS
void	G_UseTargets2(gentity_t *ent, gentity_t *activator, const char *target);
#endif
void	G_SetMovedir ( vec3_t angles, vec3_t movedir);

void	G_InitGentity( gentity_t *e );
gentity_t	*G_Spawn (void);
gentity_t *G_TempEntity( vec3_t origin, int event );
void	G_Sound( gentity_t *ent, int channel, int soundIndex );
void	G_FreeEntity( gentity_t *e );
qboolean	G_EntitiesFree( void );

void	G_TouchTriggers (gentity_t *ent);

float	*tv (float x, float y, float z);
char	*vtos( const vec3_t v );

float vectoyaw( const vec3_t vec );

void G_AddPredictableEvent( gentity_t *ent, int event, int eventParm );
void G_AddEvent( gentity_t *ent, int event, int eventParm );
void G_SetOrigin( gentity_t *ent, vec3_t origin );
void AddRemap(const char *oldShader, const char *newShader, float timeOffset);
const char *BuildShaderStateConfig( void );

#ifdef TA_WEAPSYS // XREAL r2785
gentity_t *G_FindRadius(gentity_t *from, const vec3_t org, float rad);
qboolean G_IsVisible(int skipEnt, const vec3_t start, const vec3_t goal);
qboolean G_ValidTarget(gentity_t *source, gentity_t *target,
		const vec3_t start, const vec3_t dir,
		float rad, float ang, int tests);
gentity_t *G_FindTarget(gentity_t *source, const vec3_t start, const vec3_t dir,
		float rad, float ang);
#endif

//
// g_combat.c
//
qboolean CanDamage (gentity_t *targ, vec3_t origin);
qboolean G_Damage (gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod);
#ifdef TA_WEAPSYS
qboolean G_RadiusDamage (vec3_t origin, gentity_t *inflictor, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod);
#else
qboolean G_RadiusDamage (vec3_t origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod);
#endif
int G_InvulnerabilityEffect( gentity_t *targ, vec3_t dir, vec3_t point, vec3_t impactpoint, vec3_t bouncedir );
void body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath );
void TossClientItems( gentity_t *self );
void TossClientGametypeItems(gentity_t *ent);
#ifdef MISSIONPACK
void TossClientPersistantPowerups( gentity_t *self );
#endif
void TossClientCubes( gentity_t *self );

// damage flags
#define DAMAGE_RADIUS				0x00000001	// damage was indirect
#define DAMAGE_NO_ARMOR				0x00000002	// armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK			0x00000004	// do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION		0x00000008  // armor, shields, invulnerability, and godmode have no effect
#ifdef MISSIONPACK
#define DAMAGE_NO_TEAM_PROTECTION	0x00000010  // armor, shields, invulnerability, and godmode have no effect
#endif
#ifdef TA_WEAPSYS // WIF_CUTS
#define DAMAGE_CUTS					0x00000020	// damage mustcut entities
#endif

//
// g_missile.c
//
void G_RunMissile( gentity_t *ent );

#ifdef TA_WEAPSYS
#ifdef TURTLEARENA // HOLD_SHURIKEN
qboolean fire_shuriken (gentity_t *self, vec3_t start, vec3_t forward,
		vec3_t right, vec3_t up, holdable_t holdable);
#endif

qboolean fire_projectile(gentity_t *self, vec3_t start, vec3_t forward,
		vec3_t right, vec3_t up, int projnum, float quadFactor,
		int mod, int splashMod, int handSide);

qboolean fire_weapon(gentity_t *self, vec3_t start, vec3_t forward,
		vec3_t right, vec3_t up, int weaponnum, float quadFactor, int handSide);

qboolean fire_weaponDir(gentity_t *self, vec3_t start, vec3_t dir,
		int weaponnum, float quadFactor, int handSide);
#else
gentity_t *fire_plasma (gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *fire_grenade (gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *fire_rocket (gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_bfg (gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_grapple (gentity_t *self, vec3_t start, vec3_t dir);
#ifdef MISSIONPACK
gentity_t *fire_nail( gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up );
gentity_t *fire_prox( gentity_t *self, vec3_t start, vec3_t aimdir );
#endif
#endif


//
// g_mover.c
//
void G_RunMover( gentity_t *ent );
void Touch_DoorTrigger( gentity_t *ent, gentity_t *other, trace_t *trace );
#ifdef TA_ENTSYS // BREAKABLE
qboolean G_SeenByHumans( gentity_t *ent );
#endif
#ifdef TA_ENTSYS // PUSHABLE
qboolean G_PlayerPush( gentity_t *pusher, vec3_t move, vec3_t amove, gentity_t **obstacle );
#endif

//
// g_trigger.c
//
void trigger_teleporter_touch (gentity_t *self, gentity_t *other, trace_t *trace );


//
// g_misc.c
//
void TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles );
#ifdef MISSIONPACK
void DropPortalSource( gentity_t *ent );
void DropPortalDestination( gentity_t *ent );
#endif


//
// g_weapon.c
//
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker );
void CalcMuzzlePoint ( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint );
void SnapVectorTowards( vec3_t v, vec3_t to );
#ifdef TURTLEARENA // LOCKON HOLD_SHURIKEN
void G_AutoAim(gentity_t *ent, int projnum, vec3_t start, vec3_t forward, vec3_t right, vec3_t up);
void G_ThrowShuriken(gentity_t *ent, holdable_t holdable);
#endif
#ifdef TA_WEAPSYS // MELEEATTACK
qboolean G_MeleeDamageSingle(gentity_t *ent, qboolean checkTeamHit, int hand, weapontype_t wt);
qboolean G_MeleeDamage( gentity_t *ent, qboolean attacking );
void G_StartMeleeAttack(gentity_t *ent);
#else
qboolean CheckGauntletAttack( gentity_t *ent );
#endif
#ifdef IOQ3ZTM // GRAPPLE_RETURN
void Weapon_ForceHookFree (gentity_t *ent);
#endif
void Weapon_HookFree (gentity_t *ent);
void Weapon_HookThink (gentity_t *ent);
#ifdef TA_NPC
void NPC_FireWeapon(gentity_t *ent);
#endif


//
// g_client.c
//
int TeamCount( int ignoreClientNum, team_t team );
int TeamLeader( int team );
team_t PickTeam( int ignoreClientNum );
void SetClientViewAngle( gentity_t *ent, vec3_t angle );
#ifdef TA_PLAYERSYS
gentity_t *SelectSpawnPoint (gentity_t *ent, vec3_t origin, vec3_t angles, qboolean isbot);
#else
gentity_t *SelectSpawnPoint (vec3_t avoidPoint, vec3_t origin, vec3_t angles, qboolean isbot);
#endif
void CopyToBodyQue( gentity_t *ent );
void ClientRespawn(gentity_t *ent);
void BeginIntermission (void);
void InitBodyQue (void);
void ClientSpawn( gentity_t *ent, qboolean firstTime );
void player_die (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);
#ifdef TURTLEARENA // NIGHTS_ITEMS
void AddScoreEx( gentity_t *ent, vec3_t origin, int score, int chain);
#endif
void AddScore( gentity_t *ent, vec3_t origin, int score );
void CalculateRanks( void );
#ifdef TA_PLAYERSYS
qboolean SpotWouldTelefrag( gentity_t *spot, gentity_t *ent );
#else
qboolean SpotWouldTelefrag( gentity_t *spot );
#endif
#ifdef TA_SP
qboolean G_ClientCompletedLevel(gentity_t *activator, char *nextMap);
#endif
#ifdef NIGHTSMODE
void G_DeNiGHTSizePlayer( gentity_t *ent );
#endif

//
// g_svcmds.c
//
qboolean	ConsoleCommand( void );
void G_RegisterCommands( void );
void G_ProcessIPBans(void);
qboolean G_FilterPacket (char *from);

//
// g_weapon.c
//
void FireWeapon( gentity_t *ent );
#if defined MISSIONPACK && !defined TURTLEARENA // NO_KAMIKAZE_ITEM
void G_StartKamikaze( gentity_t *ent );
#endif

#ifdef TA_NPCSYS
//
// g_npcsys.c
//
void ClearRegisteredNPCs( void );
void RegisterNPC( bg_npcinfo_t *npc );
void SaveRegisteredNPCs( void );
void G_SpawnNPC( gentity_t *ent, bg_npcinfo_t *npc );
void G_RunNPC( gentity_t *ent );
#endif

#ifdef TA_ENTSYS
void G_SetMiscAnim(gentity_t *ent, int anim);
#endif

//
// g_cmds.c
//
void DeathmatchScoreboardMessage( gentity_t *ent );
char *ConcatArgs( int start );

//
// g_main.c
//
void MoveClientToIntermission( gentity_t *ent );
void FindIntermissionPoint( void );
void SetLeader(int team, int client);
void CheckTeamLeader( int team );
void G_RunThink (gentity_t *ent);
void AddTournamentQueue(gclient_t *client);
void QDECL G_LogPrintf( const char *fmt, ... ) __attribute__ ((format (printf, 1, 2)));
void SendScoreboardMessageToAllClients( void );
void QDECL G_DPrintf( const char *fmt, ... ) __attribute__ ((format (printf, 1, 2)));
void QDECL G_Printf( const char *fmt, ... ) __attribute__ ((format (printf, 1, 2)));
void QDECL G_Error( const char *fmt, ... ) __attribute__ ((noreturn, format (printf, 1, 2)));
#ifdef IOQ3ZTM
void G_CvarClearModification( vmCvar_t *vmCvar );
#endif

//
// g_client.c
//
char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot, int connectionNum, int localPlayerNum );
void ClientUserinfoChanged( int clientNum );
void ClientDisconnect( int clientNum );
void ClientBegin( int clientNum );
void ClientCommand( int clientNum );
float ClientHandicap( gclient_t *client );

//
// g_active.c
//
void ClientThink( int clientNum );
void ClientEndFrame( gentity_t *ent );
void G_RunClient( gentity_t *ent );

//
// g_team.c
//
qboolean OnSameTeam( gentity_t *ent1, gentity_t *ent2 );
void Team_CheckDroppedItem( gentity_t *dropped );
qboolean CheckObeliskAttack( gentity_t *obelisk, gentity_t *attacker );
#if defined MISSIONPACK && defined TA_WEAPSYS
void ObeliskPain( gentity_t *self, gentity_t *attacker, int damage );
#endif

//
// g_session.c
//
void G_ReadSessionData( gclient_t *client );
void G_InitSessionData( gclient_t *client, char *userinfo );

void G_InitWorldSession( void );
void G_WriteSessionData( void );

#ifdef TA_SP
void G_ReadCoopSessionData( gclient_t *client );
void G_InitCoopSessionData( gclient_t *client );

void G_WriteCoopSessionData( qboolean restart );
#endif

//
// g_arenas.c
//
void UpdateTournamentInfo( void );
#ifndef TA_SP
void SpawnModelsOnVictoryPads( void );
void Svcmd_AbortPodium_f( void );
#endif

//
// g_bot.c
//
void G_InitBots( qboolean restart );
char *G_GetBotInfoByNumber( int num );
char *G_GetBotInfoByName( const char *name );
void G_CheckBotSpawn( void );
void G_RemoveQueuedBotBegin( int clientNum );
qboolean G_BotConnect( int clientNum, qboolean restart );
void Svcmd_AddBot_f( void );
void Svcmd_BotList_f( void );
void BotInterbreedEndMatch( void );
#ifdef IOQ3ZTM // MAP_ROTATION
void G_LoadArenas( void );
const char *G_GetNextArenaInfoByGametype( const char *map, gametype_t gametype );
void G_AdvanceMap( void );
#endif

#ifdef TA_SP // Load/save
//
// g_save.c
//
qboolean G_SaveGame(const char *savegame);
void G_LoadGame(void);
void G_LoadGameClient(int client);
#endif

#ifdef TA_PATHSYS
//
// g_paths.c
//
// Path type, save in entities?
typedef enum
{
	PATH_UNKNOWN= 0x00,		// zero value

	// values returned by G_SetupPath
	PATH_ERROR	= 0x01,		// Path failed linking.
	PATH_CIRCIT	= 0x02,		// Path is a circit.
	PATH_LINE	= 0x04,		// Path has a start and end.
					// ... (rename? doesn't mean its a strait line, just not a circit)

	// other flags (saved in path entities)
	PATH_FIRST	= 0x08,
	PATH_LAST	= 0x10,

	PATH_DUMMY
} gpathtype_e;

gpathtype_e G_SetupPath(gentity_t *ent, const char *target);
qboolean G_ReachedPath(gentity_t *ent, qboolean check);
void G_MoveOnPath(gentity_t *ent);
#endif

// ai_main.c
#define MAX_FILEPATH			144

//bot settings
typedef struct bot_settings_s
{
	char characterfile[MAX_FILEPATH];
	float skill;
} bot_settings_t;

int BotAISetup( int restart );
int BotAIShutdown( int restart );
int BotAILoadMap( int restart );
int BotAISetupClient(int client, struct bot_settings_s *settings, qboolean restart);
int BotAIShutdownClient( int client, qboolean restart );
int BotAIStartFrame( int time );
void BotTestAAS(vec3_t origin);

#include "g_team.h" // teamplay specific stuff
#include "g_syscalls.h"


extern	level_locals_t	level;
extern	gentity_t		g_entities[MAX_GENTITIES];

#define	FOFS(x) ((size_t)&(((gentity_t *)0)->x))

extern	vmCvar_t	g_gametype;
extern	vmCvar_t	g_dedicated;
extern	vmCvar_t	g_cheats;
extern	vmCvar_t	g_maxclients;			// allow this many total, including spectators
extern	vmCvar_t	g_maxGameClients;		// allow this many active
extern	vmCvar_t	g_restarted;

extern	vmCvar_t	g_dmflags;
extern	vmCvar_t	g_fraglimit;
extern	vmCvar_t	g_timelimit;
extern	vmCvar_t	g_capturelimit;
extern	vmCvar_t	g_friendlyFire;
extern	vmCvar_t	g_password;
extern	vmCvar_t	g_needpass;
extern	vmCvar_t	g_gravity;
extern	vmCvar_t	g_speed;
extern	vmCvar_t	g_knockback;
extern	vmCvar_t	g_quadfactor;
extern	vmCvar_t	g_forcerespawn;
extern	vmCvar_t	g_inactivity;
extern	vmCvar_t	g_debugMove;
extern	vmCvar_t	g_debugDamage;
extern	vmCvar_t	g_weaponRespawn;
extern	vmCvar_t	g_weaponTeamRespawn;
extern	vmCvar_t	g_synchronousClients;
extern	vmCvar_t	g_motd;
extern	vmCvar_t	g_warmup;
extern	vmCvar_t	g_doWarmup;
#ifndef NOTRATEDM
extern	vmCvar_t	g_blood;
#endif
extern	vmCvar_t	g_allowVote;
extern	vmCvar_t	g_teamAutoJoin;
extern	vmCvar_t	g_teamForceBalance;
extern	vmCvar_t	g_banIPs;
extern	vmCvar_t	g_filterBan;
extern	vmCvar_t	g_obeliskHealth;
extern	vmCvar_t	g_obeliskRegenPeriod;
extern	vmCvar_t	g_obeliskRegenAmount;
extern	vmCvar_t	g_obeliskRespawnDelay;
#ifdef MISSIONPACK_HARVESTER
extern	vmCvar_t	g_cubeTimeout;
#endif
extern	vmCvar_t	g_redteam;
extern	vmCvar_t	g_blueteam;
extern	vmCvar_t	g_smoothClients;
extern	vmCvar_t	pmove_fixed;
extern	vmCvar_t	pmove_msec;
extern	vmCvar_t	g_rankings;
#ifdef MISSIONPACK
extern	vmCvar_t	g_enableDust;
extern	vmCvar_t	g_enableBreath;
#ifndef TA_SP
extern	vmCvar_t	g_singlePlayer;
#endif
#endif
extern	vmCvar_t	g_playerCapsule;
#ifdef TA_SP
extern	vmCvar_t	g_singlePlayer;
extern	vmCvar_t	g_savegameLoading;
extern	vmCvar_t	g_savegameFilename;
extern	vmCvar_t	g_spSaveData;
extern	vmCvar_t	g_saveVersions;
extern	vmCvar_t	g_saveFilename;
extern	vmCvar_t	g_saveMapname;
#endif
extern	vmCvar_t	g_proxMineTimeout;
#ifdef TURTLEARENA // POWERS // PW_FLASHING
extern	vmCvar_t	g_teleportFluxTime;
#endif
#ifdef IOQ3ZTM // LASERTAG
extern	vmCvar_t	g_laserTag;
#endif
#ifdef TA_PATHSYS // 2DMODE
extern	vmCvar_t	g_2dmode;
#endif

